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
#include "RootSignature.h"

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
	std::wstring s;

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
	if (currBlock_ && currBlock_->lengthTo(camera_.position()) > Help::distance_) {
		currBlock_ = nullptr;
	}

	if (currBlock_) {
		csuHeap_->upload("offset", &currBlock_->position(), sizeof(currBlock_->position()));
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

void HelloWorld::onKeyDown(WPARAM key) {
	camera_.onKeyDown(key);
	switch (key) {
	case '1':
		currBlockIndex_ = 1;
		break;
	case '2':
		currBlockIndex_ = 2;
		break;
    case '3':
		currBlockIndex_ = 3;
		break;
    case '4':
		currBlockIndex_ = 4;
		break;
    case '5':
		currBlockIndex_ = 5;
		break;
	case '6':
		currBlockIndex_ = 6;
		break;
	default:
		break;
	}
}

void HelloWorld::onKeyUp(WPARAM key) {
	camera_.onKeyUp(key);
}

void HelloWorld::onMouseMovement(LPARAM param) {
	camera_.onMouseMovement(param);
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
	default:
		return ;  
	}

	auto newBlock = new Block(*currBlock_, offset, currBlockIndex_);
	blocks_.push_back(newBlock);
	vertices_.insert(vertices_.end(), newBlock->begin(), newBlock->end());
	textureIndex_.insert(textureIndex_.end(), newBlock->textureIndex_.begin(), newBlock->textureIndex_.end());

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

	ThrowIfFailed(device_->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), 
		D3D12_HEAP_FLAG_NONE, 
		&CD3DX12_RESOURCE_DESC::Buffer(textureIndex_.size() * sizeof(UINT)), 
		D3D12_RESOURCE_STATE_GENERIC_READ, 
		nullptr, 
		IID_PPV_ARGS(&textureIndexBuffer_)));
	
	textureIndexBuffer_->Map(0, &range, reinterpret_cast<void**>(&textureIndexData_));
	memcpy(textureIndexData_, textureIndex_.data(), textureIndex_.size() * sizeof(UINT));
	textureIndexBuffer_->Unmap(0, nullptr);

	textureIndexBufferView_.BufferLocation = textureIndexBuffer_->GetGPUVirtualAddress();
	textureIndexBufferView_.SizeInBytes = textureIndex_.size() * sizeof(UINT);
	textureIndexBufferView_.StrideInBytes = sizeof(UINT);

	std::sort(blocks_.begin(), blocks_.end());
	auto start = std::chrono::system_clock::now();
	// bvh_->root_ = bvh_->buildBVH(blocks_);
	bvh_->root_ = bvh_->buildBVH(blocks_, 0, blocks_.size() - 1);
	auto end = std::chrono::system_clock::now();
	auto seconds = L"milliseconds: " + std::to_wstring(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
	// SetWindowText(hwnd_, seconds.c_str());
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
	textureIndex_.clear();

	for (auto& block : blocks_) {
		vertices_.insert(vertices_.end(), block->begin(), block->end());
		textureIndex_.insert(textureIndex_.end(), block->textureIndex_.begin(), block->textureIndex_.end());
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

	ThrowIfFailed(device_->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), 
		D3D12_HEAP_FLAG_NONE, 
		&CD3DX12_RESOURCE_DESC::Buffer(textureIndex_.size() * sizeof(UINT)), 
		D3D12_RESOURCE_STATE_GENERIC_READ, 
		nullptr, 
		IID_PPV_ARGS(&textureIndexBuffer_)));
	
	textureIndexBuffer_->Map(0, &range, reinterpret_cast<void**>(&textureIndexData_));
	memcpy(textureIndexData_, textureIndex_.data(), textureIndex_.size() * sizeof(UINT));
	textureIndexBuffer_->Unmap(0, nullptr);

	textureIndexBufferView_.BufferLocation = textureIndexBuffer_->GetGPUVirtualAddress();
	textureIndexBufferView_.SizeInBytes = textureIndex_.size() * sizeof(UINT);
	textureIndexBufferView_.StrideInBytes = sizeof(UINT);

	auto start = std::chrono::system_clock::now();
	bvh_->root_ = bvh_->buildBVH(blocks_, 0, blocks_.size() - 1);
	auto end = std::chrono::system_clock::now();
	auto seconds = L"milliseconds: " + std::to_wstring(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
	// SetWindowText(hwnd_, seconds.c_str());
}

