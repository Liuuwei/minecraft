#pragma once

#include <map>
#include <set>

#include "Block.h"
#include "BoundingBox.h"
#include "Ray.h"

struct BVHNode;

class BVH {
public:
	BVH() {}
	BVHNode* buildBVH(const std::vector<Block*>& blocks, int left, int right);
	BVHNode* buildBVH(const std::set<Block*>& blocks);
	BVHNode* root_ = nullptr;

	Block* intersection(Ray ray, BVHNode* node, float& time) const;

private:
	my::BoundingBox joint(const my::BoundingBox& left, const my::BoundingBox& right) const;
};

struct BVHNode {
	my::BoundingBox box_;
	Block* block_ = nullptr;
	BVHNode* left_ = nullptr;
	BVHNode* right_ = nullptr;	
};