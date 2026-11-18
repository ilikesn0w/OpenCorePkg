/** @file
  Apple Device Control protocol implementation.

  Copyright (c) 2026, ilikesn0w. All rights reserved.
  Portions copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>
#include <PiDxe.h>

#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Library/OcMiscLib.h>

#include <Guid/GlobalVariable.h>

#include <Guid/AppleVariable.h>

#include <Protocol/ConsoleControl.h>
#include <Protocol/DevicePath.h>
#include <Protocol/EdidActive.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/PciIo.h>

#include <Protocol/AppleDeviceControl.h>

#include <IndustryStandard/Pci22.h>

#define APPLE_CON_OUT  BIT0 ///< 1
#define APPLE_ERR_OUT  BIT1 ///< 2
#define APPLE_CON_IN   BIT2 ///< 4

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL    *Dp;
  UINT64                      Attributes;
} APPLE_CONSOLE_DEVICE;

STATIC APPLE_CONSOLE_DEVICE  mConsoleDevices[] = {
  { NULL, 0 }
};

STATIC BOOLEAN  mDisplayWasConnected;

STATIC
EFI_STATUS
InternalSanitizeConsoleVariable (
  IN CHAR16  *Name
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *Data;
  EFI_DEVICE_PATH_PROTOCOL  *Dup;
  EFI_DEVICE_PATH_PROTOCOL  *DupStart;
  EFI_DEVICE_PATH_PROTOCOL  *Instance;
  EFI_DEVICE_PATH_PROTOCOL  *End;
  UINTN                     DpSize;

  Data   = NULL;
  Status = GetEfiGlobalVariable2 (Name, (VOID **)&Data, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCADC: Failed to get %s - %r\n", Name, Status));
    return Status;
  }

  if (Data == NULL) {
    return EFI_UNSUPPORTED;
  }

  Dup = DuplicateDevicePath (Data);
  FreePool (Data);
  if (Dup == NULL) {
    DEBUG ((DEBUG_INFO, "OCADC: Duplicated device path is NULL\n"));
    return EFI_NOT_FOUND;
  }

  DupStart = Dup;
  Status   = EFI_NOT_FOUND;

  do {
    Instance = GetNextDevicePathInstance (&Dup, &DpSize);
    if (Instance == NULL) {
      DEBUG ((DEBUG_INFO, "OCADC: Next device path is NULL\n"));
      break;
    }

    End = Instance;
    while (!IsDevicePathEndType (End)) {
      End = NextDevicePathNode (End);
    }

    SetDevicePathEndNode (End);

    Status = EfiBootManagerConnectDevicePath (Instance, NULL);
    FreePool (Instance);
  } while (Dup != NULL);

  FreePool (DupStart);
  return Status;
}

STATIC
EFI_STATUS
InternalSanitizeConsoleVariables (
  VOID
  )
{
  EFI_STATUS  Status;
  VOID        *Data;

  Data   = NULL;
  Status = InternalSanitizeConsoleVariable (EFI_CON_OUT_VARIABLE_NAME);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCADC: Failed to sanitize ConOut - %r\n", Status));
  }

  Status = InternalSanitizeConsoleVariable (EFI_CON_IN_VARIABLE_NAME);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCADC: Failed to sanitize ConIn - %r\n", Status));
  }

  Status = GetEfiGlobalVariable2 (
             EFI_ERR_OUT_VARIABLE_NAME,
             &Data,
             NULL
             );
  if (!EFI_ERROR (Status) && (Data != NULL)) {
    FreePool (Data);
    Status = InternalSanitizeConsoleVariable (EFI_ERR_OUT_VARIABLE_NAME);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCADC: Failed to sanitize ErrOut - %r\n", Status));
    }
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
InternalConnectDisplay (
  IN APPLE_CONSOLE_DEVICE  *ConsoleDevices
  )
{
  EFI_STATUS                    Status;
  EFI_PCI_IO_PROTOCOL           *PciIo;
  EFI_EDID_ACTIVE_PROTOCOL      *EdidActive;
  VOID                          *ConOutData;
  VOID                          *ConInData;
  UINTN                         HandleCount;
  EFI_HANDLE                    *HandleBuffer;
  UINT8                         PciConfig[64];
  UINTN                         Index;
  EFI_DEVICE_PATH_PROTOCOL      *Dp;
  UINT64                        ConsoleDevicesAttributes;
  EFI_CONSOLE_CONTROL_PROTOCOL  *ConControl;
  UINT8                         *Edid;
  UINT32                        HeightCm;
  UINT32                        VActive;
  UINT32                        Divisor;
  UINT16                        DisplayDensity;
  UINT16                        ActualDensity;
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *Gop;
  UINTN                         ScaleSize;
  UINT8                         UiScale;
  UINT16                        DensityThreshold;

  mDisplayWasConnected = TRUE;

  ConOutData = NULL;
  Status     = GetVariable2 (
                 EFI_CON_OUT_VARIABLE_NAME,
                 &gEfiGlobalVariableGuid,
                 &ConOutData,
                 NULL
                 );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCADC: Failed to get ConOut - %r\n", Status));
    ConOutData = NULL;
  }

  ConInData = NULL;
  Status    = GetVariable2 (
                EFI_CON_IN_VARIABLE_NAME,
                &gEfiGlobalVariableGuid,
                &ConInData,
                NULL
                );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCADC: Failed to get ConIn - %r\n", Status));
    ConInData = NULL;
  }

  HandleCount  = 0;
  HandleBuffer = NULL;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OCADC: Failed to locate handle buffer for PciIo - %r\n", Status));
    FreePool (ConOutData);
    FreePool (ConInData);
    return Status;
  } else if (HandleBuffer == NULL) {
    DEBUG ((DEBUG_ERROR, "OCADC: Handle buffer for PciIo is NULL\n"));
    FreePool (ConOutData);
    FreePool (ConInData);
    return EFI_NOT_FOUND;
  }

  for (Index = 0; Index < HandleCount; ++Index) {
    PciIo = NULL;
    Dp    = NULL;

    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiPciIoProtocolGuid,
                    (VOID **)&PciIo
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OCADC: Failed to handle PciIo[%u] - %r\n", (UINT32)Index, Status));
      continue;
    }

    Status = PciIo->Pci.Read (
                          PciIo,
                          EfiPciIoWidthUint32,
                          0,
                          16,
                          PciConfig
                          );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OCADC: Failed to PciIo->Pci.Read on PciIo[%u] - %r\n", (UINT32)Index, Status));
      continue;
    }

    if (PciConfig[0x0B] != PCI_CLASS_DISPLAY) {
      DEBUG ((DEBUG_VERBOSE, "OCADC: PciConfig[0x0B] on PciIo[%u] is not a display\n", (UINT32)Index));
      continue;
    }

    Status = gBS->ConnectController (
                    HandleBuffer[Index],
                    NULL,
                    NULL,
                    TRUE
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCADC: Failed to ConnectController on HandleBuffer[%u] - %r\n", (UINT32)Index, Status));
    }

    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiDevicePathProtocolGuid,
                    (VOID **)&Dp
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OCADC: Failed to handle Dp[%u] - %r\n", (UINT32)Index, Status));
      continue;
    }

    Status = EfiBootManagerUpdateConsoleVariable (
               ConOutDev,
               Dp,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCADC: Failed to update ConOutDev on Dp[%u] - %r\n", (UINT32)Index, Status));
    }
  }

  FreePool (HandleBuffer);

  if (((ConOutData == NULL) || (ConInData == NULL)) && (ConsoleDevices != NULL)) {
    for ( ; ConsoleDevices->Dp != NULL; ++ConsoleDevices) {
      ConsoleDevicesAttributes = ConsoleDevices->Attributes;

      if ((ConsoleDevicesAttributes & APPLE_CON_IN) != 0) {
        Status = EfiBootManagerUpdateConsoleVariable (
                   ConIn,
                   ConsoleDevices->Dp,
                   NULL
                   );
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_INFO, "OCADC: Failed to update ConIn - %r\n", Status));
        }
      }

      if ((ConsoleDevicesAttributes & APPLE_CON_OUT) != 0) {
        Status = EfiBootManagerUpdateConsoleVariable (
                   ConOut,
                   ConsoleDevices->Dp,
                   NULL
                   );
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_INFO, "OCADC: Failed to update ConOut - %r\n", Status));
        }
      }

      if ((ConsoleDevicesAttributes & APPLE_ERR_OUT) != 0) {
        Status = EfiBootManagerUpdateConsoleVariable (
                   ErrOut,
                   ConsoleDevices->Dp,
                   NULL
                   );
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_INFO, "OCADC: Failed to update ErrOut - %r\n", Status));
        }
      }
    }
  }

  FreePool (ConOutData);
  FreePool (ConInData);

  Status = InternalSanitizeConsoleVariables ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCADC: Failed to sanitize console variables - %r\n", Status));
  }

  ConControl = NULL;
  Status     = gBS->LocateProtocol (
                      &gEfiConsoleControlProtocolGuid,
                      NULL,
                      (VOID **)&ConControl
                      );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OCADC: Failed to locate ConsoleControl - %r\n", Status));
  } else {
    Status = ConControl->SetMode (ConControl, EfiConsoleControlScreenGraphics);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OCADC: Failed to ConControl->SetMode - %r\n", Status));
    }
  }

  EdidActive = NULL;
  Status     = gBS->LocateProtocol (
                      &gEfiEdidActiveProtocolGuid,
                      NULL,
                      (VOID **)&EdidActive
                      );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OCADC: Failed to locate EdidActive - %r\n", Status));
    return EFI_SUCCESS;
  }

  if ((EdidActive->SizeOfEdid == 0) || ((EdidActive->SizeOfEdid & 0x7F) != 0)) {
    return EFI_SUCCESS;
  }

  Edid = EdidActive->Edid;

  if ((UINT32)(Edid[18] * 256 + Edid[19]) - 259 > 1) {
    return EFI_SUCCESS;
  }

  if ((Edid[21] == 0) || (Edid[22] == 0) || (Edid[54] == 0) || (Edid[55] == 0)) {
    return EFI_SUCCESS;
  }

  HeightCm = Edid[22];
  VActive  = ((UINT32)(Edid[61] & 0xF0) << 4) | Edid[59];
  Divisor  = 100 * HeightCm;

  DisplayDensity = (UINT16)(254ULL * VActive / Divisor);
  Status         = gRT->SetVariable (
                          L"DisplayDensity",
                          &gAppleBootVariableGuid,
                          EFI_VARIABLE_BOOTSERVICE_ACCESS,
                          sizeof (DisplayDensity),
                          &DisplayDensity
                          );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCADC: Failed to set DisplayDensity - %r\n", Status));
  }

  Gop    = NULL;
  Status = gBS->HandleProtocol (
                  gST->ConsoleOutHandle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **)&Gop
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OCADC: Failed to locate GOP - %r\n", Status));
    return EFI_SUCCESS;
  }

  ActualDensity = (UINT16)(254ULL * Gop->Mode->Info->VerticalResolution / Divisor);
  Status        = gRT->SetVariable (
                         L"ActualDensity",
                         &gAppleBootVariableGuid,
                         EFI_VARIABLE_BOOTSERVICE_ACCESS,
                         sizeof (ActualDensity),
                         &ActualDensity
                         );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCADC: Failed to set ActualDensity - %r\n", Status));
  }

  ScaleSize = sizeof (UiScale);
  Status    = gRT->GetVariable (
                     APPLE_UI_SCALE_VARIABLE_NAME,
                     &gAppleVendorVariableGuid,
                     NULL,
                     &ScaleSize,
                     &UiScale
                     );
  if (EFI_ERROR (Status)) {
    DensityThreshold = 0x7FFF;
    Status           = gRT->SetVariable (
                              L"DensityThreshold",
                              &gAppleBootVariableGuid,
                              EFI_VARIABLE_BOOTSERVICE_ACCESS,
                              sizeof (DensityThreshold),
                              &DensityThreshold
                              );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCADC: Failed to set DensityThreshold - %r\n", Status));
    }

    UiScale = (UINT8)(2 - (ActualDensity < DensityThreshold));
    Status  = gRT->SetVariable (
                     APPLE_UI_SCALE_VARIABLE_NAME,
                     &gAppleVendorVariableGuid,
                     EFI_VARIABLE_BOOTSERVICE_ACCESS,
                     sizeof (UiScale),
                     &UiScale
                     );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCADC: Failed to set UIScale - %r\n", Status));
    }
  }

  return EFI_SUCCESS;
}

/**
  Connect all the drivers to all the controllers.

  This function makes sure all the current system drivers manage the correspoinding
  controllers if have. And at the same time, makes sure all the system controllers
  have driver to manage it if have.
**/
STATIC
VOID
InternalConnectAllDriversToAllControllers (
  VOID
  )
{
  EFI_STATUS  Status;
  UINTN       HandleCount;
  EFI_HANDLE  *HandleBuffer;
  UINTN       Index;

  do {
    //
    // Connect All EFI 1.10 drivers following EFI 1.10 algorithm
    //
    gBS->LocateHandleBuffer (
           AllHandles,
           NULL,
           NULL,
           &HandleCount,
           &HandleBuffer
           );

    for (Index = 0; Index < HandleCount; Index++) {
      gBS->ConnectController (HandleBuffer[Index], NULL, NULL, TRUE);
    }

    if (HandleBuffer != NULL) {
      FreePool (HandleBuffer);
    }

    //
    // Check to see if it's possible to dispatch an more DXE drivers.
    // The above code may have made new DXE drivers show up.
    // If any new driver is dispatched (Status == EFI_SUCCESS) and we will try
    // the connect again.
    //
    Status = gDS->Dispatch ();
  } while (!EFI_ERROR (Status));
}

