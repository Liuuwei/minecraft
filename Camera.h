#pragma once
#include <DirectXMath.h>
#include <windows.h>

class Camera {
public:
	Camera() : position_(0, 0, 0), front_(0, 0, -1), up_(0, 1, 0), right_(1, 0, 0), worldUp_(up_) {}
	Camera(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 front) : position_(position), front_(front), up_(0, 1, 0), right_(1, 0, 0), worldUp_(up_) {}

	DirectX::XMMATRIX getViewMatrix();
	DirectX::XMMATRIX getProjectionMatrix(float width, float height, float near, float far);

	void setCenter(int x, int y);

	void onKeyUp(WPARAM key);
	void onKeyDown(WPARAM key);

	void onMouseMovement(LPARAM param);

	void update(float deltaTime);

	DirectX::XMFLOAT3 position() const { return position_; }
	DirectX::XMFLOAT3 front() const { return front_; }
private:
	struct Keys {
		bool FORWARD;
		bool BACKWARD;
		bool LEFT;
		bool RIGHT;
	};

	DirectX::XMFLOAT3 position_{};
	DirectX::XMFLOAT3 front_{};
	DirectX::XMFLOAT3 up_{};
	DirectX::XMFLOAT3 right_{};
	DirectX::XMFLOAT3 worldUp_{};

	float yaw_ = -90.0f;
	float pitch_ = 0.0f;
	float movementSpeed_ = 2.5f;
	float mouseSensitivity_ = 0.1f;
	float zoom_ = 45.0f;

	int lastX_ = 9527;
	int lastY_ = 9527;

	int centerX_;
	int centerY_;

	Keys keys_{};
};
