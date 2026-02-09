/*++ @file

 Created by HermitCrabs on 26/4/2015.
 Copyright 2010-2014 The HermitCrab Labs. All rights reserved.

 Module Name:

 AppleGraphicsPolicy.h

 Abstract:

 Revision History

 1.0 Initial Version

 --*/

#ifndef APPLE_GRAPHICS_POLICY_H
#define APPLE_GRAPHICS_POLICY_H

#include <Uefi.h>

// APPLE_GRAPHICS_POLICY_PROTOCOL_REVISION
#define APPLE_GRAPHICS_POLICY_PROTOCOL_REVISION  0x00020000

// APPLE_GRAPHICS_POLICY_PROTOCOL_GUID
#define APPLE_GRAPHICS_POLICY_PROTOCOL_GUID  \
  { 0xA4BB4654, 0x9F72, 0x4BC8, \
    { 0x93, 0xEB, 0x65, 0x9F, 0xD8, 0x70, 0x8B, 0x10 } }

typedef struct APPLE_GRAPHICS_POLICY_PROTOCOL APPLE_GRAPHICS_POLICY_PROTOCOL;

/**
  Initialize the graphics controller.

  @param[in]  This      Pointer to the protocol instance.
  @param[in]  Context   Optional context information.

  @retval EFI_SUCCESS           Controller initialized successfully.
  @retval EFI_INVALID_PARAMETER Context is invalid.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_GRAPHICS_POLICY_CONNECT_CONTROLLER)(
  IN APPLE_GRAPHICS_POLICY_PROTOCOL  *This,
  IN VOID                            *Context
  );

/**
  Match a GPU by Vendor and Device ID.

  @param[in]  This             Pointer to the protocol instance.
  @param[in]  VendorId         PCI Vendor ID of the GPU.
  @param[in]  DeviceId         PCI Device ID of the GPU.
  @param[in]  ControllerHandle Handle of the controller to match.

  @retval EFI_SUCCESS           Device matched successfully.
  @retval EFI_NOT_FOUND         No matching device found.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_GRAPHICS_POLICY_MATCH_DEVICE)(
  IN APPLE_GRAPHICS_POLICY_PROTOCOL  *This,
  IN UINT16                          VendorId,
  IN UINT16                          DeviceId,
  IN EFI_HANDLE                      ControllerHandle
  );

/**
  Apply the current graphics policy.

  @param[in]  This  Pointer to the protocol instance.

  @retval EFI_SUCCESS  Policy applied successfully.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_GRAPHICS_POLICY_APPLY_POLICY)(
  IN APPLE_GRAPHICS_POLICY_PROTOCOL  *This
  );

/**
  Query a policy property.

  @param[in]      This         Pointer to the protocol instance.
  @param[in]      PropertyGuid GUID of the property to query.
  @param[out]     Buffer       Buffer to store the property value.
  @param[in,out]  BufferSize   On input: size of Buffer; on output: size of data written.

  @retval EFI_SUCCESS           Property read successfully.
  @retval EFI_BUFFER_TOO_SMALL  BufferSize too small.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_GRAPHICS_POLICY_GET_PROPERTY)(
  IN APPLE_GRAPHICS_POLICY_PROTOCOL  *This,
  IN CONST EFI_GUID                  *PropertyGuid,
  OUT VOID                           *Buffer,
  IN OUT UINTN                       *BufferSize
  );

/**
  Inject a property into a device path.

  @param[in]  This          Pointer to the protocol instance.
  @param[in]  DevicePath    Device path to inject property into.
  @param[in]  PropertyName  Name of the property.
  @param[in]  SubProperty   Sub-property name (optional).
  @param[in]  Data          Pointer to the property data.
  @param[in]  DataSize      Size of the property data in bytes.

  @retval EFI_SUCCESS            Property injected successfully.
  @retval EFI_OUT_OF_RESOURCES   Failed due to lack of memory.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_GRAPHICS_POLICY_SET_DEVICE_PROPERTY)(
  IN APPLE_GRAPHICS_POLICY_PROTOCOL  *This,
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN CONST CHAR16                    *PropertyName,
  IN CONST CHAR16                    *SubProperty,
  IN CONST VOID                      *Data,
  IN UINTN                           DataSize
  );

/**
  Finalize the protocol, freeing resources.

  @param[in]  This  Pointer to the protocol instance.

  @retval EFI_SUCCESS  Finalized successfully.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_GRAPHICS_POLICY_FINALIZE)(
  IN APPLE_GRAPHICS_POLICY_PROTOCOL  *This
  );

/**
  Generic control function for extended operations.

  @param[in]      This       Pointer to the protocol instance.
  @param[in]      Function   Function code.
  @param[in,out]  Buffer     Input/output buffer.
  @param[in,out]  BufferSize Size of the buffer.

  @retval EFI_SUCCESS  Function executed successfully.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_GRAPHICS_POLICY_CONTROL)(
  IN APPLE_GRAPHICS_POLICY_PROTOCOL  *This,
  IN UINT32                          Function,
  IN OUT VOID                        *Buffer,
  IN OUT UINTN                       *BufferSize
  );

struct APPLE_GRAPHICS_POLICY_PROTOCOL {
  UINT32                                       Revision;
  APPLE_GRAPHICS_POLICY_CONNECT_CONTROLLER     ConnectController;
  APPLE_GRAPHICS_POLICY_MATCH_DEVICE           MatchDevice;
  APPLE_GRAPHICS_POLICY_APPLY_POLICY           ApplyPolicy;
  APPLE_GRAPHICS_POLICY_GET_PROPERTY           GetProperty;
  APPLE_GRAPHICS_POLICY_SET_DEVICE_PROPERTY    SetDeviceProperty;
  APPLE_GRAPHICS_POLICY_FINALIZE               Finalize;
  APPLE_GRAPHICS_POLICY_CONTROL                Control;
};

extern EFI_GUID  gAppleGraphicsPolicyProtocolGuid;

#endif // APPLE_GRAPHICS_POLICY_H
