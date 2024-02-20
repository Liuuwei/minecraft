#include "DSVHeap.h"

using namespace Microsoft::WRL;

DSVHeap::DSVHeap(const ComPtr<ID3D12Device2>& device) : DescriptorHeap(device) {
	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.NumDescriptors = 1;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

	ThrowIfFailed(device_->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap_)));
}

void DSVHeap::createDepthStencilBuffer(D3D12_HEAP_PROPERTIES* heapProperties, D3D12_HEAP_FLAGS heapFlags, D3D12_RESOURCE_DESC* resourceDesc, D3D12_RESOURCE_STATES states, D3D12_CLEAR_VALUE* clearValue) {
	ThrowIfFailed(device_->CreateCommittedResource(
		heapProperties, 
		heapFlags, 
		resourceDesc, 
		states, 
		clearValue, 
		IID_PPV_ARGS(&depthStencilBuffer_)));
}

void DSVHeap::createDepthStencilView() {
	D3D12_DEPTH_STENCIL_VIEW_DESC desc{};
	desc.Format = DXGI_FORMAT_D32_FLOAT;
	desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	desc.Flags = D3D12_DSV_FLAG_NONE;

	device_->CreateDepthStencilView(depthStencilBuffer_.Get(), &desc, heap_->GetCPUDescriptorHandleForHeapStart());
}
