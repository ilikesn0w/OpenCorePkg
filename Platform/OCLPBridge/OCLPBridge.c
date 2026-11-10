/** @file
  OCLP bridge driver.

  Copyright (c) 2026, ilikesn0w. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "OCLPBridgePrivate.h"

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Library/OcMiscLib.h>
#include <Library/OcStringLib.h>

#include <Guid/OcVariable.h>

#include <Protocol/DriverBinding.h>

#include <Protocol/OcInterface.h>

STATIC CHAR8  mOcVersion[64];
STATIC CHAR8  mOCLPVersion[64];
STATIC CHAR8  mTitleSuffix[160];

STATIC OC_POPULATE_CONTEXT  mExistingPopulate;

/**
  Maybe it's bad to return EFI_UNSUPPORTED in binding, but IDC.
**/
STATIC
EFI_STATUS
EFIAPI
OCLPBridgeDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
OCLPBridgeDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
OCLPBridgeDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                  ControllerHandle,
  IN  UINTN                       NumberOfChildren,
  IN  EFI_HANDLE                  *ChildHandleBuffer OPTIONAL
  )
{
  return EFI_UNSUPPORTED;
}

STATIC EFI_DRIVER_BINDING_PROTOCOL  mOCLPBridgeDriverBinding = {
  OCLPBridgeDriverBindingSupported,
  OCLPBridgeDriverBindingStart,
  OCLPBridgeDriverBindingStop,
  1,
  NULL,
  NULL
};

STATIC
EFI_STATUS
EFIAPI
OverrideOcPickerContext (
  IN     OC_INTERFACE_PROTOCOL  *This,
  IN     OC_STORAGE_CONTEXT     *Storage,
  IN OUT OC_PICKER_CONTEXT      *Picker
  )
{
  EFI_STATUS  Status;

  Status = mExistingPopulate (This, Storage, Picker);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OCLPBridge: Failed to populate context using existing interface protocol - %r\n", Status));
    return Status;
  }

  //
  // Check if TitleSuffix is NULL and interpret it as OCS_EXPOSE_VERSION_UI is not set.
  // Yes, maybe it's bad, but I think it's still better than getting config from storage.
  //
  if ((Picker->TitleSuffix == NULL) || (Picker->TitleSuffix[0] == '\0')) {
    DEBUG ((DEBUG_WARN, "OCLPBridge: OCS_EXPOSE_VERSION_UI is set, cannot set version string\n"));
    return Status;
  }

  Picker->TitleSuffix = mTitleSuffix;

  return Status;
}

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS             Status;
  OC_INTERFACE_PROTOCOL  *Existing;
  UINTN                  DataSize;

  Status = EfiLibInstallAllDriverProtocols2 (
             ImageHandle,
             SystemTable,
             &mOCLPBridgeDriverBinding,
             ImageHandle,
             &gOCLPBridgeComponentName,
             &gOCLPBridgeComponentName2,
             NULL,
             NULL,
             NULL,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OCLPBridge: Failed to install DB/CN/CN2 protocols - %r\n", Status));
    //
    // DB/CN/CN2 are not critical, so we can continue even without them.
    //
  }

  Existing = OcGetProtocol (&gOcInterfaceProtocolGuid, 0, NULL, NULL);
  if (Existing == NULL) {
    DEBUG ((DEBUG_ERROR, "OCLPBridge: Failed to get existing OC interface protocol\n"));
    return EFI_NOT_FOUND;
  }

  DataSize = sizeof (mOcVersion) - 1;
  Status   = gRT->GetVariable (
                    OC_VERSION_VARIABLE_NAME,
                    &gOcVendorVariableGuid,
                    0,
                    &DataSize,
                    mOcVersion
                    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OCLPBridge: Failed to get OC version - %r\n", Status));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "Found OC version %a\n", mOcVersion));

  DataSize = sizeof (mOCLPVersion) - 1;
  Status   = gRT->GetVariable (
                    L"OCLP-Version",
                    &gOcVendorVariableGuid,
                    0,
                    &DataSize,
                    mOCLPVersion
                    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OCLPBridge: Failed to get OCLP version - %r\n", Status));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "Found OCLP version %a\n", mOCLPVersion));

  Status = OcAsciiSafeSPrint (
             mTitleSuffix,
             sizeof (mTitleSuffix),
             "OCLP %a with OC %a",
             mOCLPVersion,
             mOcVersion
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OCLPBridge: Failed to create version string - %r\n", Status));
    return Status;
  }

  mExistingPopulate         = Existing->PopulateContext;
  Existing->PopulateContext = OverrideOcPickerContext;

  return EFI_SUCCESS;
}
