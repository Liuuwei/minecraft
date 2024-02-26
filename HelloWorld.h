#pragma once

#include <windows.h>
#include <WinUser.h>
#include <directxtk12/Keyboard.h>
#include <directxtk12/Mouse.h>
#include <directxtk12/Audio.h>

#include "Animation.h"
#include "Block.h"
#include "BVH.h"
#include "Camera.h"
#include "GameTimer.h"
#include "PipelineState.h"
#include "Rectangle.h"
#include "RootSignature.h"
#include "Square.h"
#include "World.h"

#include <MyDirectx/MyDirectx.h>
#include <MyDirectx/CSUHeap.h>
#include <MyDirectx/VertexBuffer.h>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

class HelloWorld : public MySample {
public:
	HelloWorld(UINT width, UINT height, std::wstring name);

	MyDirectx myDirectx_;
	World world_;

	void onInit() override;
	void onUpdate(WPARAM wParam, LPARAM lParam) override;
	void onRender() override;
	void onDestroy() override;

	void onMouseWheel(DirectX::Mouse::State state);
	void onKey(DirectX::Keyboard::KeyboardStateTracker tracker);

	void onRightButtonDown() override;
	void onLeftButtonDown() override;

	void onMouseWheel(WPARAM wParam) override;

private:
	void show(const std::wstring& msg) {
		SetWindowText(hwnd_, msg.c_str());
	}

	static const UINT numTextures_ = 6;

	std::unique_ptr<my::RootSignature> defaultRootSignature_;
	std::unique_ptr<my::RootSignature> rectangleRootSignature_;
	std::unique_ptr<my::RootSignature> squareRootSignature_;
	std::unique_ptr<my::RootSignature> cursorRootSignature_;
	std::unique_ptr<my::RootSignature> cameraRootSignature_;
	std::unique_ptr<PipelineState> defaultPSO_;
	std::unique_ptr<PipelineState> skyboxPSO_;
	std::unique_ptr<PipelineState> minecraftPSO_;
	std::unique_ptr<PipelineState> boxPSO_;
	std::unique_ptr<PipelineState> rectanglePSO_;
	std::unique_ptr<PipelineState> squarePSO_;
	std::unique_ptr<PipelineState> cameraPSO_;
	UINT rtvDescriptorSize_;
	UINT csuDescriptorSize_;
	bool depthBoundsTestSupported_;

	std::vector<Block::Vertex> blockVertices_;
	std::unique_ptr<VertexBuffer<Block::Vertex>> blockVertex_;
	
	std::vector<my::Rectangle::Vertex> rectangleVertices_;
	std::unique_ptr<VertexBuffer<my::Rectangle::Vertex>> rectangleVertex_;

	std::vector<my::Square::Vertex> squareVertices_;
	std::unique_ptr<VertexBuffer<my::Square::Vertex>> squareVertex_;

	std::vector<UINT> textureVertices_;
	std::unique_ptr<VertexBuffer<UINT>> textureVertex_;

	std::vector<Block::Vertex> cameraVertices_;
	std::unique_ptr<VertexBuffer<Block::Vertex>> cameraVertex_;
	std::unique_ptr<VertexBuffer<UINT>> cameraTextureVertex_;

	int currBlockIndex_ = 1;

	std::vector<Block::Vertex> skyVertices_;
	std::unique_ptr<VertexBuffer<Block::Vertex>> skyVertex_;

	ComPtr<ID3D12Resource> crossHairBuffer_;
	D3D12_VERTEX_BUFFER_VIEW crossHairBufferView_;

	Block* currBlock_;

	void createPipeline();
	void createRootSignature();
	void createDescriptorHeap();
	void createVertex();
	void loadAssets();
	void populateCommandList();

	Camera camera_ = Camera();
	GameTimer gameTimer_;

	POINT center_;

	std::unique_ptr<CSUHeap> csuHeap_ = nullptr;

	BVH* bvh_;
	std::vector<Block*> blocks_;
	// std::set<Block*> blocks_;

	bool outOfControl_ = false;

	std::unique_ptr<DirectX::Keyboard> keyboard_;
	std::unique_ptr<DirectX::Mouse> mouse_;

	DirectX::Mouse::ButtonStateTracker mouseTracker_;
	DirectX::Keyboard::KeyboardStateTracker keyboardTracker_;

	std::unique_ptr<DirectX::AudioEngine> audioEngine_;
	std::unique_ptr<SoundEffect> moveSoundEffect_;
	std::unique_ptr<SoundEffect> soundEffect_;
	std::unique_ptr<SoundEffectInstance> soundEffectInstance_;
};
