#include "GameTimer.h"

#include "Camera.h"
#include "DXSample.h"

GameTimer::GameTimer() {
	INT64 countPerSecond;
	QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&countPerSecond));
	secondsPerCount_ = 1.0 / static_cast<double>(countPerSecond);
}

float GameTimer::totalTime() const {
	if (stopped_) {
		return static_cast<float>((stopTime_ - baseTime_ - pausedTime_) * secondsPerCount_);
	}

	return static_cast<float>((currTime_ - baseTime_ - pausedTime_) * secondsPerCount_);
}

float GameTimer::deltaTime() const {
	return static_cast<float>(deltaTime_);	
}

void GameTimer::reset() {
	QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&baseTime_));
	currTime_ = baseTime_;
	pausedTime_ = 0;
	stopped_ = false;
}

void GameTimer::start() {
	if (stopped_) {
		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&prevTime_));
		pausedTime_ += prevTime_ - stopTime_;
		stopTime_ = 0;
		stopped_ = false;
	}
}

void GameTimer::stop() {
	if (!stopped_) {
		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&stopTime_));
		stopped_ =  true;
	}
}


void GameTimer::tick() {
	if (stopped_) {
		deltaTime_ = 0;
		return ;
	}

	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currTime_));
	deltaTime_ = (currTime_ - prevTime_) * secondsPerCount_;
	prevTime_ = currTime_;

	if (deltaTime_ < 0.0) {
		deltaTime_ = 0.0;
	}
}
