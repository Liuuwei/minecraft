#include "Square.h"

void my::Square::init() {
	vertices_ = {
		{x_ - radius_, y_ + radius_, -0.5f, 0.5f},
		{x_ + radius_, y_ + radius_, 0.5f, 0.5f},
		{x_ - radius_, y_ - radius_, -0.5f, -0.5f},

		{x_ - radius_, y_ - radius_, -0.5f, -0.5f},
		{x_ + radius_, y_ + radius_, 0.5f, 0.5f},
		{x_ + radius_, y_ - radius_, 0.5f, -0.5f},
	};
}

my::Square::Square() : Square(DirectX::XMFLOAT2(0.0, 0.0), 0.5) {
	init();	
}

my::Square::Square(DirectX::XMFLOAT2 position) : position_(position), radius_(0.5), x_(position.x), y_(position.y) {
	init();
}

my::Square::Square(DirectX::XMFLOAT2 position, float radius) : position_(position), radius_(radius), x_(position.x), y_(position.y) {
	init();
}

my::Square::Square(const Square& rhs, DirectX::XMFLOAT2 offset) : position_(rhs.position_.x + offset.x, rhs.position_.y + offset.y), radius_(rhs.radius_), x_(position_.x), y_(position_.y) {
	init();	
}
