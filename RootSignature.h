#pragma once
#include <d3d12.h>
#include <d3dx12.h>

namespace my
{


class RootSignature {
public:
	static void init(ID3D12Device2* device, D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData) {
		device_ = device;
		featureData_ = featureData;
	}

	RootSignature(int numParameters, std::vector<CD3DX12_ROOT_PARAMETER1> parameters, D3D12_ROOT_SIGNATURE_FLAGS flag = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	RootSignature(int numParameters, std::vector<CD3DX12_ROOT_PARAMETER1> parameters, int numSamplers, std::vector<D3D12_STATIC_SAMPLER_DESC> samplerDesc, D3D12_ROOT_SIGNATURE_FLAGS flag = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	
	ID3D12RootSignature* rootSignature() const { return rootSignature_.Get(); }
private:
	static ID3D12Device2* device_;
	static D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData_;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;                                      
};


}
