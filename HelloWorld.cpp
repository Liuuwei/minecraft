#include "HelloWorld.h"

#include <D3Dcompiler.h>
#include <queue>
#include <array>
#include <istream>
#include <directxtk12/DDSTextureLoader.h>
#include <directxtk12/ResourceUploadBatch.h>
#include <time.h>

#include "Help.h"
#include "Shader.h"
#include "Win32Application.h"
#include "Block.h"

HelloWorld::HelloWorld(UINT width, UINT height, std::wstring name) :
	DXSample(width, height, std::move(name)),
	viewport_(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
	scissorRect_(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)),
	rtvDescriptorSize_(0),
	frameIndex_(0),
	frameNumber_(0) {
}

void HelloWorld::onInit() {
	createFactory();
	createDevice();
	createCommandQueue();
	createCommandAllocator();
	createCommandList();
	createSwapChain();
	createRootSignature();
	createPipeline();
	createDescriptorHeap();
	createFence();

	createVertex();

	loadAssets();

	gameTimer_.tick();
	RECT rect;
	GetWindowRect(hwnd_, &rect);
	center_.x = (rect.left + rect.right) / 2;
	center_.y = (rect.top + rect.bottom) / 2;
	camera_.setCenter(center_.x, center_.y);
}

void HelloWorld::onUpdate(WPARAM wParam, LPARAM lParam) {
	gameTimer_.tick();	
	camera_.onMouseMovement(lParam);
	camera_.update(gameTimer_.deltaTime()); 

	auto model = XMMatrixTranslationFromVector(XMLoadFloat3(&XMFLOAT3(0, 0, 0)));
	auto view = camera_.getViewMatrix();
	auto proj = camera_.getProjectionMatrix(width_, height_, 0.1f, 1000.0f);

	XMFLOAT4X4 m, v, p;

	XMStoreFloat4x4(&m, XMMatrixTranspose(model));
	XMStoreFloat4x4(&v, XMMatrixTranspose(view));
	XMStoreFloat4x4(&p, XMMatrixTranspose(proj));

	csuHeap_->upload("model", &m, sizeof(m));
	csuHeap_->upload("view", &v, sizeof(v));
	csuHeap_->upload("projection", &p, sizeof(p));

	csuHeap_->upload("model_", &m, sizeof(m));
	csuHeap_->upload("view_", &v, sizeof(v));
	csuHeap_->upload("projection_", &p, sizeof(p));

	auto mvp = model * view * proj;
	XMFLOAT4X4 mvp_;
	XMStoreFloat4x4(&mvp_, XMMatrixTranspose(mvp));

	csuHeap_->upload("mvp", &mvp_, sizeof(mvp_));

	float time = 1e9;
	currBlock_ = bvh_->intersection(Ray(camera_.position(), camera_.front()), bvh_->root_, time);

	std::wstring s;

	Block::Face face(Block::NONE);
	if (currBlock_) {
		csuHeap_->upload("offset", &currBlock_->position(), sizeof(currBlock_->position()));
		face = currBlock_->selectedFace(camera_.position(), camera_.front());
	}

	switch (face) {
	case Block::FRONT:
		s += L"FRONT";
		break;
	case Block::BACK:
		s += L"BACK";
		break;
	case Block::UP:
		s += L"UP";
		break;
	case Block::DOWN:
		s += L"DOWN";
		break;
	case Block::RIGHT:
		s += L"RIGHT";
		break;
	case Block::LEFT:
		s += L"LEFT";
		break;
	case Block::NONE:
		s += L"NONE";
		break;
	}
	// SetWindowText(hwnd_, s.c_str());

	SetCursorPos(center_.x, center_.y);
}

void HelloWorld::onRender() {
	prepare();
	clear();

	populateCommandList();

	present();

	waitForPreviousFrame();

	static float time = 0.0f;
	time += gameTimer_.deltaTime();
	if (time >= 1.0f) {
		wchar_t msg[128];
		swprintf_s(msg, 128, L"fps: %d", static_cast<int>(1.0f / gameTimer_.deltaTime()));
		// SetWindowText(hwnd_, msg);
		time = 0.0f;
	}
}

