#pragma once

#include "DescriptorHeap.h"

class CSUHeap : public DescriptorHeap {
public:
	CSUHeap(const Microsoft::WRL::ComPtr<ID3D12Device2>& device, UINT csvCount, UINT srcCount, UINT uavCount);

	void createConstantBuffer(const std::string& name, UINT size);
	void createConstantBuffer(const std::string& name, D3D12_HEAP_PROPERTIES* heapProperties, D3D12_HEAP_FLAGS heapFlags, D3D12_RESOURCE_DESC* resourceDesc, D3D12_RESOURCE_STATES states, D3D12_CLEAR_VALUE* clearValue = nullptr);
	void createConstantBufferView(const std::string& name, UINT size);
	void createShaderResourceBuffer(const std::string& name, const std::string& filename, ID3D12CommandQueue* commandQueue);
	void createShaderResourceView(const std::string& name, D3D12_SHADER_RESOURCE_VIEW_DESC desc);

	void upload(const std::string& name, void* data, UINT size);
private:
	std::map<std::string, UINT8*> data_;

	int csvIndex_;
	int srcIndex_;
	int uavIndex_;
};
