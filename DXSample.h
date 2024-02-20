#pragma once

#include "DXSampleHelper.h"

#include <dxgi.h>
#include <DirectXMath.h>
#include <d3dx12.h>
#include <d3d12.h>
#include <dxgi1_4.h>

class DXSample {
public:
	DXSample(UINT width, UINT height, std::wstring name);
	virtual ~DXSample();

	virtual void onInit() = 0;
	virtual void onUpdate(WPARAM wParam, LPARAM lParam) = 0;
	virtual void onRender() = 0;
	virtual void onDestroy() = 0;

	virtual void onKeyDown(WPARAM param) {}
	virtual void onKeyUp(WPARAM param) {}

	virtual void onMouseMovement(LPARAM param) {}

	virtual void onRightButtonDown() {}
	virtual void onLeftButtonDown() {}

	UINT getWidth() const { return width_; }
	UINT getHeight() const { return height_; }
	const WCHAR* getTitle() const { return title_.c_str(); }

	void parseCommandLineArgs(WCHAR* argv[], int argc);

	HWND hwnd_;

protected:
	std::wstring getAssetFullPath(LPCWSTR assetName) const;
	void getHardwareAdapter(IDXGIFactory1* factory, IDXGIAdapter1** adapter, bool requestHighPerformanceAdapter = false);
	void setCustomWindowText(LPCWSTR text);

	UINT width_;
	UINT height_;
	float aspectRatio_;

private:
	std::wstring assetsPath_;
	std::wstring title_;
};