#include "Block.h"

#include <intsafe.h>
#include <iso646.h>
#include <tuple>

#include "Help.h"

using namespace DirectX;

ID3D12Device2* Block::device_ = nullptr;

void Block::initialize() {
	vertices_ = {
		// 正面
		{x_ - radius_, y_ + radius_, z_ + radius_, -0.5f, 0.5f, 0.5f},
		{x_ + radius_, y_ + radius_, z_ + radius_, 0.5f, 0.5f, 0.5f},
		{x_ - radius_, y_ - radius_, z_ + radius_, -0.5f, -0.5f, 0.5f},

		{x_ - radius_, y_ - radius_, z_ + radius_, -0.5f, -0.5f, 0.5f},
		{x_ + radius_, y_ + radius_, z_ + radius_, 0.5f, 0.5f, 0.5f},
		{x_ + radius_, y_ - radius_, z_ + radius_, 0.5f, -0.5f, 0.5f},

		// 背面
		{x_ + radius_, y_ + radius_, z_ - radius_, 0.5f, 0.5f, -0.5f},
		{x_ - radius_, y_ + radius_, z_ - radius_, -0.5f, 0.5f, -0.5f},
		{x_ + radius_, y_ - radius_, z_ - radius_, 0.5f, -0.5f, -0.5f},

		{x_ + radius_, y_ - radius_, z_ - radius_, 0.5f, -0.5f, -0.5f},
		{x_ - radius_, y_ + radius_, z_ - radius_, -0.5f, 0.5f, -0.5f},
		{x_ - radius_, y_ - radius_, z_ - radius_, -0.5f, -0.5f, -0.5f},

		// 上面
		{x_ - radius_, y_ + radius_, z_ - radius_, -0.5f, 0.5f, -0.5f},
		{x_ + radius_, y_ + radius_, z_ - radius_, 0.5f, 0.5f, -0.5f},
		{x_ - radius_, y_ + radius_, z_ + radius_, -0.5f, 0.5f, 0.5f},

		{x_ - radius_, y_ + radius_, z_ + radius_, -0.5f, 0.5f, 0.5f},
		{x_ + radius_, y_ + radius_, z_ - radius_, 0.5f, 0.5f, -0.5f},
		{x_ + radius_, y_ + radius_, z_ + radius_, 0.5f, 0.5f, 0.5f},

		// 底面
		{x_ - radius_, y_ - radius_, z_ + radius_, -0.5f, -0.5f, 0.5f},
		{x_ + radius_, y_ - radius_, z_ + radius_, 0.5f, -0.5f, 0.5f},
		{x_ - radius_, y_ - radius_, z_ - radius_, -0.5f, -0.5f, -0.5f},

		{x_ - radius_, y_ - radius_, z_ - radius_, -0.5f, -0.5f, -0.5f},
		{x_ + radius_, y_ - radius_, z_ + radius_, +0.5f, -0.5f, +0.5f},
		{x_ + radius_, y_ - radius_, z_ - radius_, +0.5f, -0.5f, -0.5f},

		// 右面
		{x_ + radius_, y_ + radius_, z_ + radius_, 0.5f, 0.5f, 0.5f},
		{x_ + radius_, y_ + radius_, z_ - radius_, 0.5f, 0.5f, -0.5f},
		{x_ + radius_, y_ - radius_, z_ + radius_, 0.5f, -0.5f, 0.5f},

		{x_ + radius_, y_ - radius_, z_ + radius_, 0.5f, -0.5f, 0.5f},
		{x_ + radius_, y_ + radius_, z_ - radius_, 0.5f, 0.5f, -0.5f},
		{x_ + radius_, y_ - radius_, z_ - radius_, 0.5f, -0.5f, -0.5f},

		// 左面
		{x_ - radius_, y_ + radius_, z_ - radius_, -0.5f, 0.5f, -0.5f},
		{x_ - radius_, y_ + radius_, z_ + radius_, -0.5f, 0.5f, 0.5f},
		{x_ - radius_, y_ - radius_, z_ - radius_, -0.5f, -0.5f, -0.5f},

		{x_ - radius_, y_ - radius_, z_ - radius_, -0.5f, -0.5f, -0.5f},
		{x_ - radius_, y_ + radius_, z_ + radius_, -0.5f, 0.5f, 0.5f},
		{x_ - radius_, y_ - radius_, z_ + radius_, -0.5f, -0.5f, 0.5f},
	};

	auto minPoint = DirectX::XMFLOAT3(x_ - radius_, y_ - radius_, z_ - radius_);
	auto maxPoint = DirectX::XMFLOAT3(x_ + radius_, y_ + radius_, z_ + radius_);
	box_ = my::BoundingBox(minPoint, maxPoint);

	XMStoreFloat4x4(&model_, XMMatrixTranslationFromVector(XMLoadFloat3(&XMFLOAT3(x_, y_, z_))));

	textureIndex_ = std::vector<UINT>(36, index_);
}

