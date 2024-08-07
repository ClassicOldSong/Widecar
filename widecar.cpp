#include <thread>
#include <chrono>
#include <vector>
#include <string>

#include <cstring>
#include <cstdio>
#include <cstdlib>

#include <windows.h>
#include <conio.h>
#include <tlhelp32.h>
#include "parsec-vdd.h"

#pragma comment(lib, "advapi32.lib")

using namespace std::chrono_literals;
using namespace parsec_vdd;

std::string get_env_as_string(const char* env) {
	auto* vp = ::getenv(env);
	if (vp) {
		printf("%s: %s\n", env, vp);
		return { vp };
	} else {
		printf("env not found: %s\n", env);
		exit(0);
	}
}

void write_reg_dword(HKEY root, LPCSTR path, LPCSTR key, DWORD val) {
	HKEY hKey;
	LONG result;

	result = RegOpenKeyExA(root, path, 0, KEY_ALL_ACCESS, &hKey);
	printf("RegOpenKeyExA: %lx\n", result);
	if (result) {
		result = RegCreateKeyExA(root, path, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, NULL);
	}
	if (result) {
		printf("RegCreateKeyExA: %lx\n", result);
		exit(2);
	}
	result = RegSetValueExA(hKey, key, 0, REG_DWORD, (const BYTE*)&val, sizeof(val));
	printf("RegSetValueExA: %lx\n", result);
	RegCloseKey(hKey);
}

std::string get_registry_path(DWORD parent_id, const std::string& app_id) {
	return "SOFTWARE\\Widecar\\sessions\\" + std::to_string(parent_id) + "_" + app_id;
}

bool check_registry_entry_exists(HKEY root, LPCSTR registry_path, LPCSTR key, DWORD* display_id) {
	HKEY hKey;
	LONG result = RegOpenKeyExA(
		root,
		registry_path,
		0,
		KEY_READ,
		&hKey
	);

	if (result == ERROR_SUCCESS) {
		DWORD data_size = sizeof(DWORD);
		DWORD type = REG_DWORD;
		result = RegQueryValueExA(
			hKey,
			key,
			NULL,
			&type,
			reinterpret_cast<LPBYTE>(display_id),
			&data_size
		);

		RegCloseKey(hKey);

		if (result == ERROR_SUCCESS && type == REG_DWORD) {
			return true;
		}
	}

	return false;
}

bool write_to_registry(HKEY root, LPCSTR registry_path, LPCSTR key, DWORD display_id) {
	write_reg_dword(root, registry_path, key, display_id);
	return true;
}

bool remove_registry(HKEY root, LPCSTR registry_path) {
	LONG result = RegDeleteKeyExA(
		root,
		registry_path,
		KEY_WOW64_64KEY,
		0
	);

	if (result != ERROR_SUCCESS) {
		printf("Failed to delete registry key. Error code: %ld\n", result);
		return false;
	}

	return true;
}

DWORD get_parent_process_id(DWORD process_id) {
	DWORD parent_process_id = -1;
	HANDLE h_snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (h_snapshot != INVALID_HANDLE_VALUE) {
		PROCESSENTRY32 pe32;
		pe32.dwSize = sizeof(PROCESSENTRY32);
		if (Process32First(h_snapshot, &pe32)) {
			do {
				if (pe32.th32ProcessID == process_id) {
					parent_process_id = pe32.th32ParentProcessID;
					break;
				}
			} while (Process32Next(h_snapshot, &pe32));
		}
		CloseHandle(h_snapshot);
	}
	return parent_process_id;
}

void write_custom_res_reg(int slot, int width, int height, int fps) {
	std::string reg_path("SOFTWARE\\Parsec\\vdd\\");
	reg_path.push_back(((char)slot) + 0x30);

	write_reg_dword(HKEY_LOCAL_MACHINE, reg_path.c_str(), "height", height);
	write_reg_dword(HKEY_LOCAL_MACHINE, reg_path.c_str(), "width", width);
	write_reg_dword(HKEY_LOCAL_MACHINE, reg_path.c_str(), "hz", fps);
}

int extract_integer_from_device_key(const std::string& device_key) {
	size_t pos = device_key.rfind('\\');
	if (pos != std::string::npos) {
		try {
			return std::stoi(device_key.substr(pos + 1));
		} catch (...) {
		}
	}
	return -1; // Invalid integer
}

DEVMODEA get_device_settings(const DISPLAY_DEVICEA& display_device) {
	DEVMODEA dev_mode = {0};
	dev_mode.dmSize = sizeof(dev_mode);

	int sleep_interval = 20;
	LONG result;

	while (sleep_interval < 1000) {
		result = EnumDisplaySettingsA(display_device.DeviceName, ENUM_CURRENT_SETTINGS, &dev_mode);
		if (!result) {
			printf("Error getting dev mode for %s: %d, retrying in %dms...\n", display_device.DeviceName, result, sleep_interval);
			Sleep(sleep_interval);
			sleep_interval *= 2;
		} else {
			return dev_mode;
		}
	}
	printf("Could not get dev mode for %s: %d, exiting...\n", display_device.DeviceName, result);
	exit(0);
	return dev_mode;
}

