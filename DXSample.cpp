#include "DXSample.h"

#include "Win32Application.h"

#include <dxgi1_6.h>

using namespace Microsoft::WRL;

DXSample::DXSample(UINT width, UINT height, std::wstring name) : width_(width), height_(height), title_(std::move(name)) {
	WCHAR assetsPath[512];
	getAssetsPath(assetsPath, _countof(assetsPath));
	assetsPath_ = assetsPath;

	aspectRatio_ = static_cast<float>(width_ )/ static_cast<float>(height_);
}

DXSample::~DXSample() {
	
}

std::wstring DXSample::getAssetFullPath(LPCWSTR assetName) const {
	return assetsPath_ + assetName;
}

void DXSample::getHardwareAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** pAdapter, bool requestHighPerformanceAdapter) {
	*pAdapter = nullptr;

	ComPtr<IDXGIAdapter1> adapter;
	ComPtr<IDXGIFactory6> factory6;
	if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6)))) {
		for (UINT adapterIndex = 0; 
			SUCCEEDED(factory6->EnumAdapterByGpuPreference(adapterIndex, requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED, IID_PPV_ARGS(&adapter))); 
			++adapterIndex) {
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
				continue;
			}

			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr))) {
				break;
			}
		}
	}

	if (adapter.Get() == nullptr) {
		for (UINT adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex) {
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
				continue;
			}

			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr))) {
				break;
			}
		}
	}
	*pAdapter = adapter.Detach();
}

void DXSample::setCustomWindowText(LPCWSTR text) {
	std::wstring windowText = title_ + L": " + text;
	SetWindowText(Win32Application::getHwnd(), windowText.c_str());
}

void DXSample::parseCommandLineArgs(WCHAR* argv[], int argc) {
	for (int i = 1; i < argc; i++) {
		if (_wcsnicmp(argv[i], L"-warp", wcslen(argv[i])) == 0 || 
			_wcsnicmp(argv[i], L"/warp", wcslen(argv[i])) == 0) {
			title_ = title_ + L"(WARP)";
		}
	}
}

