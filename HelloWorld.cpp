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

#include <MyDirectx/Geometry.h>


using namespace physx;

HelloWorld::HelloWorld(UINT width, UINT height, std::wstring name) : myDirectx_(width, height, name), 
                                                                     MySample(width, height, std::move(name)),
                                                                     rtvDescriptorSize_(0) {
}

void HelloWorld::onInit() {
	myDirectx_.hwnd_ = hwnd_;
	myDirectx_.onInit();

	My::DescriptorHeap::init(myDirectx_.getDevice(), myDirectx_.getCommandQueue());
	VertexBuffer<Block::Vertex>::setDevice(myDirectx_.getDevice());
	VertexBuffer<UINT>::setDevice(myDirectx_.getDevice());
	VertexBuffer<my::Rectangle::Vertex>::setDevice(myDirectx_.getDevice());
	VertexBuffer<my::Square::Vertex>::setDevice(myDirectx_.getDevice());

	createRootSignature();
	createPipeline();
	createDescriptorHeap();

	loadAssets();

	createVertex();

	gameTimer_.reset();

	auto pose = physx::PxVec3(10.0f, 1.5f, 10.0f);
	auto body = world_.createBoxDynamic(pose, 0.4f, 1.0f, 0.4f);
	body->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_X, true);
	body->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y, true);
	body->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z, true);

	camera_.scene_ = world_.scene_;
	camera_.body_ = body;
	camera_.hwnd_ = hwnd_;

	auto block = Block(XMFLOAT3(0.0f, 0.0f, 0.0f), 0.4f, 5);
	cameraVertices_.insert(cameraVertices_.end(), block.begin(), block.end());
	cameraVertex_ = std::make_unique<VertexBuffer<Block::Vertex>>(cameraVertices_);
	cameraTextureVertex_ = std::make_unique<VertexBuffer<UINT>>(block.textureVertices_);

	keyboard_ = std::make_unique<DirectX::Keyboard>();
	mouse_ = std::make_unique<DirectX::Mouse>();
	mouse_->SetWindow(hwnd_);
	mouse_->SetVisible(true);
	mouse_->SetMode(DirectX::Mouse::Mode::MODE_RELATIVE);

	ThrowIfFailed(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
	audioEngine_ = std::make_unique<DirectX::AudioEngine>();

	moveSoundEffect_ = std::make_unique<SoundEffect>(audioEngine_.get(), L"audio/move.wav");
}

