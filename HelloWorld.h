#pragma once

#include <directxtk12/Model.h>
#include <directxtk12/CommonStates.h>

#include "Animation.h"
#include "Block.h"
#include "BVH.h"
#include "DXSample.h"
#include "Camera.h"
#include "GameTimer.h"
#include "CSUHeap.h"
#include "DSVHeap.h"
#include "RTVHeap.h"
#include "PipelineState.h"
#include "Rectangle.h"
#include "RootSignature.h"
#include "Square.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

class HelloWorld : public DXSample {
public:
	HelloWorld(UINT width, UINT height, std::wstring name);

	void onInit() override;
	void onUpdate(WPARAM wParam, LPARAM lParam) override;
	void onRender() override;
	void onDestroy() override;

	void onKeyDown(WPARAM key) override;

	void onKeyUp(WPARAM key) override;

	void onMouseMovement(LPARAM param) override;

	void onRightButtonDown() override;
	void onLeftButtonDown() override;

	void onMouseWheel(WPARAM wParam) override;

private:
	void show(const std::wstring& msg) {
		SetWindowText(hwnd_, msg.c_str());
	}
	static const UINT frameCount_ = 2;
	static const UINT textureWidth_ = 256;
	static const UINT textureHeight_ = 256;
	static const UINT texturePixelSize = 4;

	static const UINT numTextures_ = 6;

	struct SceneConstantBuffer {
		XMFLOAT4 offset;
		float padding[60];
	};
	static_assert(sizeof(SceneConstantBuffer) % 256 == 0, "Constant Buffer size muse be 256-byte aligned");

	CD3DX12_VIEWPORT viewport_;
	CD3DX12_RECT scissorRect_;
	ComPtr<IDXGIFactory4> factory_;
	ComPtr<IDXGISwapChain3> swapChain_;
	ComPtr<ID3D12Device2> device_;
	ComPtr<ID3D12Resource> depthStencil_;
	ComPtr<ID3D12Resource> renderTargets_[frameCount_];
	ComPtr<ID3D12CommandAllocator> commandAllocator_;
	ComPtr<ID3D12CommandQueue> commandQueue_;
	std::unique_ptr<my::RootSignature> defaultRootSignature_;
	std::unique_ptr<my::RootSignature> rectangleRootSignature_;
	std::unique_ptr<my::RootSignature> squareRootSignature_;
	std::unique_ptr<my::RootSignature> cursorRootSignature_;
	std::unique_ptr<PipelineState> defaultPSO_;
	std::unique_ptr<PipelineState> skyboxPSO_;
	std::unique_ptr<PipelineState> minecraftPSO_;
	std::unique_ptr<PipelineState> boxPSO_;
	std::unique_ptr<PipelineState> rectanglePSO_;
	std::unique_ptr<PipelineState> squarePSO_;
	ComPtr<ID3D12GraphicsCommandList1> commandList_;
	UINT rtvDescriptorSize_;
	UINT csuDescriptorSize_;
	bool depthBoundsTestSupported_;

	std::vector<Block::Vertex> vertices_;
	std::vector<my::Rectangle::Vertex> rectangleVertices_;
	UINT8* rectangleData_;
	ComPtr<ID3D12Resource> rectangleBuffer_;
	D3D12_VERTEX_BUFFER_VIEW rectangleView_;
	std::vector<my::Square::Vertex> squareVertices_;
	UINT8* squareData_;
	ComPtr<ID3D12Resource> squareBuffer_;
	D3D12_VERTEX_BUFFER_VIEW squareView_;
	ComPtr<ID3D12Resource> blockBuffer_;
	D3D12_VERTEX_BUFFER_VIEW blockBufferView_;
	UINT8* blockData_;
	Block* block;
	std::vector<UINT> textureIndex_;
	ComPtr<ID3D12Resource> textureIndexBuffer_;
	UINT8* textureIndexData_;
	UINT currBlockIndex_ = 1;
	D3D12_VERTEX_BUFFER_VIEW textureIndexBufferView_;
	ComPtr<ID3D12Resource> skyBuffer_;
	D3D12_VERTEX_BUFFER_VIEW skyBufferView_;
	UINT8* skyData_;
	Block* skyBlock_;
	ComPtr<ID3D12Resource> crossHairBuffer_;
	D3D12_VERTEX_BUFFER_VIEW crossHairBufferView_;
	ComPtr<ID3D12Resource> constantBuffer_;
	UINT8* cbvDataBegin_;
	UINT8* worldData_;
	ComPtr<ID3D12Resource> worldBuffer_;
	ComPtr<ID3D12Resource> texture_ = nullptr;
	ComPtr<ID3D12Resource> textureUpload_;

	Block* currBlock_;

	UINT frameIndex_;
	UINT frameNumber_;
	HANDLE fenceEvent_;
	ComPtr<ID3D12Fence> fence_;
	UINT64 fenceValue_;

	void createFactory();
	void createDevice();
	void createCommandQueue();
	void createSwapChain();
	void createCommandAllocator();
	void createCommandList();
	void createPipeline();
	void createRootSignature();
	void createDescriptorHeap();
	void createFence();
	void createVertex();
	void loadAssets();
	std::vector<UINT8> generateTextureData(); 
	void populateCommandList();
	void waitForPreviousFrame();

	void prepare();
	void present();
	void clear();

	Camera camera_ = Camera();
	GameTimer gameTimer_;

	POINT center_;

	std::unique_ptr<CSUHeap> csuHeap_ = nullptr;
	std::unique_ptr<RTVHeap> rtvHeap_ = nullptr;
	std::unique_ptr<DSVHeap> dsvHeap_ = nullptr;

	std::unique_ptr<DirectX::GraphicsMemory> graphicsMemory_;
	std::unique_ptr<DirectX::CommonStates> states_;
	std::unique_ptr<DirectX::Model> model_;
	std::unique_ptr<DirectX::EffectFactory> effectFactory_;
	std::unique_ptr<DirectX::EffectTextureFactory> modelResource_;
	Model::EffectCollection modelEffects_;

	DirectX::ModelBone::TransformArray drawBones_;
	DirectX::ModelBone::TransformArray animBones_;

	DX::AnimationCMO animation_;

	XMFLOAT3 move;

	BVH* bvh_;
	std::vector<Block*> blocks_;
	// std::set<Block*> blocks_;
};
