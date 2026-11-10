/** @file
  OCLP bridge driver.

  Copyright (c) 2026, ilikesn0w. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef OCLP_BRIDGE_PRIVATE_H
#define OCLP_BRIDGE_PRIVATE_H

#include <Uefi.h>

#include <Library/UefiLib.h>

#include <Protocol/ComponentName.h>
#include <Protocol/ComponentName2.h>

EFI_STATUS
EFIAPI
OCLPBridgeComponentNameGetDriverName (
  IN EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                       *Language,
  OUT CHAR16                      **DriverName
  );

EFI_STATUS
EFIAPI
OCLPBridgeComponentNameGetControllerName (
  IN EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  EFI_HANDLE                  ControllerHandle,
  IN  EFI_HANDLE                  ChildHandle        OPTIONAL,
  IN  CHAR8                       *Language,
  OUT CHAR16                      **ControllerName
  );

extern EFI_COMPONENT_NAME2_PROTOCOL  gOCLPBridgeComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gOCLPBridgeComponentName2;

#endif // OCLP_BRIDGE_PRIVATE_H