Block::Block(UINT index) : Block(DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), 0.5f, index) {
	initialize();
}

Block::Block(DirectX::XMFLOAT3 position, UINT index) : position_(position), x_(position.x), y_(position.y), z_(position.z), radius_(0.5f), index_(index) {
	initialize();
}

Block::Block(DirectX::XMFLOAT3 position, float radius, UINT index) : position_(position), x_(position_.x), y_(position_.y), z_(position_.z), radius_(radius), index_(index) {
	initialize();
}

Block::Block(const Block& rhs, DirectX::XMFLOAT3 offset, UINT index) : position_(DirectX::XMFLOAT3(rhs.position().x + offset.x, rhs.position().y + offset.y, rhs.position().z + offset.z)), x_(position_.x), y_(position_.y), z_(position_.z), radius_(rhs.radius()), index_(index) {
	initialize();
}

float Block::lengthTo(DirectX::XMFLOAT3 target) const {
	auto length = std::pow(x_ - target.x,  2) + std::pow(y_ - target.y, 2) + std::pow(z_ - target.z, 2);
	return std::sqrt(length);
}

std::vector<std::vector<DirectX::XMFLOAT3>> Block::getFaceVertices(Face face) const {
	std::vector<XMFLOAT3> first, second;
	switch (face) {
	case FRONT: 
		first = {{x_ - radius_, y_ + radius_, z_ + radius_}, {x_ + radius_, y_ + radius_, z_ + radius_}, {x_ - radius_, y_ - radius_, z_ + radius_}};
		second = {{x_ - radius_, y_ - radius_, z_ + radius_}, {x_ + radius_, y_ + radius_, z_ + radius_}, {x_ + radius_, y_ - radius_, z_ + radius_}};
		break;
	case BACK:
		first = {{x_ + radius_, y_ + radius_, z_ - radius_}, {x_ - radius_, y_ + radius_, z_ - radius_}, {x_ + radius_, y_ - radius_, z_ - radius_}};
		second = {{x_ + radius_, y_ - radius_, z_ - radius_}, {x_ - radius_, y_ + radius_, z_ - radius_}, {x_ - radius_, y_ - radius_, z_ - radius_}};
		break;
	case UP:
		first = {{x_ - radius_, y_ + radius_, z_ - radius_}, {x_ + radius_, y_ + radius_, z_ - radius_}, {x_ - radius_, y_ + radius_, z_ + radius_}};
		second = {{x_ - radius_, y_ + radius_, z_ + radius_}, {x_ + radius_, y_ + radius_, z_ - radius_}, {x_ + radius_, y_ + radius_, z_ + radius_}};
		break;
	case DOWN:
		first = {{x_ - radius_, y_ - radius_, z_ + radius_}, {x_ + radius_, y_ - radius_, z_ + radius_}, {x_ - radius_, y_ - radius_, z_ - radius_}};
		second = {{x_ - radius_, y_ - radius_, z_ - radius_}, {x_ + radius_, y_ - radius_, z_ + radius_}, {x_ + radius_, y_ - radius_, z_ - radius_}};
		break;
	case RIGHT:
		first = {{x_ + radius_, y_ + radius_, z_ + radius_}, {x_ + radius_, y_ + radius_, z_ - radius_}, {x_ + radius_, y_ - radius_, z_ + radius_}};
		second = {{x_ + radius_, y_ - radius_, z_ + radius_}, {x_ + radius_, y_ + radius_, z_ - radius_}, {x_ + radius_, y_ - radius_, z_ - radius_}};
		break;
	case LEFT:
		first = {{x_ - radius_, y_ + radius_, z_ - radius_}, {x_ - radius_, y_ + radius_, z_ + radius_}, {x_ - radius_, y_ - radius_, z_ - radius_}};
		second = {{x_ - radius_, y_ - radius_, z_ - radius_}, {x_ - radius_, y_ + radius_, z_ + radius_}, {x_ - radius_, y_ - radius_, z_ + radius_}};
		break;
	default:
		break;
	}

	return {first, second};
}


