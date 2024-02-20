#pragma once
#include <DirectXMath.h>

struct Ray {
public:
	Ray(DirectX::XMFLOAT3 o, DirectX::XMFLOAT3 d) : origin_(DirectX::XMLoadFloat3(&o)), direction_(DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&d))) {
		DirectX::XMStoreFloat3(&origin3_, origin_);
		DirectX::XMStoreFloat3(&direction3_, direction_);
	}
	Ray(DirectX::XMVECTOR o, DirectX::XMVECTOR d) : origin_(o), direction_(DirectX::XMVector3Normalize(d)) {
		DirectX::XMStoreFloat3(&origin3_, origin_);
		DirectX::XMStoreFloat3(&direction3_, direction_);
	}

	DirectX::XMVECTOR origin_{};
	DirectX::XMVECTOR direction_{};
	DirectX::XMFLOAT3 origin3_{};
	DirectX::XMFLOAT3 direction3_{};
};
