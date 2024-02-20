#pragma once
#include <intsafe.h>

class GameTimer {
public:
	GameTimer();

	float totalTime() const;
	float deltaTime() const;

	void reset();
	void start();
	void stop();
	void tick();

private:
	double secondsPerCount_ = 0.0;
	double deltaTime_ = -1.0;
	INT64 baseTime_ = 0;
	INT64 pausedTime_ = 0;
	INT64 stopTime_ = 0;
	INT64 prevTime_ = 0;
	INT64 currTime_ = 0;

	bool stopped_ = false;
};
