#pragma once
#include <DirectXMath.h>
#include <vector>

namespace my
{

class Square {
public: 
	struct Vertex {
		Vertex(float x, float y, float u, float v) : position_(x, y), uv_(u, v) {}
		DirectX::XMFLOAT2 position_;
		DirectX::XMFLOAT2 uv_;
	};
public:
	Square();
	Square& operator=(const Square& rhs) = default;
	Square(const Square& rhs, DirectX::XMFLOAT2 offset);
	Square(DirectX::XMFLOAT2 position);
	Square(DirectX::XMFLOAT2 position, float radius);

	std::vector<Vertex> vertices_;
private:
	void init();

	DirectX::XMFLOAT2 position_;
	float radius_;

	float x_, y_;
};


}