Block::Face Block::selectedFace(DirectX::XMFLOAT3 pOrigin, DirectX::XMFLOAT3 pDirection) {
	auto origin = XMLoadFloat3(&pOrigin);
	auto direction = XMVector3Normalize(XMLoadFloat3(&pDirection));

	Face face = NONE;
    float time = FLT_MAX;
	for (int i = 0; i <= LEFT; i++) {
		auto f = static_cast<Face>(i);
		auto t = intersectionTime(origin, direction, f);
		if (std::abs(t - Help::falseNumber) > 1e-8 && t < time) {
			time = t;
			face = f;
		}
	}

	return face;
}

Block::Face Block::selectedFace(DirectX::XMFLOAT3 pOrigin, DirectX::XMFLOAT3 pDirection, std::wstring& msg) {
	auto origin = XMLoadFloat3(&pOrigin);
	auto direction = XMVector3Normalize(XMLoadFloat3(&pDirection));

	Face face = NONE;
    float time = FLT_MAX;
	for (int i = 0; i <= LEFT; i++) {
		auto f = static_cast<Face>(i);
		float t = intersectionTime(origin, direction, f, msg);
		if (std::abs(t - Help::falseNumber) > 1e-8 && t < time) {
			time = t;
			face = f;
		}
	}

	return face;
}

float Block::intersectionTime(DirectX::XMVECTOR origin, DirectX::XMVECTOR direction, Face face) const {
	auto faceVertices = getFaceVertices(face);
	for (auto& vertices : faceVertices) {
		auto a = XMLoadFloat3(&vertices[0]), b = XMLoadFloat3(&vertices[1]), c = XMLoadFloat3(&vertices[2]);
		auto ab = b - a, bc = c - b, ca = a - c, ac = c - a;
		auto normal = XMVector3Normalize(XMVector3Cross(ab, ac));
		XMVECTOR point;
		float time = Help::intersection(origin, direction, a, normal, point);
		if (std::abs(time - Help::falseNumber) > 1e-8) {
			auto ap = point - a, bp = point - b, cp = point - c;
			XMFLOAT3 ab_, ap_, bc_, bp_, ca_, cp_;
			XMStoreFloat3(&ab_, ab); XMStoreFloat3(&ap_, ap); XMStoreFloat3(&bc_, bc); XMStoreFloat3(&bp_, bp); XMStoreFloat3(&ca_, ca); XMStoreFloat3(&cp_, cp);
			auto _1 = XMVector3Cross(ab, ap);
			auto _2 = XMVector3Cross(bc, bp);
			auto _3 = XMVector3Cross(ca, cp);
			auto x1 = XMVectorGetX(_1), y1 = XMVectorGetY(_1), z1 = XMVectorGetZ(_1);
			auto x2 = XMVectorGetX(_2), y2 = XMVectorGetY(_2), z2 = XMVectorGetZ(_2);
			auto x3 = XMVectorGetX(_3), y3 = XMVectorGetY(_3), z3 = XMVectorGetZ(_3);

			if (((greaterEqual(x1, 0.0f) && greaterEqual(x2, 0.0f) && greaterEqual(x3, 0.0f)) || (lessEqual(x1, 0.0f) && lessEqual(x2, 0.0f) && lessEqual(x3, 0.0f))) && ((greaterEqual(y1, 0.0f) && greaterEqual(y2, 0.0f) && greaterEqual(y3, 0.0f)) || (lessEqual(y1, 0.0f) && lessEqual(y2, 0.0f) && lessEqual(y3, 0.0f))) && ((greaterEqual(z1, 0.0f) && greaterEqual(z2, 0.0f) && greaterEqual(z3, 0.0f)) || (lessEqual(z1, 0.0f) && lessEqual(z2, 0.0f) && lessEqual(z3, 0.0f)))) {
				return time;
			}
		}
	}

	return Help::falseNumber;
}

