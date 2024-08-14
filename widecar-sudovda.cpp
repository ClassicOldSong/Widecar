#include <windows.h>
#include <iostream>
#include <vector>
#include <setupapi.h>
#include <initguid.h>
#include <combaseapi.h>
#include <thread>
#include "sudovda.h"
#include "AdapterOption.h"

using namespace SUDOVDA;

LONG getDeviceSettings(const wchar_t* deviceName, DEVMODEW& devMode) {
	devMode.dmSize = sizeof(DEVMODEW);
	return EnumDisplaySettingsW(deviceName, ENUM_CURRENT_SETTINGS, &devMode);
}

LONG changeDisplaySettings(const wchar_t* deviceName, int width, int height, int refresh_rate) {
	DEVMODEW devMode = {0};
	devMode.dmSize = sizeof(devMode);

	if (EnumDisplaySettingsW(deviceName, ENUM_CURRENT_SETTINGS, &devMode)) {
		devMode.dmPelsWidth = width;
		devMode.dmPelsHeight = height;
		devMode.dmDisplayFrequency = refresh_rate;
		devMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;

		return ChangeDisplaySettingsExW(deviceName, &devMode, NULL, CDS_UPDATEREGISTRY, NULL);
	}

	return 0;
}

bool setPrimaryDisplay(const wchar_t* primaryDeviceName) {
	DEVMODEW primaryDevMode{};
	if (!getDeviceSettings(primaryDeviceName, primaryDevMode)) {
		return false;
	};

	int offset_x = primaryDevMode.dmPosition.x;
	int offset_y = primaryDevMode.dmPosition.y;

	LONG result;

	DISPLAY_DEVICEW displayDevice;
	displayDevice.cb = sizeof(DISPLAY_DEVICEA);
	int device_index = 0;

	while (EnumDisplayDevicesW(NULL, device_index, &displayDevice, 0)) {
		device_index++;
		if (!(displayDevice.StateFlags & DISPLAY_DEVICE_ACTIVE)) {
			continue;
		}

		DEVMODEW devMode{};
		if (getDeviceSettings(displayDevice.DeviceName, devMode)) {
			devMode.dmPosition.x -= offset_x;
			devMode.dmPosition.y -= offset_y;
			devMode.dmFields = DM_POSITION;

			result = ChangeDisplaySettingsExW(displayDevice.DeviceName, &devMode, NULL, CDS_UPDATEREGISTRY | CDS_NORESET, NULL);
			if (result != DISP_CHANGE_SUCCESSFUL) {
				// wprintf(L"Failed to adjust display settings for device %ws: %ld\n", displayDevice.DeviceName, result);
				return false;
			}
		}
	}

	// Update primary device's config to ensure it's primary
	primaryDevMode.dmPosition.x = 0;
	primaryDevMode.dmPosition.y = 0;
	primaryDevMode.dmFields = DM_POSITION;
	ChangeDisplaySettingsExW(primaryDeviceName, &primaryDevMode, NULL, CDS_UPDATEREGISTRY | CDS_NORESET | CDS_SET_PRIMARY, NULL);

	result = ChangeDisplaySettingsExW(NULL, NULL, NULL, 0, NULL);
	if (result != DISP_CHANGE_SUCCESSFUL) {
		// printf("Failed to apply display settings: %ld\n", result);
		return false;
	}

	return true;
}

int main() {
	std::locale::global(std::locale(""));

	auto availableGpus = getAvailableGPUs();

	for (auto it: availableGpus) {
		std::cout << "GPU -----------------" << std::endl;
		std::wcout << L"Name: " << it.name << std::endl;
		std::cout << "VMEM: " << it.desc.DedicatedVideoMemory << std::endl;
		std::cout << std::endl;
	}

	auto options = AdapterOption();

	// LUID selectedGPU = options.selectGPU(L"Intel(R) UHD Graphics 630");
	LUID selectedGPU = options.selectGPU(L"???");

	std::cout << "!!!!!!" << std::endl;
	std::wcout << L"Selected GPU: " << options.target_name << std::endl;

	HANDLE hDevice = OpenDevice(&SUVDA_INTERFACE_GUID);
	if (hDevice == INVALID_HANDLE_VALUE) {
		std::cerr << "Failed to open device." << std::endl;
		return 1;
	}

	std::thread ping_thread([hDevice]{
		for (;;) {
			PingDriver(hDevice);
			Sleep(1000);
		}
	});

	ping_thread.detach();

	VIRTUAL_DISPLAY_GET_PROTOCOL_VERSION_OUT protocolVersion;
	if (GetProtocolVersion(hDevice, protocolVersion)) {
		printf("Get version: %d.%d.%d\n", protocolVersion.Version.Major, protocolVersion.Version.Minor, protocolVersion.Version.Incremental);
		printf("Local version: %d.%d.%d\n", VDAProtocolVersion.Major, VDAProtocolVersion.Minor, VDAProtocolVersion.Incremental);
		printf("Version match: %d\n\n", isProtocolCompatible(protocolVersion.Version));
	}

	SetRenderAdapter(hDevice, selectedGPU);

	GUID guid;
	if (FAILED(CoCreateGuid(&guid))) {
		std::cerr << "Failed to create GUID." << std::endl;
		CloseHandle(hDevice);
		return 1;
	}

	char sn[14];
	wchar_t szGUID[64] = {0};
	StringFromGUID2(guid, szGUID, 64);
	snprintf(sn, 13, "%ws", szGUID + 1);

	VIRTUAL_DISPLAY_ADD_OUT output;
	if (AddVirtualDisplay(hDevice, 3020, 2320, 120, guid, "MyVdisp1", sn, output)) {
		std::cout << "Virtual display added successfully. Target ID: " << output.TargetId << std::endl;
		std::wcout << L"GUID: " << szGUID << std::endl;
		std::cout << "LUID: " << output.AdapterLuid.HighPart << "-" << output.AdapterLuid.LowPart << std::endl;
	} else {
		std::cerr << "Failed to add virtual display." << std::endl;
		return 0;
	}

	wchar_t deviceName[CCHDEVICENAME];
	if (GetAddedDisplayName(output, deviceName)) {
		wprintf(L"Added display name: %ws\n", deviceName);
	}

	changeDisplaySettings(deviceName, 3020, 2320, 120);
	setPrimaryDisplay(deviceName);

	system("pause");

	VIRTUAL_DISPLAY_GET_WATCHDOG_OUT watchdogOut;
	if (GetWatchdogTimeout(hDevice, watchdogOut)) {
		printf("Watchdog: Timeout %d, Countdown %d\n", watchdogOut.Timeout, watchdogOut.Countdown);
	}

	if (RemoveVirtualDisplay(hDevice, guid)) {
		std::cout << "Virtual display removed successfully." << std::endl;
	} else {
		std::cerr << "Failed to remove virtual display." << std::endl;
	}

	CloseHandle(hDevice);
	return 0;
}