void HelloWorld::onDestroy() {
	waitForPreviousFrame();

	CloseHandle(fenceEvent_);
}

void HelloWorld::onRightButtonDown() {
	if (currBlock_ == nullptr) {
		return ;
	}
	auto face = currBlock_->selectedFace(camera_.position(), camera_.front());

	XMFLOAT3 offset{};
	switch (face) {
	case Block::FRONT:
		offset = XMFLOAT3(0.0f, 0.0f, 1.0f);
		break;
	case Block::BACK:
		offset = XMFLOAT3(0.0f, 0.0f, -1.0f);
		break;
	case Block::RIGHT:
		offset = XMFLOAT3(1.0f, 0.0f, 0.0f);
		break;
	case Block::LEFT:
		offset = XMFLOAT3(-1.0f, 0.0f, 0.0f);
		break;
	case Block::UP:
		offset = XMFLOAT3(0.0f, 1.0f, 0.0f);
		break;
	case Block::DOWN:
		offset = XMFLOAT3(0.0f, -1.0f, 0.0f);
		break;
	}

	auto newBlock = new Block(*currBlock_, offset);
	blocks_.push_back(newBlock);
	vertices_.insert(vertices_.end(), newBlock->begin(), newBlock->end());

	CD3DX12_RANGE range(0, 0);
	ThrowIfFailed(device_->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), 
		D3D12_HEAP_FLAG_NONE, 
		&CD3DX12_RESOURCE_DESC::Buffer(vertices_.size() * sizeof(Block::Vertex)), 
		D3D12_RESOURCE_STATE_GENERIC_READ, 
		nullptr, 
		IID_PPV_ARGS(&blockBuffer_)));
	
	blockBuffer_->Map(0, &range, reinterpret_cast<void**>(&blockData_));
	memcpy(blockData_, vertices_.data(), vertices_.size() * sizeof(Block::Vertex));
	blockBuffer_->Unmap(0, nullptr);

	blockBufferView_.BufferLocation = blockBuffer_->GetGPUVirtualAddress();
	blockBufferView_.SizeInBytes = vertices_.size() * sizeof(Block::Vertex);
	blockBufferView_.StrideInBytes = sizeof(Block::Vertex);

	std::sort(blocks_.begin(), blocks_.end());
	auto start = std::chrono::system_clock::now();
	// bvh_->root_ = bvh_->buildBVH(blocks_);
	bvh_->root_ = bvh_->buildBVH(blocks_, 0, blocks_.size() - 1);
	auto end = std::chrono::system_clock::now();
	auto seconds = L"milliseconds: " + std::to_wstring(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
	SetWindowText(hwnd_, seconds.c_str());
}

void HelloWorld::onLeftButtonDown() {
	if (currBlock_ == nullptr) {
		return ;
	}

	auto result = std::remove_if(blocks_.begin(), blocks_.end(), [&](auto it) {
		auto position = it->position();
		if (position.x == currBlock_->position().x && position.y == currBlock_->position().y && position.z == currBlock_->position().z) {
			return true;
		}
		return false;
	});
	blocks_.erase(result, blocks_.end());

	currBlock_ = nullptr;
	vertices_.clear();

	for (auto& block : blocks_) {
		vertices_.insert(vertices_.end(), block->begin(), block->end());
	}

	if (vertices_.empty()) {
		return ;
	}

	CD3DX12_RANGE range(0, 0);
	ThrowIfFailed(device_->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), 
		D3D12_HEAP_FLAG_NONE, 
		&CD3DX12_RESOURCE_DESC::Buffer(vertices_.size() * sizeof(Block::Vertex)), 
		D3D12_RESOURCE_STATE_GENERIC_READ, 
		nullptr, 
		IID_PPV_ARGS(&blockBuffer_)));
	
	blockBuffer_->Map(0, &range, reinterpret_cast<void**>(&blockData_));
	memcpy(blockData_, vertices_.data(), vertices_.size() * sizeof(Block::Vertex));
	blockBuffer_->Unmap(0, nullptr);

	blockBufferView_.BufferLocation = blockBuffer_->GetGPUVirtualAddress();
	blockBufferView_.SizeInBytes = vertices_.size() * sizeof(Block::Vertex);
	blockBufferView_.StrideInBytes = sizeof(Block::Vertex);

	auto start = std::chrono::system_clock::now();
	bvh_->root_ = bvh_->buildBVH(blocks_, 0, blocks_.size() - 1);
	auto end = std::chrono::system_clock::now();
	auto seconds = L"milliseconds: " + std::to_wstring(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
	SetWindowText(hwnd_, seconds.c_str());
}

