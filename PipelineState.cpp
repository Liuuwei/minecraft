#include "PipelineState.h"

#include "Shader.h"

#include <d3dx12.h>

D3D12_GRAPHICS_PIPELINE_STATE_DESC PipelineState::getDefaultDesc() {
	CD3DX12_RASTERIZER_DESC rsState(D3D12_DEFAULT);
	
	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	desc.SampleMask = UINT_MAX;
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.NumRenderTargets = 1;
	desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.RasterizerState = rsState;

	return desc;
}

PipelineState::PipelineState(ID3D12Device* device, const D3D12_GRAPHICS_PIPELINE_STATE_DESC desc) {
	ThrowIfFailed(device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pso_)));
}

PipelineState::PipelineState(ID3D12Device* device, ID3D12RootSignature* rootSignature, std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout, Shader* shader) {
	auto desc = PipelineState::getDefaultDesc();
	desc.pRootSignature = rootSignature;
	desc.VS = shader->vertex();
	if (shader->hasPixel()) {
		desc.PS = shader->pixel();
	}
	if (shader->hasGeometry()) {
		desc.GS = shader->geometry();
	}
	desc.InputLayout = D3D12_INPUT_LAYOUT_DESC{inputLayout.data(), static_cast<UINT>(inputLayout.size())};

	ThrowIfFailed(device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pso_)));
}

PipelineState::PipelineState(ID3D12Device* device, ID3D12RootSignature* rootSignature, std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout, Shader* shader, D3D12_GRAPHICS_PIPELINE_STATE_DESC desc) {
	desc.pRootSignature = rootSignature;
	desc.VS = shader->vertex();
	if (shader->hasPixel()) {
		desc.PS = shader->pixel();
	}
	if (shader->hasGeometry()) {
		desc.GS = shader->geometry();
	}
	desc.InputLayout = D3D12_INPUT_LAYOUT_DESC{inputLayout.data(), static_cast<UINT>(inputLayout.size())};

	ThrowIfFailed(device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pso_)));
}