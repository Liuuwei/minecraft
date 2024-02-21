#pragma once
#include <array>
#include <complex>
#include <DirectXMath.h>
#include <vector>

#include "Animation.h"
#include "BoundingBox.h"
#include "Help.h"
#include "CSUHeap.h"

class Block {
public:
	struct Vertex {
		Vertex(float x, float y, float z, float r, float g, float b) : position_(x, y, z), uv_(r, g, b) {}
		Vertex& operator=(const Vertex& rhs) = default;
		DirectX::XMFLOAT3 position_;
		DirectX::XMFLOAT3 uv_;
	};

	enum Face {
		FRONT = 0,
		BACK,
		UP,
		DOWN,
		RIGHT,
		LEFT,
		NONE, 
	};

public:
	Block(UINT index = 1);
	Block(DirectX::XMFLOAT3 position, UINT index = 1);
	Block(DirectX::XMFLOAT3 position, float radius, UINT index = 1);
	Block(const Block& rhs, DirectX::XMFLOAT3 offset, UINT index = 1);

	Block& operator=(const Block& rhs) = default;

	bool operator<(const Block& rhs) const {
		return box_ < rhs.box_;
	}

	static void setDevice(ID3D12Device2* device) { 
		device_ = device;
	}

	void initialize();

	DirectX::XMFLOAT3 position() const { return position_; }
	float x() const { return x_; }
	float y() const { return y_; }
	float z() const { return z_; }
	float radius() const { return radius_; }

	std::vector<Vertex> vertices() const { return vertices_; }
	
	std::vector<Vertex>::const_iterator begin() const { return vertices_.begin(); }
	std::vector<Vertex>::const_iterator end() const { return vertices_.end(); }
	
	DirectX::XMMATRIX model() const { return DirectX::XMLoadFloat4x4(&model_); }
	DirectX::XMMATRIX view() const { return DirectX::XMLoadFloat4x4(&view_); }
	DirectX::XMMATRIX projection() const { return DirectX::XMLoadFloat4x4(&projection_); }
	DirectX::XMMATRIX mvp() const { return model() * view() * projection(); }

	float lengthTo(DirectX::XMFLOAT3 target) const;

	std::vector<std::vector<DirectX::XMFLOAT3>> getFaceVertices(Face face) const;

	Face selectedFace(DirectX::XMFLOAT3 pOrigin, DirectX::XMFLOAT3 pDirection);
	Face selectedFace(DirectX::XMFLOAT3 pOrigin, DirectX::XMFLOAT3 pDirection, std::wstring& msg);
private:
	static ID3D12Device2* device_;

	DirectX::XMFLOAT3  position_;
	float x_;
	float y_;
	float z_;
	float radius_;

public:
	std::vector<Vertex> vertices_;
	std::vector<UINT> textureIndex_;
	UINT index_;

private:
	bool selected_ = false;

	bool lessEqual(float a, float b) const {
		return Help::lessEqual(a, b);
	}

	bool greaterEqual(float a, float b) const {
		return Help::greaterEqual(a, b);
	}

	float intersectionTime(DirectX::XMVECTOR origin, DirectX::XMVECTOR direction, Face face) const;
	float intersectionTime(DirectX::XMVECTOR origin, DirectX::XMVECTOR direction, Face face, std::wstring& msg) const;
public:
	DirectX::XMFLOAT4X4 model_;
	DirectX::XMFLOAT4X4 view_;
	DirectX::XMFLOAT4X4 projection_;

	my::BoundingBox box_;
};
