#pragma once

#include <setupapi.h>
#include <cfgmgr32.h>
#include <stdio.h>
#include "sudovda-ioctl.h"

#ifdef _MSC_VER
#pragma comment(lib, "cfgmgr32.lib")
#pragma comment(lib, "setupapi.lib")
#endif

#ifdef __cplusplus
namespace SUDOVDA
{
#endif

static const HANDLE OpenDevice(const GUID* interfaceGuid) {
	// Get the device information set for the specified interface GUID
	HDEVINFO deviceInfoSet = SetupDiGetClassDevs(interfaceGuid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (deviceInfoSet == INVALID_HANDLE_VALUE) {
		return INVALID_HANDLE_VALUE;
	}

	HANDLE handle = INVALID_HANDLE_VALUE;

	SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
	ZeroMemory(&deviceInterfaceData, sizeof(SP_DEVICE_INTERFACE_DATA));
	deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	for (DWORD i = 0; SetupDiEnumDeviceInterfaces(deviceInfoSet, nullptr, interfaceGuid, i, &deviceInterfaceData); ++i)
	{
		DWORD detailSize = 0;
		SetupDiGetDeviceInterfaceDetailA(deviceInfoSet, &deviceInterfaceData, NULL, 0, &detailSize, NULL);

		SP_DEVICE_INTERFACE_DETAIL_DATA_A *detail = (SP_DEVICE_INTERFACE_DETAIL_DATA_A *)calloc(1, detailSize);
		detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);

		if (SetupDiGetDeviceInterfaceDetailA(deviceInfoSet, &deviceInterfaceData, detail, detailSize, &detailSize, NULL))
		{
			handle = CreateFileA(detail->DevicePath,
				GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				NULL,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED | FILE_FLAG_WRITE_THROUGH,
				NULL);

			if (handle != NULL && handle != INVALID_HANDLE_VALUE) {
				break;
			}
		}

		free(detail);
	}

	SetupDiDestroyDeviceInfoList(deviceInfoSet);
	return handle;
}

static const bool isProtocolCompatible(const SUVDA_PROTOCAL_VERSION& otherVersion) {
	// Changes to existing ioctl must be marked as major
	if (VDAProtocolVersion.Major != otherVersion.Major) {
		return false;
	}

	// We shouldn't break compatibility with minor/incremental changes
	// e.g. add new ioctl
	if (VDAProtocolVersion.Minor < otherVersion.Minor) {
		return false;
	}

	if (
		VDAProtocolVersion.Minor == otherVersion.Minor &&
		VDAProtocolVersion.Incremental < otherVersion.Incremental
	) {
		return false;
	}

	return true;
};

bool AddVirtualDisplay(HANDLE hDevice, UINT Width, UINT Height, UINT RefreshRate, const GUID& MonitorGuid, const CHAR* DeviceName, const CHAR* SerialNumber, VIRTUAL_DISPLAY_ADD_OUT& output) {
	VIRTUAL_DISPLAY_ADD_PARAMS params{Width, Height, RefreshRate, MonitorGuid};
	strcpy_s(params.DeviceName, DeviceName);
	strcpy_s(params.SerialNumber, SerialNumber);

	DWORD bytesReturned;
	BOOL success = DeviceIoControl(
		hDevice,
		IOCTL_ADD_VIRTUAL_DISPLAY,
		(LPVOID)&params,
		sizeof(params),
		(LPVOID)&output,
		sizeof(output),
		&bytesReturned,
		nullptr
	);

	if (!success) {
		std::cerr << "AddVirtualDisplay failed: " << GetLastError() << std::endl;
	}

	return success;
}

bool RemoveVirtualDisplay(HANDLE hDevice, const GUID& MonitorGuid) {
	VIRTUAL_DISPLAY_REMOVE_PARAMS params{MonitorGuid};
	DWORD bytesReturned;
	BOOL success = DeviceIoControl(
		hDevice,
		IOCTL_REMOVE_VIRTUAL_DISPLAY,
		(LPVOID)&params,
		sizeof(params),
		nullptr,
		0,
		&bytesReturned,
		nullptr
	);

	if (!success) {
		std::cerr << "RemoveVirtualDisplay failed: " << GetLastError() << std::endl;
	}

	return success;
}

bool SetRenderAdapter(HANDLE hDevice, const LUID& AdapterLuid) {
	VIRTUAL_DISPLAY_SET_RENDER_ADAPTER_PARAMS params{AdapterLuid};
	DWORD bytesReturned;
	BOOL success = DeviceIoControl(
		hDevice,
		IOCTL_SET_RENDER_ADAPTER,
		(LPVOID)&params,
		sizeof(params),
		nullptr,
		0,
		&bytesReturned,
		nullptr
	);

	if (!success) {
		std::cerr << "SetRenderAdapter failed: " << GetLastError() << std::endl;
	}

	return success;
}

bool GetWatchdogTimeout(HANDLE hDevice, VIRTUAL_DISPLAY_GET_WATCHDOG_OUT& output) {
	DWORD bytesReturned;
	BOOL success = DeviceIoControl(
		hDevice,
		IOCTL_GET_WATCHDOG,
		nullptr,
		0,
		(LPVOID)&output,
		sizeof(output),
		&bytesReturned,
		nullptr
	);

	if (!success) {
		std::cerr << "DeviceIoControl failed: " << GetLastError() << std::endl;
	}

	return success;
}

bool GetProtocolVersion(HANDLE hDevice, VIRTUAL_DISPLAY_GET_PROTOCOL_VERSION_OUT& output) {
	DWORD bytesReturned;
	BOOL success = DeviceIoControl(
		hDevice,
		IOCTL_GET_PROTOCOL_VERSION,
		nullptr,
		0,
		(LPVOID)&output,
		sizeof(output),
		&bytesReturned,
		nullptr
	);

	if (!success) {
		std::cerr << "DeviceIoControl failed: " << GetLastError() << std::endl;
	}

	return success;
}

bool PingDriver(HANDLE hDevice) {
	DWORD bytesReturned;
	BOOL success = DeviceIoControl(
		hDevice,
		IOCTL_DRIVER_PING,
		nullptr,
		0,
		nullptr,
		0,
		&bytesReturned,
		nullptr
	);

	if (!success) {
		std::cerr << "DeviceIoControl failed: " << GetLastError() << std::endl;
	}

	return success;
}

bool GetAddedDisplayName(const VIRTUAL_DISPLAY_ADD_OUT& addedDisplay, wchar_t* deviceName) {
	// get all paths
	UINT pathCount;
	UINT modeCount;
	if (GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &pathCount, &modeCount))
		return 0;

	std::vector<DISPLAYCONFIG_PATH_INFO> paths(pathCount);
	std::vector<DISPLAYCONFIG_MODE_INFO> modes(modeCount);
	if (QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &pathCount, paths.data(), &modeCount, modes.data(), nullptr))
		return 0;

	auto path = std::find_if(paths.begin(), paths.end(), [&addedDisplay](DISPLAYCONFIG_PATH_INFO _path) {
		return _path.targetInfo.id == addedDisplay.TargetId;
	});

	DISPLAYCONFIG_SOURCE_DEVICE_NAME sourceName = {};
	sourceName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
	sourceName.header.size = sizeof(DISPLAYCONFIG_SOURCE_DEVICE_NAME);
	sourceName.header.adapterId = addedDisplay.AdapterLuid;
	sourceName.header.id = path->sourceInfo.id;
	if (DisplayConfigGetDeviceInfo((DISPLAYCONFIG_DEVICE_INFO_HEADER*)&sourceName)) {
		return false;
	}

	wcscpy_s(deviceName, CCHDEVICENAME, sourceName.viewGdiDeviceName);

	return true;
}

#ifdef __cplusplus
} // namespace SUDOVDA
#endif