void HelloWorld::onMouseWheel(WPARAM wParam) {
	auto delta = GET_WHEEL_DELTA_WPARAM(wParam);
	if (delta <= -120) {
		currBlockIndex_ += -delta / 120;
	} else if (delta >= 120) {
		currBlockIndex_ -= delta / 120;
	}

	if (currBlockIndex_ > numTextures_) {
		currBlockIndex_ = 1;
	} else if (currBlockIndex_ < 1) {
		currBlockIndex_ = numTextures_;
	}
}


void HelloWorld::populateCommandList() {
	commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList_->IASetVertexBuffers(0, 1, &skyBufferView_);
	commandList_->IASetVertexBuffers(1, 1, &textureIndexBufferView_);

	commandList_->SetGraphicsRootSignature(defaultRootSignature_->rootSignature());
	commandList_->SetPipelineState(skyboxPSO_->pipelineState());

	ID3D12DescriptorHeap* heaps[] = {csuHeap_->heap().Get()};
	commandList_->SetDescriptorHeaps(_countof(heaps), heaps);

	commandList_->SetGraphicsRootDescriptorTable(0, csuHeap_->gpuHandle("model"));
	commandList_->SetGraphicsRootDescriptorTable(1, csuHeap_->gpuHandle("skybox"));
	
	commandList_->DrawInstanced(36, 1, 0, 0);

	commandList_->SetPipelineState(defaultPSO_->pipelineState());
	commandList_->IASetVertexBuffers(0, 1, &blockBufferView_);
	commandList_->IASetVertexBuffers(1, 1, &textureIndexBufferView_);
	commandList_->DrawInstanced(vertices_.size(), 1, 0, 0);

	commandList_->SetGraphicsRootSignature(rectangleRootSignature_->rootSignature());
	commandList_->SetPipelineState(rectanglePSO_->pipelineState());
	commandList_->IASetVertexBuffers(0, 1, &rectangleView_);
	auto move = XMMatrixTranslation(-0.25, -0.91, 0.0);
	auto scale = XMMatrixScaling(1.0 / 20.0, 1.0/ 20.0, 1.0);
	auto model = scale * move;
	XMFLOAT4X4 m;
	XMStoreFloat4x4(&m, XMMatrixTranspose(model));
	csuHeap_->upload("recModel", &m, sizeof(m));
	float aspect = static_cast<float>(width_) / static_cast<float>(height_);
	csuHeap_->upload("aspect", &aspect, sizeof(float));
	csuHeap_->upload("selected", &currBlockIndex_, sizeof(currBlockIndex_));
	commandList_->SetGraphicsRootDescriptorTable(0, csuHeap_->gpuHandle("recModel"));
	commandList_->DrawInstanced(rectangleVertices_.size(), 1, 0, 0);

	commandList_->SetGraphicsRootSignature(squareRootSignature_->rootSignature());
	commandList_->SetPipelineState(squarePSO_->pipelineState());
	commandList_->SetGraphicsRootDescriptorTable(0, csuHeap_->gpuHandle("recModel"));
	commandList_->SetGraphicsRootDescriptorTable(1, csuHeap_->gpuHandle("grass2D"));
	commandList_->IASetVertexBuffers(0, 1, &squareView_);
	commandList_->DrawInstanced(squareVertices_.size(), 1, 0, 0);

	commandList_->SetGraphicsRootSignature(defaultRootSignature_->rootSignature());
	commandList_->SetPipelineState(boxPSO_->pipelineState()); 
	commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	commandList_->SetGraphicsRootDescriptorTable(0, csuHeap_->gpuHandle("model_"));
	if (currBlock_ != nullptr)
		commandList_->DrawInstanced(32, 1, 0, 0);

	commandList_->SetGraphicsRootSignature(cursorRootSignature_->rootSignature());
	commandList_->SetPipelineState(minecraftPSO_->pipelineState());
	scale = XMMatrixScaling(20.0f / static_cast<float>(width_), 20.0f / static_cast<float>(height_), 1.0f);
	XMFLOAT4X4 s;
	XMStoreFloat4x4(&s, XMMatrixTranspose(scale));
	csuHeap_->upload("scale", &s, sizeof(s));
	commandList_->SetGraphicsRootConstantBufferView(0, csuHeap_->buffer("scale")->GetGPUVirtualAddress());
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
	Block::setDevice(device_.Get());
}

