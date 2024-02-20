#include "RTVHeap.h"

RTVHeap::RTVHeap(const Microsoft::WRL::ComPtr<ID3D12Device2>& device, UINT frameCount) : DescriptorHeap(device), renderTargets_(frameCount), frameCount_(frameCount) {
	descriptorSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.NumDescriptors = frameCount;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	ThrowIfFailed(device_->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap_)));
}

void RTVHeap::createRenderTargetView(IDXGISwapChain3* swapChain) {
	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(heap_->GetCPUDescriptorHandleForHeapStart());
	
	for (int i = 0; i < frameCount_; i++) {
		ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets_[i])));
		device_->CreateRenderTargetView(renderTargets_[i].Get(), nullptr, handle);
		handle.Offset(1, descriptorSize_);
	}
}
