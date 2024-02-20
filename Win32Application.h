#pragma once
#include <windows.h>

class DXSample;

class Win32Application {
public:
	static int run(DXSample* sample, HINSTANCE hInstance, int nCmdShow);
	static HWND getHwnd() { return hwnd_; }
protected:
	static LRESULT CALLBACK windowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
private:
	static HWND hwnd_;
};
