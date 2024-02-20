#pragma once
#include <DirectXMath.h>

namespace my
{

class BoundingBox {
public:
	BoundingBox() : min_(0, 0, 0), max_(0, 0, 0), center_(0, 0, 0) {}
	BoundingBox(DirectX::XMFLOAT3 p) : min_(p), max_(p), center_(p.x / 2.0f, p.y / 2.0f, p.z / 2.0f) {}
	BoundingBox(DirectX::XMFLOAT3 min, DirectX::XMFLOAT3 max) : min_(min), max_(max), center_((min_.x + max_.x) / 2.0f, (min_.y + max_.y) / 2.0f, (min_.z + max_.z) / 2.0f) {}

	BoundingBox& operator=(const BoundingBox& rhs) = default;

	bool operator<(const BoundingBox& rhs) const {
		if (center_.x != rhs.center_.x) {
			return center_.x < rhs.center_.x;
		}
		if (center_.y != rhs.center_.y) {
			return center_.y < rhs.center_.y;
		}
		if (center_.z != rhs.center_.z) {
			return center_.z < rhs.center_.z;
		}

		return true;
	}

public:
	DirectX::XMFLOAT3 min_;
	DirectX::XMFLOAT3 max_;

	DirectX::XMFLOAT3 center_;
};

}