#include "Camera.h"

#include <windowsx.h>
#include <WinUser.h>

#include "Help.h"

using namespace DirectX;
using namespace physx;

DirectX::XMMATRIX Camera::getViewMatrix() {
	return XMMatrixLookAtRH(XMLoadFloat3(&position_), XMLoadFloat3(&position_) + XMLoadFloat3(&front_), XMLoadFloat3(&up_));
}

DirectX::XMMATRIX Camera::getProjectionMatrix(float width, float height, float n, float f) {
	return XMMatrixPerspectiveFovRH(XMConvertToRadians(zoom_), width / (height), n, f);
}

DirectX::XMMATRIX Camera::getOrthogonalMatrix(float width, float height, float n, float f) {
	return XMMatrixOrthographicRH(width, height, n, f);
}


void Camera::onMouseMove(int x, int y) {
	yaw_ += static_cast<float>(x) * mouseSensitivity_;
	pitch_ -= static_cast<float>(y) * mouseSensitivity_;

	pitch_ = Help::clamp(pitch_, -89.0f, 89.0f);
}

void Camera::update(DirectX::Keyboard::State state, DirectX::Keyboard::KeyboardStateTracker tracker) {
	PxVec3 front(levelFront_.x, levelFront_.y, levelFront_.z);
	PxVec3 up(worldUp_.x, worldUp_.y, worldUp_.z);
	PxVec3 right(right_.x, right_.y, right_.z);

	float value = 3.0f;

	PxVec3 speed(0.0f, 0.0f, 0.0f);

	auto currVelocity = body_->getLinearVelocity();
	
	if (tracker.IsKeyPressed(Keyboard::F5)) {
		thirdPerson_ ^= 1;
	}
	if (state.W) {
		speed += front * value;
	}
	if (state.S) {
		speed += -front * value;
	}
	if (state.D) {
		speed += right * value;
	}
	if (state.A) {
		speed += -right * value;
	}
	speed.y = currVelocity.y;
	if (tracker.IsKeyPressed(Keyboard::Space)) {
		if (!sky_) {
			speed += up * value * 1.7f;
		}
		// if (Help::equal(currVelocity.y, 0.0f)) {
			// speed += up * value * 1.7f;
		// }
	}
	
	if (state.W || state.S || state.D || state.A || (tracker.pressed.Space && !sky_)) {
		body_->setLinearVelocity(speed);
	} else {
		currVelocity.x = 0.0f;
		currVelocity.z = 0.0f;
		body_->setLinearVelocity(currVelocity);
	}
	
	// auto move = PxVec3(0.0f, 0.0f, 0.0f);
	// if (state.W) {
	// 	move += front * value;
	// }
	// if (state.S) {
	// 	move += -front * value;
	// }
	// if (state.D) {
	// 	move += right * value;
	// }
	// if (state.A) {
	// 	move += -right * value;
	// }
	// if (!sky_ && tracker.IsKeyPressed(Keyboard::Space)) {
	// 	rise_ = true;
	// }
	//
	// static float riseLength = 0.0f;
	// if (rise_) {
	// 	riseLength += up.y * value;
	// }
	//
	//
	// if (rise_) {
	// 	move.y += up.y * value;
	// 	if (riseLength > 1.0f) {
	// 		rise_ = false;
	// 		riseLength = 0.0f;
	// 	}
	// }
	// if (!rise_)
	// 	move.y -= up.y * value / 0.9f;
	//
	// if (xStop_ && move.x > 0.0f) move.x = 0.0f;
	// if (negXStop_ && move.x < 0.0f) move.x = 0.0f;
	// if (yStop_ && move.y > 0.0f) move.y = 0.0f;
	// if (negYStop_ && move.y < 0.0f) move.y = 0.0f;
	// if (zStop_ && move.z > 0.0f) move.z = 0.0f;
	// if (negZStop_ && move.z < 0.0f) move.z = 0.0f;
	//
	// body_->setGlobalPose(PxTransform(body_->getGlobalPose().p + move));
}


void Camera::update(float deltaTime) {
	XMFLOAT3 front;
	front.x = cosf(XMConvertToRadians(yaw_)) * cosf(XMConvertToRadians(pitch_));
	front.y = sinf(XMConvertToRadians(pitch_));
	front.z = sinf(XMConvertToRadians(yaw_)) * cosf(XMConvertToRadians(pitch_));
	XMStoreFloat3(&front_, XMVector3Normalize(XMLoadFloat3(&front)));
	
	XMStoreFloat3(&right_, XMVector3Normalize(XMVector3Cross(XMLoadFloat3(&front_), XMLoadFloat3(&worldUp_))));
	XMStoreFloat3(&up_, XMVector3Normalize(XMVector3Cross(XMLoadFloat3(&right_), XMLoadFloat3(&front_))));

	auto tmp = front_;
	tmp.y = 0.0f;
	XMStoreFloat3(&levelFront_, XMVector3Normalize(XMLoadFloat3(&tmp)));

	auto pose = body_->getGlobalPose().p;
	XMFLOAT3 position(pose.x, pose.y, pose.z);
	position_ = position;
	position_.y += 0.25f;

	float value = 3.0f;
	auto back = XMFLOAT3(-front_.x * value, front_.y * value, -front_.z * value);

	if (thirdPerson_) {
		position_.x += back.x;
		position_.y += 3.0f;
		position_.z += back.z;	
	}
}

void Camera::setCenter(int x, int y) {
	centerX_ = x;
	centerY_ = y;
}
