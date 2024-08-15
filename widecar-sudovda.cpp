#include <windows.h>
#include <iostream>
#include <vector>
#include <setupapi.h>
#include <initguid.h>
#include <combaseapi.h>
#include <thread>
#include "sudovda.h"
// #include "AdapterOption.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "ole32.lib")

// {dff7fd29-5b75-41d1-9731-b32a17a17104}
static const GUID DEFAULT_DISPLAY_GUID = { 0xdff7fd29, 0x5b75, 0x41d1, { 0x97, 0x31, 0xb3, 0x2a, 0x17, 0xa1, 0x71, 0x04 } };

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
	std::setlocale(LC_ALL, "");

	auto s_client_uid = std::getenv("SUNSHINE_CLIENT_UID");
	auto s_client_name = std::getenv("SUNSHINE_CLIENT_NAME");
	auto s_client_width = std::getenv("SUNSHINE_CLIENT_WIDTH");
	auto s_client_height = std::getenv("SUNSHINE_CLIENT_HEIGHT");
	auto s_client_fps = std::getenv("SUNSHINE_CLIENT_FPS");
	auto s_app_id = std::getenv("SUNSHINE_APP_ID");
	auto s_app_name = std::getenv("SUNSHINE_APP_NAME");

	UINT width = 1920;
	UINT height = 1080;
	UINT fps = 60;

	if (s_client_width && s_client_height && s_client_fps) {
		auto sc_width = strtol(s_client_width, NULL, 10);
		auto sc_height = strtol(s_client_height, NULL, 10);
		auto sc_fps = strtol(s_client_fps, NULL, 10);

		if (sc_width && sc_height && sc_fps) {
			width = sc_width;
			height = sc_height;
			fps = sc_fps;
		}
	// } else {
	// 	return 0;
	}

	if (!s_client_uid) {
		printf("CLIENT UID NOT PROVIDED!\n");
	} else {
		printf("Client UID: %s\n", s_client_uid);
	}

	if (!s_app_name || !strlen(s_app_name) || !strcmp(s_app_name, "unknown")) {
		s_app_name = "SudoMakerVDD";
	}

	if (!s_client_name || !strlen(s_client_name) || !strcmp(s_client_name, "unknown")) {
		s_client_name = s_app_name;
	}

	HANDLE hDevice = OpenDevice(&SUVDA_INTERFACE_GUID);
	if (hDevice == INVALID_HANDLE_VALUE) {
		printf("Failed to open device.\n");
		return 1;
	}

	if (!CheckProtocolCompatible(hDevice)) {
		printf("SUDOVDA protocol not compatible!\n");
		return 1;
	}

	// auto availableGpus = getAvailableGPUs();

	// for (auto it: availableGpus) {
	// 	std::cout << "GPU -----------------" << std::endl;
	// 	std::wcout << L"Name: " << it.name << std::endl;
	// 	std::cout << "VMEM: " << it.desc.DedicatedVideoMemory << std::endl;
	// 	std::cout << std::endl;
	// }

	// auto options = AdapterOption();

	// // LUID selectedGPU = options.selectGPU(L"Intel(R) UHD Graphics 630");
	// LUID selectedGPU = options.selectGPU(L"???");

	// std::cout << "!!!!!!" << std::endl;
	// std::wcout << L"Selected GPU: " << options.target_name << std::endl;

	// SetRenderAdapter(hDevice, selectedGPU);

	GUID guid;
	if (s_client_uid && strcmp(s_client_uid, "unknown")) {
		size_t len = strlen(s_client_uid);
		if (len > sizeof(GUID)) {
			len = sizeof(GUID);
		}
		memcpy((void*)&guid, (void*)s_client_uid, len);
	} else {
		guid = DEFAULT_DISPLAY_GUID;
		s_client_uid = "unknown";
	}

	if (RemoveVirtualDisplay(hDevice, guid)) {
		printf("Virtual display removed successfully.\n");
		CloseHandle(hDevice);
		return 0;
	} else {
		printf("Failed to remove virtual display. Creating one instead.\n");
	}

	VIRTUAL_DISPLAY_ADD_OUT output;
	if (!AddVirtualDisplay(hDevice, width, height, fps, guid, s_client_name, s_client_uid, output)) {
		printf("Failed to add virtual display.\n");
		return 0;
	}

	// Start watchdog

	// VIRTUAL_DISPLAY_GET_WATCHDOG_OUT watchdogOut;
	// if (GetWatchdogTimeout(hDevice, watchdogOut)) {
	// 	printf("Watchdog: Timeout %d, Countdown %d\n", watchdogOut.Timeout, watchdogOut.Countdown);
	// }


	// if (watchdogOut.Timeout) {
	// 	auto sleepInterval = watchdogOut.Timeout * 1000 / 2;
	// 	std::thread ping_thread([&hDevice, sleepInterval]{
	// 		for (;;) {
	// 			if (!sleepInterval) return;
	// 			PingDriver(hDevice);
	// 			Sleep(sleepInterval);
	// 		}
	// 	});

	// 	ping_thread.detach();
	// }

	uint32_t retryInterval = 20;
	wchar_t deviceName[CCHDEVICENAME];
	while (!GetAddedDisplayName(output, deviceName)) {
		Sleep(retryInterval);
		if (retryInterval > 160) {
			printf("Cannot get name for newly added virtual display!\n");
			return 0;
		}
		retryInterval *= 2;
	}

	wprintf(L"Virtual display added successfully: %ws\n", deviceName);
	printf("Configuration: W: %d, H: %d, FPS: %d\n", width, height, fps);
	printf("AppID: %s\n", s_app_id);

	changeDisplaySettings(deviceName, width, height, fps);
	setPrimaryDisplay(deviceName);

	// system("pause");

	CloseHandle(hDevice);
	return 0;
}