void HelloWorld::createCommandQueue() {
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	ThrowIfFailed(device_->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue_)));

	my::DescriptorHeap::setCommandQueue(commandQueue_.Get());
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

	my::RootSignature::init(device_.Get(), featureData);

	std::vector<CD3DX12_DESCRIPTOR_RANGE1> ranges(2);
	std::vector<CD3DX12_ROOT_PARAMETER1> rootParameter(2);

	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 4, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	rootParameter[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);

	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1 + 6, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	rootParameter[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);

	std::vector<D3D12_STATIC_SAMPLER_DESC> sampler(1);
	sampler[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler[0].MipLODBias = 0;
	sampler[0].MaxAnisotropy = 0;
	sampler[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler[0].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
	sampler[0].MinLOD = 0.0f;
	sampler[0].MaxLOD = D3D12_FLOAT32_MAX;
	sampler[0].ShaderRegister = 0;
	sampler[0].RegisterSpace = 0;
	sampler[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	defaultRootSignature_ = std::make_unique<my::RootSignature>(2, rootParameter, 1, sampler);

	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 3, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	rootParameter[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	rootParameter[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);
	rectangleRootSignature_ = std::make_unique<my::RootSignature>(2, rootParameter);

	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 2, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	rootParameter[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 6, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	rootParameter[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);
	squareRootSignature_ = std::make_unique<my::RootSignature>(2, rootParameter, 1, sampler);

	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	rootParameter[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC);
	cursorRootSignature_ = std::make_unique<my::RootSignature>(1, rootParameter);
}

void HelloWorld::createPipeline() {
	std::vector<D3D12_INPUT_ELEMENT_DESC> input = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}, 
		{"TEXTURE_INDEX", 0, DXGI_FORMAT_R32_SINT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	auto shader = Shader(L"defaultVertex.hlsl", L"defaultPixel.hlsl");
	auto defaultDesc = PipelineState::getDefaultDesc();
	defaultDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	defaultPSO_ = std::make_unique<PipelineState>(device_.Get(), defaultRootSignature_->rootSignature(), input, &shader, defaultDesc);

	auto skyBoxPSODesc = PipelineState::getDefaultDesc();
	skyBoxPSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	skyBoxPSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	shader = Shader(L"skyboxVertex.hlsl", L"skyboxPixel.hlsl");
	skyboxPSO_ = std::make_unique<PipelineState>(device_.Get(), defaultRootSignature_->rootSignature(), input, &shader, skyBoxPSODesc);

	input = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	auto desc = PipelineState::getDefaultDesc();
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	shader = Shader(L"mineVertex.hlsl", L"mineGeometry.hlsl", L"minePixel.hlsl");
	minecraftPSO_ = std::make_unique<PipelineState>(device_.Get(), cursorRootSignature_->rootSignature(), input, &shader, desc);

	desc = PipelineState::getDefaultDesc();
	desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	shader = Shader(L"boxVertex.hlsl", L"boxPixel.hlsl");
	boxPSO_ = std::make_unique<PipelineState>(device_.Get(), defaultRootSignature_->rootSignature(), input, &shader, desc);

	input = {
		{"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};
	shader = Shader(L"rectangleVertex.hlsl", L"rectanglePixel.hlsl");
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	rectanglePSO_ = std::make_unique<PipelineState>(device_.Get(), rectangleRootSignature_->rootSignature(), input, &shader, desc);

	shader = Shader(L"squareVertex.hlsl", L"squarePixel.hlsl");
	squarePSO_ = std::make_unique<PipelineState>(device_.Get(), squareRootSignature_->rootSignature(), input, &shader, desc);
}

void HelloWorld::createDescriptorHeap() {
	rtvHeap_ = std::make_unique<RTVHeap>(device_.Get(), frameCount_);
	rtvHeap_->createRenderTargetView(swapChain_.Get());

	csuHeap_ = std::make_unique<CSUHeap>(device_, 12, 1 + 6 + 6, 0);

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
			// for (int k = 1; k <= 100; k++) {
				auto offset = XMFLOAT3(i, 0, j);
				auto block = new Block(Block(), offset);
				blocks_.push_back(block);
				vertices_.insert(vertices_.end(), block->begin(), block->end());
				textureIndex_.insert(textureIndex_.end(), block->textureIndex_.begin(), block->textureIndex_.end());
			// }
		}
	}

	// block = new Block(XMFLOAT3(0.0f, 0.0f, 0.0f));
	// blocks_.push_back(block);
	// vertices_.insert(vertices_.end(), block->begin(), block->end());
	// textureIndex_.insert(textureIndex_.end(), block->textureIndex_.begin(), block->textureIndex_.end());
	// bvh_->root_ = bvh_->buildBVH(blocks_, 0, blocks_.size() - 1);
	std::sort(blocks_.begin(), blocks_.end());
	bvh_->root_ = bvh_->buildBVH(blocks_, 0, blocks_.size() - 1);

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

	ThrowIfFailed(device_->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), 
		D3D12_HEAP_FLAG_NONE, 
		&CD3DX12_RESOURCE_DESC::Buffer(textureIndex_.size() * sizeof(UINT)), 
		D3D12_RESOURCE_STATE_GENERIC_READ, 
		nullptr, 
		IID_PPV_ARGS(&textureIndexBuffer_)));
	
	textureIndexBuffer_->Map(0, &range, reinterpret_cast<void**>(&textureIndexData_));
	memcpy(textureIndexData_, textureIndex_.data(), textureIndex_.size() * sizeof(int));
	textureIndexBuffer_->Unmap(0, nullptr);

	textureIndexBufferView_.BufferLocation = textureIndexBuffer_->GetGPUVirtualAddress();
	textureIndexBufferView_.SizeInBytes = textureIndex_.size() * sizeof(UINT);
	textureIndexBufferView_.StrideInBytes = sizeof(UINT);

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

	auto rec1 = my::Rectangle(XMFLOAT2(-1.0 + 0.05, 0.0), 0.1, 2.0);
	auto rec2 = my::Rectangle(XMFLOAT2(1.0 - 0.05, 0.0), 0.1, 2.0);
	auto rec3 = my::Rectangle(XMFLOAT2(0.0, 1.0 - 0.05), 2.0, 0.1);
	auto rec4 = my::Rectangle(XMFLOAT2(0.0, -1.0 + 0.05), 2.0, 0.1);

	auto rec1_2 = my::Rectangle(rec1, XMFLOAT2(2.0f, 0.0f));
	auto rec2_2 = my::Rectangle(rec2, XMFLOAT2(2.0f, 0.0f));
	auto rec3_2 = my::Rectangle(rec3, XMFLOAT2(2.0f, 0.0f));
	auto rec4_2 = my::Rectangle(rec4, XMFLOAT2(2.0f, 0.0f));

	auto rec1_3 = my::Rectangle(rec1_2, XMFLOAT2(2.0f, 0.0f));
	auto rec2_3 = my::Rectangle(rec2_2, XMFLOAT2(2.0f, 0.0f));
	auto rec3_3 = my::Rectangle(rec3_2, XMFLOAT2(2.0f, 0.0f));
	auto rec4_3 = my::Rectangle(rec4_2, XMFLOAT2(2.0f, 0.0f));

	auto rec1_4 = my::Rectangle(rec1_3, XMFLOAT2(2.0f, 0.0f));
	auto rec2_4 = my::Rectangle(rec2_3, XMFLOAT2(2.0f, 0.0f));
	auto rec3_4 = my::Rectangle(rec3_3, XMFLOAT2(2.0f, 0.0f));
	auto rec4_4 = my::Rectangle(rec4_3, XMFLOAT2(2.0f, 0.0f));

	auto rec1_5 = my::Rectangle(rec1_4, XMFLOAT2(2.0f, 0.0f));
	auto rec2_5 = my::Rectangle(rec2_4, XMFLOAT2(2.0f, 0.0f));
	auto rec3_5 = my::Rectangle(rec3_4, XMFLOAT2(2.0f, 0.0f));
	auto rec4_5 = my::Rectangle(rec4_4, XMFLOAT2(2.0f, 0.0f));

	auto rec1_6 = my::Rectangle(rec1_5, XMFLOAT2(2.0f, 0.0f));
	auto rec2_6 = my::Rectangle(rec2_5, XMFLOAT2(2.0f, 0.0f));
	auto rec3_6 = my::Rectangle(rec3_5, XMFLOAT2(2.0f, 0.0f));
	auto rec4_6 = my::Rectangle(rec4_5, XMFLOAT2(2.0f, 0.0f));

	rectangleVertices_.insert(rectangleVertices_.end(), rec1.vertices_.begin(), rec1.vertices_.end());
	rectangleVertices_.insert(rectangleVertices_.end(), rec2.vertices_.begin(), rec2.vertices_.end());
	rectangleVertices_.insert(rectangleVertices_.end(), rec3.vertices_.begin(), rec3.vertices_.end());
	rectangleVertices_.insert(rectangleVertices_.end(), rec4.vertices_.begin(), rec4.vertices_.end());
	 
	rectangleVertices_.insert(rectangleVertices_.end(), rec1_2.vertices_.begin(), rec1_2.vertices_.end());
	rectangleVertices_.insert(rectangleVertices_.end(), rec2_2.vertices_.begin(), rec2_2.vertices_.end());
	rectangleVertices_.insert(rectangleVertices_.end(), rec3_2.vertices_.begin(), rec3_2.vertices_.end());
	rectangleVertices_.insert(rectangleVertices_.end(), rec4_2.vertices_.begin(), rec4_2.vertices_.end());

	rectangleVertices_.insert(rectangleVertices_.end(), rec1_3.vertices_.begin(), rec1_3.vertices_.end());
	rectangleVertices_.insert(rectangleVertices_.end(), rec2_3.vertices_.begin(), rec2_3.vertices_.end());
	rectangleVertices_.insert(rectangleVertices_.end(), rec3_3.vertices_.begin(), rec3_3.vertices_.end());
	rectangleVertices_.insert(rectangleVertices_.end(), rec4_3.vertices_.begin(), rec4_3.vertices_.end());

	rectangleVertices_.insert(rectangleVertices_.end(), rec1_4.vertices_.begin(), rec1_4.vertices_.end());
	rectangleVertices_.insert(rectangleVertices_.end(), rec2_4.vertices_.begin(), rec2_4.vertices_.end());
	rectangleVertices_.insert(rectangleVertices_.end(), rec3_4.vertices_.begin(), rec3_4.vertices_.end());
	rectangleVertices_.insert(rectangleVertices_.end(), rec4_4.vertices_.begin(), rec4_4.vertices_.end());

	rectangleVertices_.insert(rectangleVertices_.end(), rec1_5.vertices_.begin(), rec1_5.vertices_.end());
	rectangleVertices_.insert(rectangleVertices_.end(), rec2_5.vertices_.begin(), rec2_5.vertices_.end());
	rectangleVertices_.insert(rectangleVertices_.end(), rec3_5.vertices_.begin(), rec3_5.vertices_.end());
	rectangleVertices_.insert(rectangleVertices_.end(), rec4_5.vertices_.begin(), rec4_5.vertices_.end());

	rectangleVertices_.insert(rectangleVertices_.end(), rec1_6.vertices_.begin(), rec1_6.vertices_.end());
	rectangleVertices_.insert(rectangleVertices_.end(), rec2_6.vertices_.begin(), rec2_6.vertices_.end());
	rectangleVertices_.insert(rectangleVertices_.end(), rec3_6.vertices_.begin(), rec3_6.vertices_.end());
	rectangleVertices_.insert(rectangleVertices_.end(), rec4_6.vertices_.begin(), rec4_6.vertices_.end());

	ThrowIfFailed(device_->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), 
		D3D12_HEAP_FLAG_NONE, 
		&CD3DX12_RESOURCE_DESC::Buffer(rectangleVertices_.size() * sizeof(my::Rectangle::Vertex)), 
		D3D12_RESOURCE_STATE_GENERIC_READ, 
		nullptr, 
		IID_PPV_ARGS(&rectangleBuffer_)));
	
	rectangleBuffer_->Map(0, &range, reinterpret_cast<void**>(&rectangleData_));
	memcpy(rectangleData_, rectangleVertices_.data(), rectangleVertices_.size() * sizeof(my::Rectangle::Vertex));
	textureIndexBuffer_->Unmap(0, nullptr);

	rectangleView_.BufferLocation = rectangleBuffer_->GetGPUVirtualAddress();
	rectangleView_.SizeInBytes = rectangleVertices_.size() * sizeof(my::Rectangle::Vertex);
	rectangleView_.StrideInBytes = sizeof(my::Rectangle::Vertex);

	auto square1 = my::Square(XMFLOAT2(0.0, 0.0), 0.9);
	auto square2 = my::Square(square1, XMFLOAT2(2.0, 0.0));
	auto square3 = my::Square(square2, XMFLOAT2(2.0, 0.0));
	auto square4 = my::Square(square3, XMFLOAT2(2.0, 0.0));
	auto square5 = my::Square(square4, XMFLOAT2(2.0, 0.0));
	auto square6 = my::Square(square5, XMFLOAT2(2.0, 0.0));

	squareVertices_.insert(squareVertices_.end(), square1.vertices_.begin(), square1.vertices_.end());
	squareVertices_.insert(squareVertices_.end(), square2.vertices_.begin(), square2.vertices_.end());
	squareVertices_.insert(squareVertices_.end(), square3.vertices_.begin(), square3.vertices_.end());
	squareVertices_.insert(squareVertices_.end(), square4.vertices_.begin(), square4.vertices_.end());
	squareVertices_.insert(squareVertices_.end(), square5.vertices_.begin(), square5.vertices_.end());
	squareVertices_.insert(squareVertices_.end(), square6.vertices_.begin(), square6.vertices_.end());

	ThrowIfFailed(device_->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), 
		D3D12_HEAP_FLAG_NONE, 
		&CD3DX12_RESOURCE_DESC::Buffer(squareVertices_.size() * sizeof(my::Square::Vertex)), 
		D3D12_RESOURCE_STATE_GENERIC_READ, 
		nullptr, 
		IID_PPV_ARGS(&squareBuffer_)));
	
	squareBuffer_->Map(0, &range, reinterpret_cast<void**>(&squareData_));
	memcpy(squareData_, squareVertices_.data(), squareVertices_.size() * sizeof(my::Square::Vertex));
	squareBuffer_->Unmap(0, nullptr);

	squareView_.BufferLocation = squareBuffer_->GetGPUVirtualAddress();
	squareView_.SizeInBytes = squareVertices_.size() * sizeof(my::Square::Vertex);
	squareView_.StrideInBytes = sizeof(my::Square::Vertex);
}