STATIC
EFI_STATUS
InternalConnectAll (
  VOID
  )
{
  EFI_STATUS  Status;

  if (mDisplayWasConnected) {
    Status = EfiBootManagerConnectAllDefaultConsoles ();
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCADC: Failed to connect default consoles - %r\n", Status));
    }
  } else {
    InternalConnectDisplay (mConsoleDevices);
  }

  InternalConnectAllDriversToAllControllers ();

  Status = EfiBootManagerConnectAllDefaultConsoles ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCADC: Failed to connect default consoles - %r\n", Status));
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
OcAppleDeviceControlConnectDisplay (
  VOID
  )
{
  return InternalConnectDisplay (mConsoleDevices);
}

STATIC
EFI_STATUS
EFIAPI
OcAppleDeviceControlConnectPci (
  VOID
  )
{
  //
  // boot.efi doesn't use this.
  //
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
OcAppleDeviceControlConnectAll (
  VOID
  )
{
  return InternalConnectAll ();
}

STATIC APPLE_DEVICE_CONTROL_PROTOCOL  mAppleDeviceControlProtocol = {
  APPLE_DEVICE_CONTROL_PROTOCOL_VERSION,
  OcAppleDeviceControlConnectDisplay,
  OcAppleDeviceControlConnectPci,
  OcAppleDeviceControlConnectAll
};

APPLE_DEVICE_CONTROL_PROTOCOL *
OcAppleDeviceControlInstallProtocol (
  IN BOOLEAN  Reinstall
  )
{
  EFI_STATUS                     Status;
  APPLE_DEVICE_CONTROL_PROTOCOL  *DevControl;
  EFI_HANDLE                     NewHandle;

  DevControl = NULL;
  if (Reinstall) {
    Status = OcUninstallAllProtocolInstances (&gAppleDeviceControlProtocolGuid);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OCADC: Uninstall failed - %r\n", Status));
      return NULL;
    }
  } else {
    Status = gBS->LocateProtocol (
                    &gAppleDeviceControlProtocolGuid,
                    NULL,
                    (VOID **)&DevControl
                    );
    if (!EFI_ERROR (Status)) {
      return DevControl;
    }
  }

  NewHandle = NULL;
  Status    = gBS->InstallMultipleProtocolInterfaces (
                     &NewHandle,
                     &gAppleDeviceControlProtocolGuid,
                     &mAppleDeviceControlProtocol,
                     NULL
                     );

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return &mAppleDeviceControlProtocol;
}
