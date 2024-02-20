#pragma once

#include <dxgi1_4.h>

#include "DescriptorHeap.h"

class RTVHeap : public DescriptorHeap {
public:
	RTVHeap(const Microsoft::WRL::ComPtr<ID3D12Device2>& device, UINT frameCount);

	void createRenderTargetView(IDXGISwapChain3* swapChain);
	Microsoft::WRL::ComPtr<ID3D12Resource> renderTarget(int index) const { return renderTargets_[index]; }
private:
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> renderTargets_;

	int frameCount_;
};
