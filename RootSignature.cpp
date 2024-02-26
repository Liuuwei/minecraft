#include "RootSignature.h"

#include <MyDirectx/MySampleHelp.h>

ID3D12Device2* my::RootSignature::device_ = nullptr;
D3D12_FEATURE_DATA_ROOT_SIGNATURE my::RootSignature::featureData_ = D3D12_FEATURE_DATA_ROOT_SIGNATURE{};

my::RootSignature::RootSignature(int numParameters, std::vector<CD3DX12_ROOT_PARAMETER1> parameters, D3D12_ROOT_SIGNATURE_FLAGS flag) {
	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init_1_1(numParameters, parameters.data(),  0, nullptr, flag);

	Microsoft::WRL::ComPtr<ID3DBlob> signature;
	Microsoft::WRL::ComPtr<ID3DBlob> error;
	ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData_.HighestVersion, &signature, &error));
	ThrowIfFailed(device_->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature_)));
}


my::RootSignature::RootSignature(int numParameters, std::vector<CD3DX12_ROOT_PARAMETER1> parameters, int numSamplers, std::vector<D3D12_STATIC_SAMPLER_DESC> samplerDesc, D3D12_ROOT_SIGNATURE_FLAGS flag) {
	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init_1_1(numParameters, parameters.data(),  numSamplers, samplerDesc.data(), flag);

	Microsoft::WRL::ComPtr<ID3DBlob> signature;
	Microsoft::WRL::ComPtr<ID3DBlob> error;
	ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData_.HighestVersion, &signature, &error));
	ThrowIfFailed(device_->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature_)));
}