void HelloWorld::onUpdate(WPARAM wParam, LPARAM lParam) {
	// WCHAR msg[128];
	// auto p_ = camera_.body_->getGlobalPose().p;
	// swprintf_s(msg, 128, L"x: %0.1f, y: %0.1f, z: %0.1f", p_.x, p_.y, p_.z);
	//
	// SetWindowText(hwnd_, msg);
	// if (!audioEngine_->Update()) {
	// 	int i = 0;
	// }
	PxVec3 dir(0.0f, -1.0f, 0.0f);
	PxRaycastBuffer hitCall;
	PxQueryFilterData data;

	bool hit = world_.scene_->raycast(camera_.body_->getGlobalPose().p + PxVec3(0.0f, -1.0001f, 0.0f), dir, 0.1f, hitCall, PxHitFlag::eDEFAULT, data);
	camera_.sky_ = !hit;

	auto x = PxVec3(1.0f, 0.0f, 0.0f);
	auto y = PxVec3(0.0f, 1.0f, 0.0f);
	auto z = PxVec3(0.0f, 0.0f ,1.0f);

	camera_.xStop_ = world_.scene_->raycast(camera_.body_->getGlobalPose().p + PxVec3(0.5001f,0.0f, 0.0f), x, 0.00001f, hitCall, PxHitFlag::eDEFAULT, data);
	camera_.negXStop_ = world_.scene_->raycast(camera_.body_->getGlobalPose().p + PxVec3(-0.5001f,0.0f, 0.0f), -x, 0.00001f, hitCall, PxHitFlag::eDEFAULT, data);
	camera_.yStop_ = world_.scene_->raycast(camera_.body_->getGlobalPose().p + PxVec3(0.0f,1.0001f, 0.0f), y, 0.00001f, hitCall, PxHitFlag::eDEFAULT, data);
	camera_.negYStop_ = world_.scene_->raycast(camera_.body_->getGlobalPose().p + PxVec3(0.0f,-1.0001f, 0.0f), -y, 0.00001f, hitCall, PxHitFlag::eDEFAULT, data);
	camera_.zStop_ = world_.scene_->raycast(camera_.body_->getGlobalPose().p + PxVec3(0.0f,0.0f, 0.5001f), z, 0.00001f, hitCall, PxHitFlag::eDEFAULT, data);
	camera_.negZStop_ = world_.scene_->raycast(camera_.body_->getGlobalPose().p + PxVec3(0.0f,0.0f, -0.5001f), -z, 0.00001f, hitCall, PxHitFlag::eDEFAULT, data);

	auto mouse = mouse_->GetState();
	mouseTracker_.Update(mouse);
	auto keyboard = keyboard_->GetState();
	keyboardTracker_.Update(keyboard);

	onMouseWheel(mouse);
	onKey(keyboardTracker_);

	gameTimer_.tick();

	camera_.onMouseMove(mouse.x, mouse.y);

	if (mouseTracker_.leftButton == Mouse::ButtonStateTracker::RELEASED) {
		onLeftButtonDown();
	}
	if (mouseTracker_.rightButton == Mouse::ButtonStateTracker::RELEASED) {
		onRightButtonDown();
	}

	camera_.update(keyboard, keyboardTracker_);
	world_.step(gameTimer_.deltaTime());
	// auto velocity = camera_.body_->getLinearVelocity();
	// if (!camera_.sky_ && (!Help::lessEqual(velocity.x, 0.005f) || !Help::lessEqual(velocity.y, 0.005f) || !Help::lessEqual(velocity.z, 0.005f))) {
	// 	if (!moveSoundEffect_->IsInUse())
	// 		moveSoundEffect_->Play();
	// }

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

	// s = std::to_wstring(1.0 / gameTimer_.deltaTime());
}

void HelloWorld::onRender() {
	myDirectx_.prepare();
	myDirectx_.clear();

	populateCommandList();

	myDirectx_.present();

	myDirectx_.waitForPreviousFrame();

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
	myDirectx_.onDestroy();
	soundEffectInstance_.release();
	soundEffect_.release();
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
	{
		auto p = camera_.body_->getGlobalPose().p;
		bool overlap = PxGeometryQuery::overlap(PxBoxGeometry(0.4f, 1.0f, 0.4f), PxTransform(p), PxBoxGeometry(0.5f, 0.5f, 0.5f), PxTransform(PxVec3(newBlock->position().x, newBlock->position().y, newBlock->position().z)));
		if (overlap) {
			return ;
		}
	}
	// if (callBack.) {
	// 	return ;
	// }
	// if (newBlock->lengthTo(XMFLOAT3(p.x, p.y, p.z)) < 1.5f) {
	// 	return ;
	// 	
	// }
	auto pose = physx::PxVec3(newBlock->x(), newBlock->y(), newBlock->z());
	auto body = world_.createBoxStatic(pose, 0.5f, 0.5f, 0.5f);
	newBlock->body_ = body;

	blocks_.push_back(newBlock);
	blockVertices_.insert(blockVertices_.end(), newBlock->begin(), newBlock->end());
	blockVertex_ = std::make_unique<VertexBuffer<Block::Vertex>>(blockVertices_);

	textureVertices_.insert(textureVertices_.end(), newBlock->textureVertices_.begin(), newBlock->textureVertices_.end());
	textureVertex_ = std::make_unique<VertexBuffer<UINT>>(textureVertices_);

	std::sort(blocks_.begin(), blocks_.end());
	auto start = std::chrono::system_clock::now();
	// bvh_->root_ = bvh_->buildBVH(blocks_);
	bvh_->root_ = bvh_->buildBVH(blocks_, 0, blocks_.size() - 1);
	auto end = std::chrono::system_clock::now();
	auto seconds = L"milliseconds: " + std::to_wstring(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());

	if (soundEffectInstance_) {
		auto s = soundEffectInstance_->GetState();
		if (s == SoundState::PLAYING) {
			soundEffectInstance_->Pause();
			soundEffectInstance_.reset();
		}
	}
	soundEffect_ = std::make_unique<SoundEffect>(audioEngine_.get(), L"audio/placeBlocks.wav");
	soundEffectInstance_ = soundEffect_->CreateInstance();
	soundEffectInstance_->Play();
}