void HelloWorld::populateCommandList() {
	commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList_->IASetVertexBuffers(0, 1, &skyBufferView_);

	commandList_->SetGraphicsRootSignature(rootSignature_.Get());
	commandList_->SetPipelineState(skyboxPSO_->pipelineState());

	ID3D12DescriptorHeap* heaps[] = {csuHeap_->heap().Get()};
	commandList_->SetDescriptorHeaps(_countof(heaps), heaps);

	commandList_->SetGraphicsRootDescriptorTable(0, csuHeap_->gpuHandle("model"));
	commandList_->SetGraphicsRootDescriptorTable(1, csuHeap_->gpuHandle("skybox"));
	
	commandList_->DrawInstanced(36, 1, 0, 0);

	commandList_->SetPipelineState(defaultPSO_->pipelineState());
	commandList_->IASetVertexBuffers(0, 1, &blockBufferView_);
	commandList_->DrawInstanced(vertices_.size(), 1, 0, 0);

	commandList_->SetGraphicsRootSignature(rootSignature_.Get());
	commandList_->SetPipelineState(boxPSO_->pipelineState()); 
	commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	commandList_->SetGraphicsRootDescriptorTable(0, csuHeap_->gpuHandle("model_"));
	if (currBlock_ != nullptr)
		commandList_->DrawInstanced(32, 1, 0, 0);

	commandList_->SetPipelineState(minecraftPSO_->pipelineState());
	commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	commandList_->IASetVertexBuffers(0, 1, &crossHairBufferView_);
	commandList_->DrawInstanced(1, 1, 0, 0);
}

void HelloWorld::waitForPreviousFrame() {
	fenceValue_++;
	ThrowIfFailed(commandQueue_->Signal(fence_.Get(), fenceValue_));

	if (fence_->GetCompletedValue() < fenceValue_) {
		fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
		WaitForSingleObject(fenceEvent_, INFINITE);
	}

	frameIndex_ = swapChain_->GetCurrentBackBufferIndex();
}

void HelloWorld::createFactory() {
	UINT dxgiFactoryFlags = 0;
#if defined(_DEBUG)
	ComPtr<ID3D12Debug> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		debugController->EnableDebugLayer();
		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif

	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory_)));
}

void HelloWorld::createDevice() {
	ComPtr<IDXGIAdapter1> hardwareAdapter;
	getHardwareAdapter(factory_.Get(), &hardwareAdapter);

	ThrowIfFailed(D3D12CreateDevice(
		hardwareAdapter.Get(), 
		D3D_FEATURE_LEVEL_11_0, 
		IID_PPV_ARGS(&device_)));

	csuDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	rtvDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	graphicsMemory_ = std::make_unique<DirectX::GraphicsMemory>(device_.Get());
}

void HelloWorld::createCommandQueue() {
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	ThrowIfFailed(device_->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue_)));
}


void HelloWorld::createSwapChain() {
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = frameCount_;
	swapChainDesc.Width = width_;
	swapChainDesc.Height = height_;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> swapChain;
	ThrowIfFailed(factory_->CreateSwapChainForHwnd(
		commandQueue_.Get(), 
		Win32Application::getHwnd(), 
		&swapChainDesc, 
		nullptr, 
		nullptr, 
		&swapChain));

	ThrowIfFailed(swapChain.As(&swapChain_));
	frameIndex_ = swapChain_->GetCurrentBackBufferIndex();

	ThrowIfFailed(factory_->MakeWindowAssociation(Win32Application::getHwnd(), DXGI_MWA_NO_ALT_ENTER));
}

