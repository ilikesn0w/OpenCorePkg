/** @file
  OCLP bridge driver.

  Copyright (c) 2026, ilikesn0w. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "OCLPBridgePrivate.h"

//
// EFI CN1.
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_COMPONENT_NAME_PROTOCOL  gOCLPBridgeComponentName = {
  OCLPBridgeComponentNameGetDriverName,
  OCLPBridgeComponentNameGetControllerName,
  "eng"
};

//
// EFI CN2.
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_COMPONENT_NAME2_PROTOCOL  gOCLPBridgeComponentName2 = {
  (EFI_COMPONENT_NAME2_GET_DRIVER_NAME)OCLPBridgeComponentNameGetDriverName,
  (EFI_COMPONENT_NAME2_GET_CONTROLLER_NAME)OCLPBridgeComponentNameGetControllerName,
  "en"
};

GLOBAL_REMOVE_IF_UNREFERENCED EFI_UNICODE_STRING_TABLE  mOCLPBridgeNameTable[] = {
  {
    "eng;en",
    L"OCLP bridge platform driver"
  },
  {
    NULL,
    NULL
  }
};

EFI_STATUS
EFIAPI
OCLPBridgeComponentNameGetDriverName (
  IN EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                       *Language,
  OUT CHAR16                      **DriverName
  )
{
  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           mOCLPBridgeNameTable,
           DriverName,
           (BOOLEAN)(This == &gOCLPBridgeComponentName)
           );
}

EFI_STATUS
EFIAPI
OCLPBridgeComponentNameGetControllerName (
  IN EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  EFI_HANDLE                  ControllerHandle,
  IN  EFI_HANDLE                  ChildHandle        OPTIONAL,
  IN  CHAR8                       *Language,
  OUT CHAR16                      **ControllerName
  )
{
  return EFI_UNSUPPORTED;
}
