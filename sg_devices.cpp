/*
SG_Devices
Get information whenever a new media / devie is inserted to the PC

The following source code is proprietary of Secured Globe, Inc. and is used under
source code license given to this project only.
(c)2015-2022 Secured Globe, Inc.

*/
#include "stdafx.h"
#include "winmain.h"
#include "sg_devices.h"

// This GUID is for all USB serial host PnP drivers, but you can replace it 
// with any valid device class guid.
GUID WceusbshGUID = 
{ 
	0x25dbce51, 0x6c8f, 0x4a72,
	0x8a,0x6d,0xb5,0x4c,0x2b,0x4f,0xc8,0x35 
};

BOOL DoRegisterDeviceInterfaceToHwnd(
	IN GUID InterfaceClassGuid,
	IN HWND hWnd,
	OUT HDEVNOTIFY* hDeviceNotify
)
// Routine Description:
//     Registers an HWND for notification of changes in the device interfaces
//     for the specified interface class GUID. 

// Parameters:
//     InterfaceClassGuid - The interface class GUID for the device 
//         interfaces. 

//     hWnd - Window handle to receive notifications.

//     hDeviceNotify - Receives the device notification handle. On failure, 
//         this value is NULL.

// Return Value:
//     If the function succeeds, the return value is TRUE.
//     If the function fails, the return value is FALSE.

// Note:
//     RegisterDeviceNotification also allows a service handle be used,
//     so a similar wrapper function to this one supporting that scenario
//     could be made from this template.
{
	DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;

	ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));
	NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
	NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	NotificationFilter.dbcc_classguid = InterfaceClassGuid;

	*hDeviceNotify = RegisterDeviceNotification(
		hWnd,                       // events recipient
		&NotificationFilter,        // type of device
		DEVICE_NOTIFY_WINDOW_HANDLE // type of recipient handle
	);

	if (NULL == *hDeviceNotify)
	{
		ShowStatus(L"RegisterDeviceNotification");
		return FALSE;
	}

	return TRUE;
}

inline int getDriveLetter(ULONG maskFromMessage)
{
	char i;

	for (i = 0; i < 26; ++i) 
	{
		if (maskFromMessage & 0x1)
			break;
		maskFromMessage = maskFromMessage >> 1;
	}
	return i;
}

DWORD GetPhysicalDriveSerialNumber(UINT nDriveNumber, CString& strSerialNumber)
{
	DWORD dwResult = NO_ERROR;
	strSerialNumber.Empty();

	WCHAR strDrivePath[256];
	swprintf(strDrivePath, 255, L"\\\\.\\PhysicalDrive%d", nDriveNumber);

	// call CreateFile to get a handle to physical drive
	HANDLE hDevice = ::CreateFile(strDrivePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, 0, NULL);

	if (INVALID_HANDLE_VALUE == hDevice)
		return ::GetLastError();

	// set the input STORAGE_PROPERTY_QUERY data structure
	STORAGE_PROPERTY_QUERY storagePropertyQuery;
	ZeroMemory(&storagePropertyQuery, sizeof(STORAGE_PROPERTY_QUERY));
	storagePropertyQuery.PropertyId = StorageDeviceProperty;
	storagePropertyQuery.QueryType = PropertyStandardQuery;

	// get the necessary output buffer size
	STORAGE_DESCRIPTOR_HEADER storageDescriptorHeader = { 0 };
	DWORD dwBytesReturned = 0;
	if (!::DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY,
		&storagePropertyQuery, sizeof(STORAGE_PROPERTY_QUERY),
		&storageDescriptorHeader, sizeof(STORAGE_DESCRIPTOR_HEADER),
		&dwBytesReturned, NULL))
	{
		dwResult = ::GetLastError();
		::CloseHandle(hDevice);
		return dwResult;
	}

	// allocate the necessary memory for the output buffer
	const DWORD dwOutBufferSize = storageDescriptorHeader.Size;
	BYTE* pOutBuffer = new BYTE[dwOutBufferSize];
	ZeroMemory(pOutBuffer, dwOutBufferSize);

	// get the storage device descriptor
	if (!::DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY,
		&storagePropertyQuery, sizeof(STORAGE_PROPERTY_QUERY),
		pOutBuffer, dwOutBufferSize,
		&dwBytesReturned, NULL))
	{
		dwResult = ::GetLastError();
		delete[]pOutBuffer;
		::CloseHandle(hDevice);
		return dwResult;
	}

	// Now, the output buffer points to a STORAGE_DEVICE_DESCRIPTOR structure
	// followed by additional info like vendor ID, product ID, serial number, and so on.
	STORAGE_DEVICE_DESCRIPTOR* pDeviceDescriptor = (STORAGE_DEVICE_DESCRIPTOR*)pOutBuffer;
	const DWORD dwSerialNumberOffset = pDeviceDescriptor->SerialNumberOffset;
	if (dwSerialNumberOffset != 0)
	{
		// finally, get the serial number
		strSerialNumber = CString(pOutBuffer + dwSerialNumberOffset);
	}

	// perform cleanup and return
	delete[]pOutBuffer;
	::CloseHandle(hDevice);
	return dwResult;
}

CString DeviceChangeToDriveLetter(WPARAM wParam, LPARAM lParam)
{
	CString result = L"", DriveSerialNumber = L"";
	PDEV_BROADCAST_HDR lpdb = (PDEV_BROADCAST_HDR)lParam;

	switch (wParam)
	{
		case DBT_DEVICEARRIVAL:
			// Check whether a CD or DVD was inserted into a drive.
			if (lpdb->dbch_devicetype == DBT_DEVTYP_VOLUME)
			{
				PDEV_BROADCAST_VOLUME lpdbv = (PDEV_BROADCAST_VOLUME)lpdb;
				int driveletter = getDriveLetter(lpdbv->dbcv_unitmask);
				GetPhysicalDriveSerialNumber(driveletter, DriveSerialNumber);
				result.Format(L"%c:", (WCHAR)driveletter + L'A');
				return result;
			}
			break;

		case DBT_DEVICEREMOVECOMPLETE:
			// Check whether a CD or DVD was removed from a drive.
			if (lpdb->dbch_devicetype == DBT_DEVTYP_VOLUME)
			{
				PDEV_BROADCAST_VOLUME lpdbv = (PDEV_BROADCAST_VOLUME)lpdb;
				int driveletter = getDriveLetter(lpdbv->dbcv_unitmask);
				result.Format(L"-%c", (WCHAR)driveletter + L'A');
				return result;
			}
			break;

		default:
			return ((CString)"");
			break;
	}
	return (CString)L"";
}

