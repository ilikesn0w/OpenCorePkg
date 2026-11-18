/** @file
  Apple Device Control protocol implementation.

  Copyright (c) 2026, ilikesn0w. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef OC_APPLE_DEVICE_CONTROL_LIB_H
#define OC_APPLE_DEVICE_CONTROL_LIB_H

#include <Protocol/AppleDeviceControl.h>

/**
  Install and initialise Apple Device Control protocol.

  @param[in] Reinstall  Overwrite installed protocol.

  @retval installed or located protocol or NULL.
**/
APPLE_DEVICE_CONTROL_PROTOCOL *
OcAppleDeviceControlInstallProtocol (
  IN BOOLEAN  Reinstall
  );

#endif // OC_APPLE_DEVICE_CONTROL_LIB_H