void HelloWorld::createCommandAllocator() {
	ThrowIfFailed(device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator_)));
}

void HelloWorld::createCommandList() {
	ThrowIfFailed(device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator_.Get(), nullptr, IID_PPV_ARGS(&commandList_)));
	ThrowIfFailed(commandList_->Close());
}

void HelloWorld::createRootSignature() {
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

	if (FAILED(device_->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData)))) {
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	CD3DX12_DESCRIPTOR_RANGE1 ranges[2];
	CD3DX12_ROOT_PARAMETER1 rootParameter[2];

	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 4, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	rootParameter[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);

	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	rootParameter[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);

	D3D12_STATIC_SAMPLER_DESC sampler = {};
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.MipLODBias = 0;
	sampler.MaxAnisotropy = 0;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;
	sampler.ShaderRegister = 0;
	sampler.RegisterSpace = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init_1_1(_countof(rootParameter), rootParameter,  1, &sampler, rootSignatureFlags);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
	ThrowIfFailed(device_->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature_)));
}

void HelloWorld::createPipeline() {
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	auto shader = Shader(L"defaultVertex.hlsl", L"defaultPixel.hlsl");
	auto defaultDesc = PipelineState::getDefaultDesc();
	defaultDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	defaultPSO_ = std::make_unique<PipelineState>(device_.Get(), rootSignature_.Get(), inputElementDescs, &shader, defaultDesc);

	auto skyBoxPSODesc = PipelineState::getDefaultDesc();
	skyBoxPSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	skyBoxPSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	shader = Shader(L"skyboxVertex.hlsl", L"skyboxPixel.hlsl");
	skyboxPSO_ = std::make_unique<PipelineState>(device_.Get(), rootSignature_.Get(), inputElementDescs, &shader, skyBoxPSODesc);

	std::vector<D3D12_INPUT_ELEMENT_DESC> input = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	auto desc = PipelineState::getDefaultDesc();
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	shader = Shader(L"mineVertex.hlsl", L"mineGeometry.hlsl", L"minePixel.hlsl");
	minecraftPSO_ = std::make_unique<PipelineState>(device_.Get(), rootSignature_.Get(), input, &shader, desc);

	desc = PipelineState::getDefaultDesc();
	desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	shader = Shader(L"boxVertex.hlsl", L"boxPixel.hlsl");
	boxPSO_ = std::make_unique<PipelineState>(device_.Get(), rootSignature_.Get(), input, &shader, desc);
}

void HelloWorld::createDescriptorHeap() {
	rtvHeap_ = std::make_unique<RTVHeap>(device_.Get(), frameCount_);
	rtvHeap_->createRenderTargetView(swapChain_.Get());

	csuHeap_ = std::make_unique<CSUHeap>(device_, 8, 2, 0);

	dsvHeap_ = std::make_unique<DSVHeap>(device_);
	dsvHeap_->createDepthStencilBuffer(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), 
		D3D12_HEAP_FLAG_NONE, 
		&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width_, height_, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE), 
		D3D12_RESOURCE_STATE_DEPTH_WRITE, 
		&CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1.0f, 0.0f));
	dsvHeap_->createDepthStencilView();
}

void HelloWorld::createFence() {
	ThrowIfFailed(device_->CreateFence(fenceValue_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)));
	fenceValue_++;

	fenceEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (fenceEvent_ == nullptr) {
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}
}

