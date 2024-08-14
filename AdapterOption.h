#pragma once

#include <wrl/client.h> 	// For ComPtr
#include <dxgi.h>       	// For IDXGIAdapter, IDXGIFactory1
#include <algorithm>    	// For sort
#include <string>

using namespace std;
using namespace Microsoft::WRL;

#pragma comment(lib, "dxgi.lib")

// Structure to vector gpus
struct GPUInfo {
    wstring name;                // GPU name
    ComPtr<IDXGIAdapter> adapter;// COM pointer to the adapter
    DXGI_ADAPTER_DESC desc;      // Adapter description
};

// Sort function for GPUs by dedicated video memory
bool CompareGPUs(const GPUInfo& a, const GPUInfo& b) {
    return a.desc.DedicatedVideoMemory > b.desc.DedicatedVideoMemory;
}

// Get a enumerate list of available GPUs
vector<GPUInfo> getAvailableGPUs() {
    vector<GPUInfo> gpus; // Vector to hold all GPU's information

    ComPtr<IDXGIFactory1> factory;
    if (!SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
        return gpus;
    }

    // Enumerate all adapters (GPUs)
    for (UINT i = 0;; i++) {
        ComPtr<IDXGIAdapter> adapter;
        if (!SUCCEEDED(factory->EnumAdapters(i, &adapter))) {
            break;
        }

        DXGI_ADAPTER_DESC desc;

        if (!SUCCEEDED(adapter->GetDesc(&desc))) {
            continue;
        }

        // Add the adapter information to the list
        GPUInfo info{ desc.Description, adapter, desc };
        gpus.push_back(info);
    }

    return gpus;
}

class AdapterOption {
public:
    bool hasTargetAdapter{}; // Indicates if a target adapter is selected
    LUID adapterLuid{};      // Adapter's unique identifier (LUID)
    wstring target_name{};   // Target adapter name
    std::vector<GPUInfo> availableGPUs;

    AdapterOption() {
        availableGPUs = getAvailableGPUs();
    }

    // Select the best GPU based on dedicated video memory
    void selectBestGPU() {
        if (availableGPUs.empty()) {
            hasTargetAdapter = false;
            adapterLuid = {};
            // target_name = {};
            return;
        }

        // Sort GPUs by dedicated video memory in descending order
        sort(availableGPUs.begin(), availableGPUs.end(), CompareGPUs);
        auto bestGPU = availableGPUs.front(); // Get the GPU with the most memory

        target_name = bestGPU.name;
        adapterLuid = bestGPU.desc.AdapterLuid;
        hasTargetAdapter = true;
    }

    // Set the target adapter from a given name and validate it
    LUID selectGPU(const wstring& adapterName) {
        if (!findAndSetAdapter(adapterName)) {
            // If the adapter is not found, select the best GPU and retry
            selectBestGPU();
        }

        return adapterLuid;
    }

private:
    // Find and set the adapter by its name
    bool findAndSetAdapter(const wstring& adapterName) {
        target_name = adapterName;
        // Iterate through all available GPUs
        for (const auto& gpu : availableGPUs) {
            if (_wcsicmp(gpu.name.c_str(), adapterName.c_str()) == 0) {
                adapterLuid = gpu.desc.AdapterLuid; // Set the adapter LUID
                hasTargetAdapter = true; // Indicate that a target adapter is selected
                return true;
            }
        }

        adapterLuid = {};
        hasTargetAdapter = false; // Indicate that no target adapter is selected
        return false;
    }
};
