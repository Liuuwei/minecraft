#pragma once
#include <d3d12.h>
#include <DirectXMath.h>
#include <intsafe.h>
#include <string>

class Help {
public:
	constexpr static float falseNumber = 9526.0f;

	static UINT calculateConstantBufferSize(UINT size) {
		return (size + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);
	}

	template<typename  T>
	static T clamp(T value, T min, T max) {
		if (value < min) {
			return min;
		} else if (value > max) {
			return max;
		}

		return value;
	}

	static float intersection(DirectX::XMVECTOR origin, DirectX::XMVECTOR direction, DirectX::XMVECTOR point, DirectX::XMVECTOR normal, DirectX::XMVECTOR& ret);
	static float intersection(DirectX::XMVECTOR origin, DirectX::XMVECTOR direction, DirectX::XMVECTOR point, DirectX::XMVECTOR normal, DirectX::XMVECTOR& ret, std::wstring& msg);
	
	static std::wstring toWString(DirectX::XMFLOAT3 v) {
		std::wstring s = L"x: " + std::to_wstring(v.x) + L", y: " + std::to_wstring(v.y) + L", z: " + std::to_wstring(v.z);
		return s;
	}
	static std::wstring toWString(DirectX::XMVECTOR v) {
		std::wstring s = L"x: " + std::to_wstring(DirectX::XMVectorGetX(v)) + L", y: " + std::to_wstring(DirectX::XMVectorGetY(v)) + L", z: " + std::to_wstring(DirectX::XMVectorGetZ(v));
		return s;
	}

	static bool equal(float a, float b) {
		return std::abs(a - b) < 1e-6;
	}

	static bool lessEqual(float a, float b) {
		return a < b || equal(a, b);
	}

	static bool greaterEqual(float a, float b) {
		return a > b || equal(a, b);
	}
};
