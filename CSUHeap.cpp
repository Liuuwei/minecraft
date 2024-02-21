#include "CSUHeap.h"

#include <d3dx12.h>
#include <directxtk12/DDSTextureLoader.h>
#include <directxtk12/ResourceUploadBatch.h>

#include "DXSampleHelper.h"
#include "Help.h"

using namespace DirectX;
using namespace Microsoft::WRL;

CSUHeap::CSUHeap(const ComPtr<ID3D12Device2>& device, UINT csvCount, UINT srcCount, UINT uavCount) :
	DescriptorHeap(device), 
	csvIndex_(0),
	srcIndex_(csvCount),
	uavIndex_(csvCount + srcCount) {
	descriptorSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.NumDescriptors = csvCount + srcCount + uavCount;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	ThrowIfFailed(device_->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap_)));
}

void CSUHeap::createConstantBuffer(const std::string& name, UINT size) {
	CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE;
	auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(Help::calculateConstantBufferSize(size));
	auto state = D3D12_RESOURCE_STATE_GENERIC_READ;
	createConstantBuffer(name, &heapProperties, heapFlags, &resourceDesc, state, nullptr);
}

void CSUHeap::createConstantBuffer(const std::string& name, D3D12_HEAP_PROPERTIES* heapProperties, D3D12_HEAP_FLAGS heapFlags, D3D12_RESOURCE_DESC* resourceDesc, D3D12_RESOURCE_STATES states, D3D12_CLEAR_VALUE* clearValue) {
	ThrowIfFailed(device_->CreateCommittedResource(
		heapProperties,
		heapFlags,
		resourceDesc,
		states,
		clearValue,
		IID_PPV_ARGS(&buffers_[name])));

	CD3DX12_RANGE range(0, 0);
	buffers_[name]->Map(0, &range, reinterpret_cast<void**>(&data_[name]));
}

void CSUHeap::createConstantBufferView(const std::string& name, UINT size) {
	createConstantBuffer(name, size);

	D3D12_CONSTANT_BUFFER_VIEW_DESC desc{};
	desc.BufferLocation = buffers_[name]->GetGPUVirtualAddress();
	desc.SizeInBytes = size;
	
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(heap_->GetCPUDescriptorHandleForHeapStart(), csvIndex_, descriptorSize_);
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(heap_->GetGPUDescriptorHandleForHeapStart(), csvIndex_, descriptorSize_);
	csvIndex_++;
	cpuHandles_[name] = cpuHandle;
	gpuHandles_[name] = gpuHandle;

	device_->CreateConstantBufferView(&desc, cpuHandle);
}

void CSUHeap::createShaderResourceBuffer(const std::string& name, const std::string& filename, ID3D12CommandQueue* commandQueue) {
	ResourceUploadBatch  upload(device_.Get());
	upload.Begin();

	ThrowIfFailed(CreateDDSTextureFromFile(
		device_.Get(),
		upload, 
		std::wstring(filename.begin(), filename.end()).c_str(), 
		buffers_[name].ReleaseAndGetAddressOf()));

	auto finish = upload.End(commandQueue);
	finish.wait();
}

void CSUHeap::createShaderResourceView(const std::string& name, const std::string& filename, D3D12_SHADER_RESOURCE_VIEW_DESC desc) {
	createShaderResourceBuffer(name, filename, commandQueue_);
	desc.Format = buffer(name)->GetDesc().Format;
	desc.TextureCube.MipLevels = buffer(name)->GetDesc().MipLevels;

	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(heap_->GetCPUDescriptorHandleForHeapStart(), srcIndex_, descriptorSize_);
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(heap_->GetGPUDescriptorHandleForHeapStart(), srcIndex_, descriptorSize_);
	srcIndex_++;
	cpuHandles_[name] = cpuHandle;
	gpuHandles_[name] = gpuHandle;
	
	device_->CreateShaderResourceView(buffers_[name].Get(), &desc, cpuHandle);
}


void CSUHeap::upload(const std::string& name, void* data, UINT size) {
	memcpy(data_[name], data, size);
}
