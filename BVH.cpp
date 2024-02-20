#include "BVH.h"

#include <d3d12.h>

BVHNode* BVH::buildBVH(const std::vector<Block*>& blocks, int leftIndex, int rightIndex) {
	if (leftIndex > rightIndex) {
		return nullptr;
	}

	if (leftIndex == rightIndex) {
		auto node = new BVHNode;
		node->block_ = blocks[leftIndex];
		node->box_ = blocks[leftIndex]->box_;
		node->left_ = nullptr;
		node->right_ = nullptr;
		return node;
	}

	auto node = new BVHNode;

	int mid = (leftIndex + rightIndex) / 2;

	auto left = buildBVH(blocks, leftIndex, mid);
	auto right = buildBVH(blocks, mid + 1, rightIndex);
	my::BoundingBox box;
	if (left && right) {
		box = joint(left->box_, right->box_);
	} else if (left) {
		box = left->box_;
	} else if (right) {
		box = right->box_;
	}

	node->box_ = box;
	node->left_ = left;
	node->right_ = right;
	if (!left && !right) {
		node->block_ = blocks[mid];
	}

	return node;
}

BVHNode* BVH::buildBVH(const std::set<Block*>& blocks) {
	if (blocks.size() == 0) {
		return nullptr;
	}
	if (blocks.size() == 1) {
		auto node = new BVHNode;
		node->block_ = *blocks.begin();
		node->box_ = (*blocks.begin())->box_;
		node->left_ = nullptr;
		node->right_ = nullptr;

		return node;
	}

	int size = blocks.size() / 2;
	std::set<Block*> leftBlocks, rightBlocks;
	for (auto& block : blocks) {
		if (size > 0) {
			size--;
			leftBlocks.insert(block);
		} else {
			rightBlocks.insert(block);
		}
	}

	auto left = buildBVH(leftBlocks);
	auto right = buildBVH(rightBlocks);

	auto node = new BVHNode;
	
	my::BoundingBox box;
	if (left && right) {
		box = joint(left->box_, right->box_);
	} else if (left) {
		box = left->box_;
	} else if (right) {
		box = right->box_;
	}
	node->box_ = box;
	node->left_ = left;
	node->right_ = right;

	return node;
}


my::BoundingBox BVH::joint(const my::BoundingBox& left, const my::BoundingBox& right) const {
	my::BoundingBox b;
	DirectX::XMFLOAT3 mi(min(left.min_.x, right.min_.x), min(left.min_.y, right.min_.y), min(left.min_.z, right.min_.z));
	DirectX::XMFLOAT3 ma(max(left.max_.x, right.max_.x), max(left.max_.y, right.max_.y), max(left.max_.z, right.max_.z));
	b.min_ = mi;
	b.max_ = ma;

	return b;
}

Block* BVH::intersection(Ray ray, BVHNode* node, float& time) const {
	if (node == nullptr) {
		return nullptr;
	}

	float xMin = (node->box_.min_.x - ray.origin3_.x) / ray.direction3_.x;
	float xMax = (node->box_.max_.x - ray.origin3_.x) / ray.direction3_.x;
	if (xMin > xMax) {
		std::swap(xMin, xMax);
	}

	float yMin = (node->box_.min_.y - ray.origin3_.y) / ray.direction3_.y;
	float yMax = (node->box_.max_.y - ray.origin3_.y) / ray.direction3_.y;
	if (yMin > yMax) {
		std::swap(yMin, yMax);
	}

	float zMin = (node->box_.min_.z - ray.origin3_.z) / ray.direction3_.z;
	float zMax = (node->box_.max_.z - ray.origin3_.z) / ray.direction3_.z;
	if (zMin > zMax) {
		std::swap(zMin, zMax);
	}

	float tMin = 1e9, tMax = -1e9;

	tMin = max(xMin, max(yMin, zMin));
	tMax = min(xMax, min(yMax, zMax));

	// time = tMin >= 0.0f ? tMin : tMax; // Bug ? 

	if (tMin > tMax || tMax < 0.0f) {
		return nullptr;
	}

	if (!node->left_ && !node->right_) {
		time = tMin;
		return node->block_;
	}

	float leftTime = 1e9, rightTime = 1e9;
	auto left = intersection(ray, node->left_, leftTime);
	auto right = intersection(ray, node->right_, rightTime);

	time = min(leftTime, rightTime);

	if (left && right) {
		return leftTime < rightTime ? left : right;
	}

	return left ? left : right;
}
