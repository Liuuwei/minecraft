#pragma once

#include <d3d12.h>
#include <d3dx12.h>
#include <map>
#include <string>
#include <wrl/client.h>

#include "DXSampleHelper.h"

class DescriptorHeap {
public:
	DescriptorHeap(const Microsoft::WRL::ComPtr<ID3D12Device2>& device) : device_(device) {}

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap() const { return heap_; }
	Microsoft::WRL::ComPtr<ID3D12Resource> buffer(const std::string& name) { return buffers_[name]; }
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(const std::string& name) { return cpuHandles_[name]; }
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(const std::string& name) { return gpuHandles_[name]; }

protected:
	Microsoft::WRL::ComPtr<ID3D12Device2> device_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap_ = nullptr;

	std::map<std::string, Microsoft::WRL::ComPtr<ID3D12Resource>> buffers_;
	std::map<std::string, CD3DX12_CPU_DESCRIPTOR_HANDLE> cpuHandles_;
	std::map<std::string, CD3DX12_GPU_DESCRIPTOR_HANDLE> gpuHandles_;

	UINT descriptorSize_ = 0;
};
