#include "Camera.h"

#include <windowsx.h>
#include <WinUser.h>

#include "Help.h"

using namespace DirectX;

DirectX::XMMATRIX Camera::getViewMatrix() {
	return XMMatrixLookAtRH(XMLoadFloat3(&position_), XMLoadFloat3(&position_) + XMLoadFloat3(&front_), XMLoadFloat3(&up_));
}

DirectX::XMMATRIX Camera::getProjectionMatrix(float width, float height, float n, float f) {
	return XMMatrixPerspectiveFovRH(XMConvertToRadians(zoom_), width / (height), n, f);
}

DirectX::XMMATRIX Camera::getOrthogonalMatrix(float width, float height, float n, float f) {
	return XMMatrixOrthographicRH(width, height, n, f);
}


void Camera::onKeyDown(WPARAM key) {
	switch (key) {
	case 'W':
		keys_.FORWARD = true;
		break;
	case 'S':
		keys_.BACKWARD = true;
		break;
	case 'D':
		keys_.RIGHT = true;
		break;
	case 'A':
		keys_.LEFT = true;
		break;
	default:
		break;
	}
}


void Camera::onKeyUp(WPARAM key) {
	switch (key) {
	case 'W':
		keys_.FORWARD = false;
		break;
	case 'S':
		keys_.BACKWARD = false;
		break;
	case 'D':
		keys_.RIGHT = false;
		break;
	case 'A':
		keys_.LEFT = false;
		break;
	default:
		break;
	}
}

void Camera::onMouseMovement(LPARAM param) {
	POINT point;
	GetCursorPos(&point);

	int deltaX = point.x - centerX_;
	int deltaY = point.y - centerY_;

	yaw_ += static_cast<float>(deltaX) * mouseSensitivity_;
	pitch_ -= static_cast<float>(deltaY) * mouseSensitivity_;

	pitch_ = Help::clamp(pitch_, -89.0f, 89.0f);
}

void Camera::update(float deltaTime) {
	XMFLOAT3 front;
	front.x = cosf(XMConvertToRadians(yaw_)) * cosf(XMConvertToRadians(pitch_));
	front.y = sinf(XMConvertToRadians(pitch_));
	front.z = sinf(XMConvertToRadians(yaw_)) * cosf(XMConvertToRadians(pitch_));
	XMStoreFloat3(&front_, XMVector3Normalize(XMLoadFloat3(&front)));
	
	XMStoreFloat3(&right_, XMVector3Cross(XMLoadFloat3(&front_), XMLoadFloat3(&worldUp_)));
	XMStoreFloat3(&up_, XMVector3Cross(XMLoadFloat3(&right_), XMLoadFloat3(&front_)));

	float velocity = movementSpeed_ * deltaTime;
	if (keys_.FORWARD) 
		XMStoreFloat3(&position_, (XMLoadFloat3(&position_) + XMLoadFloat3(&front_) * velocity));
	if (keys_.BACKWARD)
		XMStoreFloat3(&position_, (XMLoadFloat3(&position_) - XMLoadFloat3(&front_) * velocity));
	if (keys_.LEFT)
		XMStoreFloat3(&position_, (XMLoadFloat3(&position_) - XMLoadFloat3(&right_) * velocity));
	if (keys_.RIGHT)
		XMStoreFloat3(&position_, (XMLoadFloat3(&position_) + XMLoadFloat3(&right_) * velocity));
}

void Camera::setCenter(int x, int y) {
	centerX_ = x;
	centerY_ = y;
}