void HelloWorld::createVertex() {
	bvh_ = new BVH;

	skyBlock_ = new Block(XMFLOAT3(0.0, 0.0f, 0.0f));
	vertices_.insert(vertices_.end(), skyBlock_->begin(), skyBlock_->end());

	CD3DX12_RANGE range(0, 0);

	ThrowIfFailed(device_->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), 
		D3D12_HEAP_FLAG_NONE, 
		&CD3DX12_RESOURCE_DESC::Buffer(vertices_.size() * sizeof(Block::Vertex)), 
		D3D12_RESOURCE_STATE_GENERIC_READ, 
		nullptr, 
		IID_PPV_ARGS(&skyBuffer_)));
	
	skyBuffer_->Map(0, &range, reinterpret_cast<void**>(&skyData_));
	memcpy(skyData_, vertices_.data(), vertices_.size() * sizeof(Block::Vertex));
	skyBuffer_->Unmap(0, nullptr);

	skyBufferView_.BufferLocation = skyBuffer_->GetGPUVirtualAddress();
	skyBufferView_.SizeInBytes = vertices_.size() * sizeof(Block::Vertex);
	skyBufferView_.StrideInBytes = sizeof(Block::Vertex);

	vertices_.clear();

	for (int i = 1; i <= 100; i++) {
		for (int j = 1; j <= 100; j++) {
			auto offset = XMFLOAT3(i, 0, j);
			auto block = new Block(Block(), offset);
			blocks_.push_back(block);
			vertices_.insert(vertices_.end(), block->begin(), block->end());
		}
	}

	block = new Block(XMFLOAT3(0.0f, 0.0f, 0.0f));
	blocks_.push_back(block);
	// bvh_->root_ = bvh_->buildBVH(blocks_, 0, blocks_.size() - 1);
	std::sort(blocks_.begin(), blocks_.end());
	bvh_->root_ = bvh_->buildBVH(blocks_, 0, blocks_.size() - 1);

	// vertices_.clear();
	vertices_.insert(vertices_.end(), block->begin(), block->end());

	ThrowIfFailed(device_->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), 
		D3D12_HEAP_FLAG_NONE, 
		&CD3DX12_RESOURCE_DESC::Buffer(vertices_.size() * sizeof(Block::Vertex)), 
		D3D12_RESOURCE_STATE_GENERIC_READ, 
		nullptr, 
		IID_PPV_ARGS(&blockBuffer_)));
	
	blockBuffer_->Map(0, &range, reinterpret_cast<void**>(&blockData_));
	memcpy(blockData_, vertices_.data(), vertices_.size() * sizeof(Block::Vertex));
	blockBuffer_->Unmap(0, nullptr);

	blockBufferView_.BufferLocation = blockBuffer_->GetGPUVirtualAddress();
	blockBufferView_.SizeInBytes = vertices_.size() * sizeof(Block::Vertex);
	blockBufferView_.StrideInBytes = sizeof(Block::Vertex);

	struct Point {
		XMFLOAT3 pos;
	};
	std::array<Point, 1> cross = {
		XMFLOAT3(center_.x, center_.y, 0.0)
	};

	ThrowIfFailed(device_->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), 
		D3D12_HEAP_FLAG_NONE, 
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(cross)), 
		D3D12_RESOURCE_STATE_GENERIC_READ, 
		nullptr, 
		IID_PPV_ARGS(&crossHairBuffer_)));

	UINT8* crossData;
	crossHairBuffer_->Map(0, &range, reinterpret_cast<void**>(&crossData));
	memcpy(crossData, cross.data(), sizeof(cross));
	crossHairBuffer_->Unmap(0, nullptr);

	crossHairBufferView_.BufferLocation = crossHairBuffer_->GetGPUVirtualAddress();
	crossHairBufferView_.SizeInBytes = sizeof(cross);
	crossHairBufferView_.StrideInBytes = sizeof(XMFLOAT3);
}

