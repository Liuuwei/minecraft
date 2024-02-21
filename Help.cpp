#include "Help.h"

#include <string>

using namespace DirectX;

float Help::intersection(DirectX::XMVECTOR origin, DirectX::XMVECTOR direction, DirectX::XMVECTOR point, DirectX::XMVECTOR normal, DirectX::XMVECTOR& ret) {
	XMFLOAT3 n, p, o, d;
	XMStoreFloat3(&n, normal);
	XMStoreFloat3(&p, point);
	XMStoreFloat3(&o, origin);
	XMStoreFloat3(&d, direction);
	auto a = XMVectorGetX(XMVector3Dot(normal, point - origin));
	auto b = XMVectorGetX(XMVector3Dot(direction, normal));
	if (b <= 0.0f) {
		return Help::falseNumber;
	}

	float time = a / b;

	if (time < 0.0f) {
		return Help::falseNumber;
	}
	ret = origin + direction * time;
	return time;
}

float Help::intersection(DirectX::XMVECTOR origin, DirectX::XMVECTOR direction, DirectX::XMVECTOR point, DirectX::XMVECTOR normal, DirectX::XMVECTOR& ret, std::wstring& msg) {
	XMFLOAT3 n, p, o, d;
	XMStoreFloat3(&n, normal);
	XMStoreFloat3(&p, point);
	XMStoreFloat3(&o, origin);
	XMStoreFloat3(&d, direction);

	auto a = XMVectorGetX(XMVector3Dot(normal, point - origin));
	auto b = XMVectorGetX(XMVector3Dot(direction, normal));

	if (b <= 0.0f) {
		return Help::falseNumber;
	}

	float time = a / b;

	if (time < 0.0f) {
		return Help::falseNumber;
	}
	ret = origin + direction * time;
	return time;
}

float Help::distance_ = 8.0f;