int query_displays(const std::string& target_device_id, std::vector<std::pair<DISPLAY_DEVICEA, DEVMODEA>>& active_displays) {
	DISPLAY_DEVICEA display_device;
	display_device.cb = sizeof(DISPLAY_DEVICEA);
	int device_index = 0;
	int match_count = 0;

	while (EnumDisplayDevicesA(NULL, device_index, &display_device, 0)) {
		std::string device_key(display_device.DeviceKey);
		std::string device_id(display_device.DeviceID);

		if (
			(display_device.StateFlags & DISPLAY_DEVICE_ACTIVE)
			&& device_id.find(target_device_id) != std::string::npos
		) {
			active_displays.emplace_back(display_device, get_device_settings(display_device));
			match_count++;
		}
		device_index++;
	}

	return match_count;
}

DISPLAY_DEVICEA find_display(int target_device_key_int, const std::string& target_device_id) {
	DISPLAY_DEVICEA display_device = {0};
	display_device.cb = sizeof(DISPLAY_DEVICEA);
	int device_index = 0;

	while (EnumDisplayDevicesA(NULL, device_index, &display_device, 0)) {
		device_index++;

		std::string device_key(display_device.DeviceKey);
		std::string device_id(display_device.DeviceID);
		int device_key_int = extract_integer_from_device_key(device_key);

		if (device_key_int == target_device_key_int && device_id.find(target_device_id) != std::string::npos) {
			return display_device;
		}
	}

	printf("No matching device found.\n");
	display_device.cb = 0; // Mark as invalid
	return display_device;
}

bool change_display_settings(DISPLAY_DEVICEA& display_device, int width, int height, int refresh_rate) {
	DEVMODEA dev_mode = {0};
	dev_mode.dmSize = sizeof(dev_mode);

	if (EnumDisplaySettingsA(display_device.DeviceName, ENUM_CURRENT_SETTINGS, &dev_mode)) {
		dev_mode.dmPelsWidth = width;
		dev_mode.dmPelsHeight = height;
		dev_mode.dmDisplayFrequency = refresh_rate;
		dev_mode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;

		LONG result = ChangeDisplaySettingsExA(display_device.DeviceName, &dev_mode, NULL, CDS_UPDATEREGISTRY, NULL);
		if (result == DISP_CHANGE_SUCCESSFUL) {
			return true;
		} else {
			printf("Failed to change display settings for %s: %ld\n", display_device.DeviceName, result);
		}
	} else {
		printf("Failed to enumerate display settings for %s.\n", display_device.DeviceName);
	}
	return false;
}

bool set_primary_display(DISPLAY_DEVICEA& primary_device) {
	DEVMODEA primary_dev_mode = get_device_settings(primary_device);

	int offset_x = primary_dev_mode.dmPosition.x;
	int offset_y = primary_dev_mode.dmPosition.y;

	printf("OFFSET: X: %d, Y: %d\n", offset_x, offset_y);

	LONG result;

	DISPLAY_DEVICEA display_device;
	display_device.cb = sizeof(DISPLAY_DEVICEA);
	int device_index = 0;

	while (EnumDisplayDevicesA(NULL, device_index, &display_device, 0)) {
		device_index++;
		if (!(display_device.StateFlags & DISPLAY_DEVICE_ACTIVE)) {
			continue;
		}

		DEVMODEA dev_mode = get_device_settings(display_device);

		dev_mode.dmPosition.x -= offset_x;
		dev_mode.dmPosition.y -= offset_y;
		dev_mode.dmFields = DM_POSITION;

		result = ChangeDisplaySettingsExA(display_device.DeviceName, &dev_mode, NULL, CDS_UPDATEREGISTRY | CDS_NORESET, NULL);
		if (result != DISP_CHANGE_SUCCESSFUL) {
			printf("Failed to adjust display settings for device %s: %ld\n", display_device.DeviceName, result);
		}
	}

	// Update primary device's config to ensure it's primary
	primary_dev_mode.dmPosition.x = 0;
	primary_dev_mode.dmPosition.y = 0;
	primary_dev_mode.dmFields = DM_POSITION;
	ChangeDisplaySettingsExA(primary_device.DeviceName, &primary_dev_mode, NULL, CDS_UPDATEREGISTRY | CDS_NORESET | CDS_SET_PRIMARY, NULL);

	result = ChangeDisplaySettingsExA(NULL, NULL, NULL, 0, NULL);
	if (result != DISP_CHANGE_SUCCESSFUL) {
		printf("Failed to apply display settings: %ld\n", result);
		return false;
	}

	return true;
}