void HelloWorld::onLeftButtonDown() {
	if (currBlock_ == nullptr) {
		return ;
	}

	currBlock_->move_ = true;
	blockVertices_.clear();
	textureVertices_.clear();
	for (auto it = blocks_.begin(); it != blocks_.end(); ) {
		if ((*it)->move_) {
			world_.scene_->removeActor(*(*it)->body_);
			it = blocks_.erase(it);
		} else {
			blockVertices_.insert(blockVertices_.end(), (*it)->begin(), (*it)->end());
			textureVertices_.insert(textureVertices_.end(), (*it)->textureVertices_.begin(), (*it)->textureVertices_.end());
			++it;
		}
	}

	currBlock_ = nullptr;

	if (blockVertices_.empty()) {
		return ;
	}

	blockVertex_ = std::make_unique<VertexBuffer<Block::Vertex>>(blockVertices_);

	textureVertex_ = std::make_unique<VertexBuffer<UINT>>(textureVertices_);

	auto start = std::chrono::system_clock::now();
	bvh_->root_ = bvh_->buildBVH(blocks_, 0, blocks_.size() - 1);
	auto end = std::chrono::system_clock::now();
	auto seconds = L"milliseconds: " + std::to_wstring(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
	// SetWindowText(hwnd_, seconds.c_str());

	if (soundEffectInstance_) {
		auto s = soundEffectInstance_->GetState();
		if (s == SoundState::PLAYING) {
			soundEffectInstance_->Pause();
			soundEffectInstance_.reset();
		}
	}
	soundEffect_ = std::make_unique<SoundEffect>(audioEngine_.get(), L"audio/placeBlocks.wav");
	soundEffectInstance_ = soundEffect_->CreateInstance();
	soundEffectInstance_->Play();
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

void HelloWorld::onKey(DirectX::Keyboard::KeyboardStateTracker tracker) {
	if (tracker.pressed.D1) {
		currBlockIndex_ = 1;
	}
	if (tracker.pressed.D2) {
		currBlockIndex_ = 2;
	}
	if (tracker.pressed.D3) {
		currBlockIndex_ = 3;
	}
	if (tracker.pressed.D4) {
		currBlockIndex_ = 4;
	}
	if (tracker.pressed.D5) {
		currBlockIndex_ = 5;
	}
	if (tracker.pressed.D6) {
		currBlockIndex_ = 6;
	}
}

void HelloWorld::onMouseWheel(DirectX::Mouse::State state) {
	static int previousValue = 0;
	auto value = std::abs(state.scrollWheelValue - previousValue);
	if (value >= 120) {
		if (state.scrollWheelValue > previousValue) {
			currBlockIndex_ -= value / 120;
		} else {
			currBlockIndex_ += value / 120;
		}
		if (currBlockIndex_ > 6) {
			currBlockIndex_ = currBlockIndex_ % 6;
		} else if (currBlockIndex_ < 1) {
			value /= 120;
			currBlockIndex_ += value;
			value %= 6;
			if (currBlockIndex_ > value) {
				currBlockIndex_ -= value;
			} else {
				currBlockIndex_ = 6 - (value - currBlockIndex_);
			}
		}
	}
	previousValue = state.scrollWheelValue;
}


void HelloWorld::populateCommandList() {
	auto commandList_ = myDirectx_.getCommandList();
	commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList_->IASetVertexBuffers(0, 1, &skyVertex_->view());
	commandList_->IASetVertexBuffers(1, 1, &textureVertex_->view());

	commandList_->SetGraphicsRootSignature(defaultRootSignature_->rootSignature());
	commandList_->SetPipelineState(skyboxPSO_->pipelineState());

	ID3D12DescriptorHeap* heaps[] = {csuHeap_->heap().Get()};
	commandList_->SetDescriptorHeaps(_countof(heaps), heaps);

	commandList_->SetGraphicsRootDescriptorTable(0, csuHeap_->gpuHandle("model"));
	commandList_->SetGraphicsRootDescriptorTable(1, csuHeap_->gpuHandle("skybox"));
	
	commandList_->DrawInstanced(36, 1, 0, 0);

	commandList_->SetPipelineState(defaultPSO_->pipelineState());
	// commandList_->IASetVertexBuffers(0, 1, &blockBufferView_);
	commandList_->IASetVertexBuffers(0, 1, &blockVertex_->view());
	commandList_->IASetVertexBuffers(1, 1, &textureVertex_->view());
	commandList_->DrawInstanced(blockVertex_->count(), 1, 0, 0);

	auto p = camera_.body_->getGlobalPose().p;
	// p.x = 50.0f; p.y = 10.0f; p.z = 50.0f;
	auto r = camera_.body_->getGlobalPose().q;

	auto move = XMMatrixTranslation(p.x, p.y, p.z);
	auto rotate = XMMatrixRotationQuaternion(XMLoadFloat4(&XMFLOAT4(r.x, r.y, r.z, r.w)));
	XMFLOAT4X4 m, v, proj;
	XMStoreFloat4x4(&m, XMMatrixTranspose(rotate * move));
	auto view = camera_.getViewMatrix();
	auto projection = camera_.getProjectionMatrix(width_, height_, 0.1f, 1000.0f);
	XMStoreFloat4x4(&v, XMMatrixTranspose(view));
	XMStoreFloat4x4(&proj, XMMatrixTranspose(projection));
	csuHeap_->upload("cameraModel", &m, sizeof(m));
	csuHeap_->upload("cameraView", &v, sizeof(v));
	csuHeap_->upload("cameraProjection", &proj, sizeof(proj));
	commandList_->SetGraphicsRootSignature(cameraRootSignature_->rootSignature());
	commandList_->SetPipelineState(cameraPSO_->pipelineState());
	commandList_->SetGraphicsRootDescriptorTable(0, csuHeap_->gpuHandle("cameraModel"));
	commandList_->IASetVertexBuffers(0, 1, &cameraVertex_->view());
	commandList_->IASetVertexBuffers(1, 1, &cameraTextureVertex_->view());
	commandList_->DrawInstanced(cameraVertex_->count(), 1, 0, 0);

	commandList_->SetGraphicsRootSignature(rectangleRootSignature_->rootSignature());
	commandList_->SetPipelineState(rectanglePSO_->pipelineState());
	commandList_->IASetVertexBuffers(0, 1, &rectangleVertex_->view());
	move = XMMatrixTranslation(-0.25, -0.91, 0.0);
	auto scale = XMMatrixScaling(1.0 / 20.0, 1.0/ 20.0, 1.0);
	auto model_ = scale * move;
	XMStoreFloat4x4(&m, XMMatrixTranspose(model_));
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
	commandList_->IASetVertexBuffers(0, 1, &squareVertex_->view());
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

void HelloWorld::createRootSignature() {
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

	if (FAILED(myDirectx_.getDevice()->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData)))) {
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	my::RootSignature::init(myDirectx_.getDevice(), featureData);

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
	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1 + 6, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	rootParameter[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);
	cameraRootSignature_ = std::make_unique<my::RootSignature>(2, rootParameter, 1, sampler);

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
	defaultPSO_ = std::make_unique<PipelineState>(myDirectx_.getDevice(), defaultRootSignature_->rootSignature(), input, &shader, defaultDesc);

	shader = Shader(L"cameraVertex.hlsl", L"cameraPixel.hlsl");
	cameraPSO_ = std::make_unique<PipelineState>(myDirectx_.getDevice(), cameraRootSignature_->rootSignature(), input, &shader);

	auto skyBoxPSODesc = PipelineState::getDefaultDesc();
	skyBoxPSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	skyBoxPSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	shader = Shader(L"skyboxVertex.hlsl", L"skyboxPixel.hlsl");
	skyboxPSO_ = std::make_unique<PipelineState>(myDirectx_.getDevice(), defaultRootSignature_->rootSignature(), input, &shader, skyBoxPSODesc);

	input = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	auto desc = PipelineState::getDefaultDesc();
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	shader = Shader(L"mineVertex.hlsl", L"mineGeometry.hlsl", L"minePixel.hlsl");
	minecraftPSO_ = std::make_unique<PipelineState>(myDirectx_.getDevice(), cursorRootSignature_->rootSignature(), input, &shader, desc);

	desc = PipelineState::getDefaultDesc();
	desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	shader = Shader(L"boxVertex.hlsl", L"boxPixel.hlsl");
	boxPSO_ = std::make_unique<PipelineState>(myDirectx_.getDevice(), defaultRootSignature_->rootSignature(), input, &shader, desc);

	input = {
		{"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};
	shader = Shader(L"rectangleVertex.hlsl", L"rectanglePixel.hlsl");
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	rectanglePSO_ = std::make_unique<PipelineState>(myDirectx_.getDevice(), rectangleRootSignature_->rootSignature(), input, &shader, desc);

	shader = Shader(L"squareVertex.hlsl", L"squarePixel.hlsl");
	squarePSO_ = std::make_unique<PipelineState>(myDirectx_.getDevice(), squareRootSignature_->rootSignature(), input, &shader, desc);
}

void HelloWorld::createDescriptorHeap() {
	csuHeap_ = std::make_unique<CSUHeap>(15, 1 + 6 + 6, 0);
}

void HelloWorld::createVertex() {
	bvh_ = new BVH;

	auto skyBlock = new Block(XMFLOAT3(0.0, 0.0f, 0.0f));
	skyVertices_.insert(skyVertices_.end(), skyBlock->begin(), skyBlock->end());
	skyVertex_ = std::make_unique<VertexBuffer<Block::Vertex>>(skyVertices_);

	for (int i = 1; i <= 100; i++) {
		for (int j = 1; j <= 100; j++) {
			// for (int k = 1; k <= 100; k++) {
				auto offset = XMFLOAT3(static_cast<float>(i), 0, static_cast<float>(j));
				auto block = new Block(offset);
				physx::PxVec3 pose(offset.x, offset.y, offset.z);
				auto half = 0.5f;
				auto body = world_.createBoxStatic(pose, half, half, half);
				block->body_ = body;
				blocks_.push_back(block);
				blockVertices_.insert(blockVertices_.end(), block->begin(), block->end());
				textureVertices_.insert(textureVertices_.end(), block->textureVertices_.begin(), block->textureVertices_.end());
			// }
		}
	}

	std::sort(blocks_.begin(), blocks_.end());
	bvh_->root_ = bvh_->buildBVH(blocks_, 0, blocks_.size() - 1);

	blockVertex_ = std::make_unique<VertexBuffer<Block::Vertex>>(blockVertices_);

	textureVertex_ = std::make_unique<VertexBuffer<UINT>>(textureVertices_);

	struct Point {
		XMFLOAT3 pos;
	};
	std::array<Point, 1> cross = {
		XMFLOAT3(center_.x, center_.y, 0.0)
	};

	ThrowIfFailed(myDirectx_.getDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), 
		D3D12_HEAP_FLAG_NONE, 
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(cross)), 
		D3D12_RESOURCE_STATE_GENERIC_READ, 
		nullptr, 
		IID_PPV_ARGS(&crossHairBuffer_)));

	CD3DX12_RANGE range(0, 0);
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

	rectangleVertex_ = std::make_unique<VertexBuffer<my::Rectangle::Vertex>>(rectangleVertices_);

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

	squareVertex_ = std::make_unique<VertexBuffer<my::Square::Vertex>>(squareVertices_);
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

	csuHeap_->createConstantBufferView("cameraModel", Help::calculateConstantBufferSize(sizeof(XMFLOAT4X4)));
	csuHeap_->createConstantBufferView("cameraView", Help::calculateConstantBufferSize(sizeof(XMFLOAT4X4)));
	csuHeap_->createConstantBufferView("cameraProjection", Help::calculateConstantBufferSize(sizeof(XMFLOAT4X4)));

}