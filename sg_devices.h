/*
SG_Devices
Get information whenever a new media / devie is inserted to the PC

The following source code is proprietary of Secured Globe, Inc. and is used under
source code license given to this project only.
(c)2015-2022 Secured Globe, Inc.

*/

#pragma once



BOOL DoRegisterDeviceInterfaceToHwnd(
	IN GUID InterfaceClassGuid,
	IN HWND hWnd,
	OUT HDEVNOTIFY* hDeviceNotify
);

DWORD GetPhysicalDriveSerialNumber(UINT nDriveNumber, CString& strSerialNumber);
CString DeviceChangeToDriveLetter(WPARAM wParam, LPARAM lParam);

