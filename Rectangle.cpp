#include "Rectangle.h"

my::Rectangle::Rectangle(DirectX::XMFLOAT2 position, float width, float height) : position_(position), width_(width), height_(height), x_(position.x), y_(position.y), widthDiv2_(width / 2.0f), heightDiv2_(height / 2.0f) {
	initialize();
}

my::Rectangle::Rectangle(const Rectangle& rhs, DirectX::XMFLOAT2 offset) : Rectangle(DirectX::XMFLOAT2(rhs.position_.x + offset.x, rhs.position_.y + offset.y), rhs.width_, rhs.height_) {

}


void my::Rectangle::initialize() {
	vertices_ = {
		{x_ - widthDiv2_, y_ + heightDiv2_, -widthDiv2_, heightDiv2_},
		{x_ + widthDiv2_, y_ + heightDiv2_, widthDiv2_, heightDiv2_},
		{x_ - widthDiv2_, y_ - heightDiv2_, -widthDiv2_, -heightDiv2_},

		{x_ - widthDiv2_, y_ - heightDiv2_, -widthDiv2_, -heightDiv2_},
		{x_ + widthDiv2_, y_ + heightDiv2_, widthDiv2_, heightDiv2_},
		{x_ + widthDiv2_, y_ - heightDiv2_, widthDiv2_, -heightDiv2_},
	};
}