void HelloWorld::loadAssets() {
	csuHeap_->createShaderResourceBuffer("skybox", "textures/snowcube1024.dds", commandQueue_.Get());
	csuHeap_->createShaderResourceBuffer("grass", "textures/planks.dds", commandQueue_.Get());

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = csuHeap_->buffer("skybox")->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MipLevels = csuHeap_->buffer("skybox")->GetDesc().MipLevels;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	csuHeap_->createShaderResourceView("skybox", srvDesc);

	srvDesc.Format = csuHeap_->buffer("grass")->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MipLevels = csuHeap_->buffer("grass")->GetDesc().MipLevels;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	csuHeap_->createShaderResourceView("grass", srvDesc);

	csuHeap_->createConstantBuffer(
		"model", 
		Help::calculateConstantBufferSize(sizeof(XMFLOAT4X4)));
	csuHeap_->createConstantBufferView("model", Help::calculateConstantBufferSize(sizeof(XMFLOAT4X4)));

	csuHeap_->createConstantBuffer(
		"view", 
		Help::calculateConstantBufferSize(sizeof(XMFLOAT4X4)));
	csuHeap_->createConstantBufferView("view", Help::calculateConstantBufferSize(sizeof(XMFLOAT4X4)));

	csuHeap_->createConstantBuffer(
		"projection", 
		Help::calculateConstantBufferSize(sizeof(XMFLOAT4X4)));
	csuHeap_->createConstantBufferView("projection", Help::calculateConstantBufferSize(sizeof(XMFLOAT4X4)));

	csuHeap_->createConstantBuffer(
		"mvp", 
		Help::calculateConstantBufferSize(sizeof(XMFLOAT4X4)));
	csuHeap_->createConstantBufferView("mvp", Help::calculateConstantBufferSize(sizeof(XMFLOAT4X4)));

	csuHeap_->createConstantBuffer(
		"model_", 
		Help::calculateConstantBufferSize(sizeof(XMFLOAT4X4)));
	csuHeap_->createConstantBufferView("model_", Help::calculateConstantBufferSize(sizeof(XMFLOAT4X4)));

	csuHeap_->createConstantBuffer(
		"view_", 
		Help::calculateConstantBufferSize(sizeof(XMFLOAT4X4)));
	csuHeap_->createConstantBufferView("view_", Help::calculateConstantBufferSize(sizeof(XMFLOAT4X4)));

	csuHeap_->createConstantBuffer(
		"projection_", 
		Help::calculateConstantBufferSize(sizeof(XMFLOAT4X4)));
	csuHeap_->createConstantBufferView("projection_", Help::calculateConstantBufferSize(sizeof(XMFLOAT4X4)));

	csuHeap_->createConstantBuffer(
		"offset", 
		Help::calculateConstantBufferSize(sizeof(XMFLOAT4X4)));
	csuHeap_->createConstantBufferView("offset", Help::calculateConstantBufferSize(sizeof(XMFLOAT3)));
}

void HelloWorld::prepare() {
	ThrowIfFailed(commandAllocator_->Reset());
	ThrowIfFailed(commandList_->Reset(commandAllocator_.Get(), nullptr));

	const CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		rtvHeap_->renderTarget(frameIndex_).Get(), 
		D3D12_RESOURCE_STATE_PRESENT, 
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	commandList_->ResourceBarrier(1, &barrier);
}

void HelloWorld::present() {
	const CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		rtvHeap_->renderTarget(frameIndex_).Get(), 
		D3D12_RESOURCE_STATE_RENDER_TARGET, 
		D3D12_RESOURCE_STATE_PRESENT);

	commandList_->ResourceBarrier(1, &barrier);

	ThrowIfFailed(commandList_->Close());

	ID3D12CommandList* cmdLists[] = {commandList_.Get()};
	commandQueue_->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	swapChain_->Present(1, 0);
}

void HelloWorld::clear() {
	commandList_->RSSetViewports(1, &viewport_);
	commandList_->RSSetScissorRects(1, &scissorRect_);
 
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap_->heap()->GetCPUDescriptorHandleForHeapStart(), frameIndex_, rtvDescriptorSize_);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(dsvHeap_->heap()->GetCPUDescriptorHandleForHeapStart());
	commandList_->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
 
	const float clearColor[] = {0.392f, 0.584f, 0.929f, 1.0f};
	commandList_->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	commandList_->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}