void HelloWorld::loadAssets() {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	csuHeap_->createShaderResourceView("skybox", "textures/snowcube1024.dds", srvDesc);

	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	csuHeap_->createShaderResourceView("grass", "textures/grass.dds", srvDesc);

	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	csuHeap_->createShaderResourceView("ice", "textures/ice.dds", srvDesc);

	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	csuHeap_->createShaderResourceView("planks", "textures/planks.dds", srvDesc);

	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	csuHeap_->createShaderResourceView("stone", "textures/stone.dds", srvDesc);

	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	csuHeap_->createShaderResourceView("woolBlue", "textures/woolBlue.dds", srvDesc);

	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	csuHeap_->createShaderResourceView("sand", "textures/sand.dds", srvDesc);

 	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	csuHeap_->createShaderResourceView("grass2D", "textures/grass2D.dds", srvDesc);
	csuHeap_->createShaderResourceView("ice2D", "textures/ice2D.dds", srvDesc);
	csuHeap_->createShaderResourceView("planks2D", "textures/planks2D.dds", srvDesc);
	csuHeap_->createShaderResourceView("stone2D", "textures/stone2D.dds", srvDesc);
	csuHeap_->createShaderResourceView("woolBlue2D", "textures/woolBlue2D.dds", srvDesc);
	csuHeap_->createShaderResourceView("sand2D", "textures/sand2D.dds", srvDesc);


	csuHeap_->createConstantBufferView("model", Help::calculateConstantBufferSize(sizeof(XMFLOAT4X4)));

	csuHeap_->createConstantBufferView("view", Help::calculateConstantBufferSize(sizeof(XMFLOAT4X4)));

	csuHeap_->createConstantBufferView("projection", Help::calculateConstantBufferSize(sizeof(XMFLOAT4X4)));

	csuHeap_->createConstantBufferView("mvp", Help::calculateConstantBufferSize(sizeof(XMFLOAT4X4)));

	csuHeap_->createConstantBufferView("model_", Help::calculateConstantBufferSize(sizeof(XMFLOAT4X4)));

	csuHeap_->createConstantBufferView("view_", Help::calculateConstantBufferSize(sizeof(XMFLOAT4X4)));

	csuHeap_->createConstantBufferView("projection_", Help::calculateConstantBufferSize(sizeof(XMFLOAT4X4)));

	csuHeap_->createConstantBufferView("offset", Help::calculateConstantBufferSize(sizeof(XMFLOAT3)));

	csuHeap_->createConstantBufferView("recModel", Help::calculateConstantBufferSize(sizeof(XMFLOAT4X4)));
	csuHeap_->createConstantBufferView("aspect", Help::calculateConstantBufferSize(sizeof(float)));
	csuHeap_->createConstantBufferView("selected", Help::calculateConstantBufferSize(sizeof(int)));

	csuHeap_->createConstantBufferView("scale", Help::calculateConstantBufferSize(sizeof(XMFLOAT4X4)));
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