float Block::intersectionTime(DirectX::XMVECTOR origin, DirectX::XMVECTOR direction, Face face, std::wstring& msg) const {
	const float zero = 1e-9;
	auto faceVertices = getFaceVertices(face);
	for (auto& vertices : faceVertices) {
		auto a = XMLoadFloat3(&vertices[0]), b = XMLoadFloat3(&vertices[1]), c = XMLoadFloat3(&vertices[2]);
		auto ab = b - a, bc = c - b, ca = a - c, ac = c - a;
		auto normal = XMVector3Normalize(XMVector3Cross(ab, ac));
		XMVECTOR point;
		float time = Help::intersection(origin, direction, a, normal, point, msg);
		if (std::abs(time - Help::falseNumber) > 1e-8) {
			auto ap = point - a, bp = point - b, cp = point - c;
			XMFLOAT3 ab_, ap_, bc_, bp_, ca_, cp_;
			XMStoreFloat3(&ab_, ab); XMStoreFloat3(&ap_, ap); XMStoreFloat3(&bc_, bc); XMStoreFloat3(&bp_, bp); XMStoreFloat3(&ca_, ca); XMStoreFloat3(&cp_, cp);
			auto _1 = XMVector3Cross(ab, ap);
			auto _2 = XMVector3Cross(bc, bp);
			auto _3 = XMVector3Cross(ca, cp);
			auto x1 = XMVectorGetX(_1), y1 = XMVectorGetY(_1), z1 = XMVectorGetZ(_1);
			auto x2 = XMVectorGetX(_2), y2 = XMVectorGetY(_2), z2 = XMVectorGetZ(_2);
			auto x3 = XMVectorGetX(_3), y3 = XMVectorGetY(_3), z3 = XMVectorGetZ(_3);

			if (((greaterEqual(x1, 0.0f) && greaterEqual(x2, 0.0f) && greaterEqual(x3, 0.0f)) || (lessEqual(x1, 0.0f) && lessEqual(x2, 0.0f) && lessEqual(x3, 0.0f))) && ((greaterEqual(y1, 0.0f) && greaterEqual(y2, 0.0f) && greaterEqual(y3, 0.0f)) || (lessEqual(y1, 0.0f) && lessEqual(y2, 0.0f) && lessEqual(y3, 0.0f))) && ((greaterEqual(z1, 0.0f) && greaterEqual(z2, 0.0f) && greaterEqual(z3, 0.0f)) || (lessEqual(z1, 0.0f) && lessEqual(z2, 0.0f) && lessEqual(z3, 0.0f)))) {
				return time;
			}
		}

	}

	return Help::falseNumber;
}