int WINAPI WinMain(
	HINSTANCE   hInstance,
	HINSTANCE   hPrevInstance,
	LPSTR       lpCmdLine,
	int         nCmdShow
) {
// int main() {
	DeviceStatus status = QueryDeviceStatus(&VDD_CLASS_GUID, VDD_HARDWARE_ID);
	if (status != DEVICE_OK) {
		printf("Parsec VDD device is not OK, got status %d.\n", status);
		return 0;
	}

	HANDLE vdd = OpenDeviceHandle(&VDD_ADAPTER_GUID);
	if (vdd == NULL || vdd == INVALID_HANDLE_VALUE) {
		printf("Failed to obtain the device handle.\n");
		return 0;
	}

	auto s_client_width = get_env_as_string("SUNSHINE_CLIENT_WIDTH");
	auto s_client_height = get_env_as_string("SUNSHINE_CLIENT_HEIGHT");
	auto s_client_fps = get_env_as_string("SUNSHINE_CLIENT_FPS");
	auto s_app_id = get_env_as_string("SUNSHINE_APP_ID");

	auto parent_id = get_parent_process_id(GetCurrentProcessId());
	auto sc_width = (DWORD)strtol(s_client_width.c_str(), NULL, 10);
	auto sc_height = (DWORD)strtol(s_client_height.c_str(), NULL, 10);
	auto sc_fps = (DWORD)strtol(s_client_fps.c_str(), NULL, 10);

	printf("ParentID: %d\n", parent_id);

	std::string target_device_id = "Root\\Parsec\\VDA";

	// Collect DEVMODE from all currently active displays
	std::vector<std::pair<DISPLAY_DEVICEA, DEVMODEA>> active_displays;
	query_displays(target_device_id, active_displays);

	printf("Active vdisplay count: %zd\n", active_displays.size());

	DWORD display_id;
	std::string registry_path = get_registry_path(parent_id, s_app_id);
	LPCSTR reg_path = registry_path.c_str();

	if (check_registry_entry_exists(HKEY_LOCAL_MACHINE, reg_path, "displayId", &display_id)) {
		remove_registry(HKEY_LOCAL_MACHINE, reg_path);
		VddRemoveDisplay(vdd, display_id);

		// Apply all saved display settings
		for (auto& display_pair : active_displays) {
			auto& display = display_pair.first;
			auto& dev_mode = display_pair.second;
			int device_key_int = extract_integer_from_device_key(display.DeviceKey);
			if (device_key_int != display_id) {
				printf("Resetting for %s, w: %d, h: %d, fps: %d\n", display.DeviceName, dev_mode.dmPelsWidth, dev_mode.dmPelsHeight, dev_mode.dmDisplayFrequency);
				change_display_settings(display, dev_mode.dmPelsWidth, dev_mode.dmPelsHeight, dev_mode.dmDisplayFrequency);
			} else {
				printf("Removed display %s for APP: %s\n", display.DeviceName, s_app_id.c_str());
			}
		}
	} else {
		write_custom_res_reg(4, sc_width, sc_height, sc_fps);

		display_id = VddAddDisplay(vdd);
		write_reg_dword(HKEY_LOCAL_MACHINE, reg_path, "displayId", display_id);

		// Get the corresponding DISPLAY_DEVICEA with display_id
		DISPLAY_DEVICEA new_display = find_display(display_id, target_device_id);

		if (new_display.cb != 0) {
			// Modify the devmode with sc_width, sc_height, and sc_fps
			DEVMODEA new_dev_mode = get_device_settings(new_display);
			new_dev_mode.dmPelsWidth = sc_width;
			new_dev_mode.dmPelsHeight = sc_height;
			new_dev_mode.dmDisplayFrequency = sc_fps;

			// Insert the new display and its devmode into the collected active displays
			active_displays.emplace_back(new_display, new_dev_mode);

			// Apply all saved display settings
			for (auto& display_pair : active_displays) {
				auto& dev_mode = display_pair.second;
				printf("Resetting for %s, w: %d, h: %d, fps: %d\n", display_pair.first.DeviceName, dev_mode.dmPelsWidth, dev_mode.dmPelsHeight, dev_mode.dmDisplayFrequency);
				change_display_settings(display_pair.first, dev_mode.dmPelsWidth, dev_mode.dmPelsHeight, dev_mode.dmDisplayFrequency);
			}

			if (set_primary_display(new_display)) {
				printf("Primary display set successfully.\n");
			}

			printf("Added display %s for APP: %s\n", new_display.DeviceName, s_app_id.c_str());
		} else {
			printf("Display not found for id: %d!", display_id);
		}

	}

	return 0;
}
