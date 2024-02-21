#pragma once
#include "DescriptorHeap.h"

class DSVHeap : public my::DescriptorHeap {
public:
	DSVHeap(const Microsoft::WRL::ComPtr<ID3D12Device2>& device);

	void createDepthStencilBuffer(D3D12_HEAP_PROPERTIES* heapProperties, D3D12_HEAP_FLAGS heapFlags, D3D12_RESOURCE_DESC* resourceDesc, D3D12_RESOURCE_STATES states, D3D12_CLEAR_VALUE* clearValue = nullptr);
	void createDepthStencilView();
private:
	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilBuffer_;
};
