#pragma once
#include <windows.h>

class MySample;

class Win32Application {
public:
	static int run(MySample* sample, HINSTANCE hInstance, int nCmdShow);
	static HWND getHwnd() { return hwnd_; }
protected:
	static LRESULT CALLBACK windowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
private:
	static HWND hwnd_;
};
