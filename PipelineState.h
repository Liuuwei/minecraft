#pragma once
#include <d3d12.h>
#include <vector>
#include <wrl/client.h>

#include "DXSampleHelper.h"

class Shader;

class PipelineState {
public:
	static D3D12_GRAPHICS_PIPELINE_STATE_DESC getDefaultDesc();

	PipelineState(ID3D12Device* device, const D3D12_GRAPHICS_PIPELINE_STATE_DESC desc);

	PipelineState(ID3D12Device* device, ID3D12RootSignature* rootSignature, std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout, Shader* shader);
	PipelineState(ID3D12Device* device, ID3D12RootSignature* rootSignature, std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout, Shader* shader, D3D12_GRAPHICS_PIPELINE_STATE_DESC desc);

	ID3D12PipelineState* pipelineState() const { return pso_.Get(); }

private:
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pso_;
};
