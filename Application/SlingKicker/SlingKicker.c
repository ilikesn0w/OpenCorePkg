/** @file
  Run Apple SlingShot.

  Copyright (c) 2020, vit9696. All rights reserved.
  Copyright (c) 2026, ilikesn0w. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>

#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Library/OcBootManagementLib.h>
#include <Library/OcConsoleLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcStringLib.h>

#include <Guid/AppleFile.h>

#include <Protocol/GraphicsOutput.h>
#include <Protocol/LoadedImage.h>

//
// Defines which macOS version SlingShot will launch.
// default = OOB (or nearest to it) version.
// latest  = Latest supported for device.
// CHAR8 gAppleVendorVariableGuid
// TODO: Move to AppleVariable.h
//
#define APPLE_INTERNET_RECOVERY_TYPE_VARIABLE_NAME  L"internet-recovery-type"

#define APPLE_SLING_SHOT_REC_DISK_OPT   L"RecoveryModeDisk"
#define APPLE_SLING_SHOT_REC_NET_OPT    L"RecoveryModeNetwork"
#define APPLE_SLING_SHOT_DIAG_DISK_OPT  L"DiagsModeDisk"
#define APPLE_SLING_SHOT_DIAG_NET_OPT   L"DiagsModeNetwork"

#define APPLE_SLING_SHOT_REC_TYPE_DEFAULT  "default"
#define APPLE_SLING_SHOT_REC_TYPE_LATEST   "latest"

//
// SlingShot supports 4 modes:
// RecoveryModeDisk is equivalent to CMD+R
// RecoveryModeNetwork is equivalent to CMD+OPT(+SHIFT)+R
// DiagsModeDisk is equivalent to D
// DiagsModeNetwork is equivalent to OPT + D
// TODO: Recheck RecoveryModeDisk and DiagsModeDisk.
//
typedef enum {
  ///
  /// RecoveryModeDisk will be given as LoadOption to SlingShot and it will launch OOB macOS.
  ///
  SlingShotModeRecDisk,
  ///
  /// RecoveryModeNetwork will be given as LoadOption to SlingShot and it will launch OOB macOS.
  ///
  SlingShotModeRecNet,
  ///
  /// RecoveryModeDisk will be given as LoadOption to SlingShot and it will launch latest available macOS.
  ///
  SlingShotModeRecDiskLatest,
  ///
  /// RecoveryModeNetwork will be given as LoadOption to SlingShot and it will launch latest available macOS.
  ///
  SlingShotModeRecNetLatest,
  ///
  /// DiagsModeDisk will be given as LoadOption to SlingShot.
  ///
  SlingShotModeDiagDisk,
  ///
  /// DiagsModeNetwork will be given as LoadOption to SlingShot.
  ///
  SlingShotModeDiagNet
} SLING_SHOT_BOOT_MODE;

STATIC
EFI_STATUS
PrintHelp (
  VOID
  )
{
  Print (L"Usage: SlingKicker <RecDisk|RecDiskLatest|RecNet|RecNetLatest|DiagDisk|DiagNet>\n");
  return EFI_INVALID_PARAMETER;
}

//
// TODO: Move this to OcBootManagementLib.
//
STATIC
EFI_STATUS
OcRunFirmwareApplicationWithLoadOptions (
  IN EFI_GUID  *ApplicationGuid,
  IN CHAR16    *Options
  )
{
  EFI_STATUS                 Status;
  EFI_HANDLE                 NewHandle;
  EFI_DEVICE_PATH_PROTOCOL   *Dp;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;

  if ((ApplicationGuid == NULL) || (Options == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((DEBUG_INFO, "SlingKicker: run fw app attempting to find %g...\n", ApplicationGuid));

  Dp = OcCreateFvFileDevicePath (ApplicationGuid);
  if (Dp == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  DEBUG ((DEBUG_INFO, "SlingKicker: run fw app attempting to load %g...\n", ApplicationGuid));
  NewHandle = NULL;
  Status    = gBS->LoadImage (
                     FALSE,
                     gImageHandle,
                     Dp,
                     NULL,
                     0,
                     &NewHandle
                     );
  if (EFI_ERROR (Status)) {
    FreePool (Dp);
    return Status;
  }

  FreePool (Dp);

  LoadedImage = NULL;
  Status      = OcHandleProtocolFallback (NewHandle, &gEfiLoadedImageProtocolGuid, (VOID **)&LoadedImage);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SlingKicker: Failed to get LoadedImage for app - %r\n", Status));
    gBS->UnloadImage (NewHandle);
    return Status;
  }

  LoadedImage->LoadOptions     = Options;
  LoadedImage->LoadOptionsSize = (UINT32)StrSize (Options);

  DEBUG ((
    DEBUG_INFO,
    "SlingKicker: run fw app attempting to start %g with %s...\n",
    ApplicationGuid,
    Options
    ));
  Status = gBS->StartImage (
                  NewHandle,
                  NULL,
                  NULL
                  );

  return Status;
}

//
// TODO: Move this to OcBootManagementLib.
//
STATIC
EFI_STATUS
OcLaunchAppleSlingShot (
  IN SLING_SHOT_BOOT_MODE  Mode
  )
{
  EFI_STATUS   Status;
  CHAR16       *LoadOptions;
  CONST CHAR8  *RecMode;

  OcUnlockAppleFirmwareUI ();

  if (Mode == SlingShotModeRecDisk) {
    LoadOptions = APPLE_SLING_SHOT_REC_DISK_OPT;
  } else if ((Mode == SlingShotModeRecNet) || (Mode == SlingShotModeRecNetLatest)) {
    LoadOptions = APPLE_SLING_SHOT_REC_NET_OPT;
  } else if ((Mode == SlingShotModeDiagDisk) || (Mode == SlingShotModeRecDiskLatest)) {
    LoadOptions = APPLE_SLING_SHOT_DIAG_DISK_OPT;
  } else if (Mode == SlingShotModeDiagNet) {
    LoadOptions = APPLE_SLING_SHOT_DIAG_NET_OPT;
  }

  RecMode = NULL;
  if ((Mode == SlingShotModeRecDisk) || (Mode == SlingShotModeRecNet) || (Mode == SlingShotModeRecDiskLatest) || (Mode == SlingShotModeRecNetLatest)) {
    if ((Mode == SlingShotModeRecDisk) || (Mode == SlingShotModeRecNet)) {
      RecMode = APPLE_SLING_SHOT_REC_TYPE_DEFAULT;
    } else {
      RecMode = APPLE_SLING_SHOT_REC_TYPE_LATEST;
    }

    Status = gRT->SetVariable (
                    APPLE_INTERNET_RECOVERY_TYPE_VARIABLE_NAME,
                    &gAppleVendorVariableGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    AsciiStrLen (RecMode),
                    (VOID *)RecMode
                    );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "SlingKicker: Failed to set internet-recovery-type - %r\n", Status));
      return Status;
    }
  }

  Status = OcRunFirmwareApplicationWithLoadOptions (&gAppleSlingShotFileGuid, LoadOptions);

  return Status;
}

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                           Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL         *Gop;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION  Pixel;
  UINTN                                Index;
  UINTN                                Argc;
  CHAR16                               **Argv;
  SLING_SHOT_BOOT_MODE                 BootMode;

  gBS->SetWatchdogTimer (0, 0, 0, NULL);

  OcProvideConsoleGop (FALSE);

  OcSetConsoleResolution (0, 0, 0, FALSE);

  Status = gBS->HandleProtocol (
                  gST->ConsoleOutHandle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **)&Gop
                  );

  if (EFI_ERROR (Status)) {
    //
    // Note: Ensure that stall value is within UINT32 in nanoseconds, for
    // firmware such as APTIO IV with less robust Stall implementations.
    //
    for (Index = 0; Index < 10; ++Index) {
      gBS->Stall (SECONDS_TO_MICROSECONDS (1));
    }

    return Status;
  }

  Status = GetArguments (&Argc, &Argv);

  if (EFI_ERROR (Status) || (Argc < 2)) {
    return PrintHelp ();
  }

  if (OcStriCmp (Argv[1], L"RecDisk") == 0) {
    BootMode = SlingShotModeRecDisk;
  } else if (OcStriCmp (Argv[1], L"RecDiskLatest") == 0) {
    BootMode = SlingShotModeRecDiskLatest;
  } else if (OcStriCmp (Argv[1], L"RecNet") == 0) {
    BootMode = SlingShotModeRecNet;
  } else if (OcStriCmp (Argv[1], L"RecNetLatest") == 0) {
    BootMode = SlingShotModeRecNetLatest;
  } else if (OcStriCmp (Argv[1], L"DiagDisk") == 0) {
    BootMode = SlingShotModeDiagDisk;
  } else if (OcStriCmp (Argv[1], L"DiagNet") == 0) {
    BootMode = SlingShotModeDiagNet;
  } else {
    return PrintHelp ();
  }

  Status = OcLaunchAppleSlingShot (BootMode);

  Pixel.Raw = 0x0;
  if (Status == EFI_NOT_FOUND) {
    //
    // Red. No SlingShot in firmware or we cannot get it.
    //
    Pixel.Pixel.Red = 0xFF;
  } else if (Status == EFI_UNSUPPORTED) {
    //
    // Yellow. SlingShot does not start.
    //
    Pixel.Pixel.Red   = 0xFF;
    Pixel.Pixel.Green = 0xFF;
  } else {
    //
    // Fuchsia. SlingShot does not load.
    //
    Pixel.Pixel.Blue = 0xFF;
    Pixel.Pixel.Red  = 0xFF;
  }

  Gop->Blt (
         Gop,
         &Pixel.Pixel,
         EfiBltVideoFill,
         0,
         0,
         0,
         0,
         Gop->Mode->Info->HorizontalResolution,
         Gop->Mode->Info->VerticalResolution,
         0
         );

  //
  // Note: Ensure that stall value is within UINT32 in nanoseconds.
  //
  for (Index = 0; Index < 10; ++Index) {
    gBS->Stall (SECONDS_TO_MICROSECONDS (1));
  }

  return EFI_SUCCESS;
}
