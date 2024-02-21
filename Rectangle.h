#pragma once
#include <DirectXMath.h>
#include <vector>

namespace my
{

class Rectangle {
public:
	struct Vertex {
		Vertex(float x, float y, float u, float v) : position_(x, y), uv_(u, v) {}
		DirectX::XMFLOAT2 position_;
		DirectX::XMFLOAT2 uv_;
	};
public:
	Rectangle(DirectX::XMFLOAT2 position, float width, float height);
	Rectangle(const Rectangle& rhs, DirectX::XMFLOAT2 offset);

	std::vector<Vertex> vertices_;
private:
	void initialize();
	DirectX::XMFLOAT2 position_;
	float x_, y_;
	float width_, height_;
	float widthDiv2_, heightDiv2_;
};

}