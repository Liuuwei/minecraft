#include "Win32Application.h"

#include "DXSample.h"

HWND Win32Application::hwnd_ = nullptr;

int Win32Application::run(DXSample* sample, HINSTANCE hInstance, int nCmdShow) {
	int argc;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	sample->parseCommandLineArgs(argv, argc);
	LocalFree(argv);

	WNDCLASSEX windowClass = { 0};
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = windowProc;
	windowClass.hInstance = hInstance;
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.lpszClassName = L"DXSampleClass";
	RegisterClassEx(&windowClass);

	RECT windowRect = {0, 0, static_cast<LONG>(sample->getWidth()), static_cast<LONG>(sample->getHeight())};
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	hwnd_ = CreateWindow(
		windowClass.lpszClassName, 
		sample->getTitle(), 
		WS_OVERLAPPEDWINDOW, 
		CW_USEDEFAULT, 
		CW_USEDEFAULT, 
		windowRect.right - windowRect.left, 
		windowRect.bottom - windowRect.top, 
		nullptr, 
		nullptr, 
		hInstance, 
		sample);

	sample->hwnd_ = hwnd_;
	sample->onInit();

	ShowWindow(hwnd_, nCmdShow);

	MSG msg = {};
	while (msg.message != WM_QUIT) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	sample->onDestroy();

	return static_cast<char>(msg.wParam);
}

LRESULT Win32Application::windowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	DXSample* sample = reinterpret_cast<DXSample*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

	switch (message) {
	case WM_CREATE:
		{
		LPCREATESTRUCT createStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
		ShowCursor(FALSE);
		}
		return 0;

	case WM_KEYDOWN: 
		if (sample) {
			sample->onKeyDown(wParam);
		} 
		return 0;
	
	case WM_KEYUP: 
		if (sample) {
			sample->onKeyUp(wParam);
		}
		return 0;

	case WM_RBUTTONDOWN:
		if (sample) {
			sample->onRightButtonDown();
		}
		return 0;

	case WM_LBUTTONDOWN:
		if (sample) {
			sample->onLeftButtonDown();
		}
		return 0;

	case WM_PAINT:
		if (sample) {
			sample->onUpdate(wParam, lParam);
			sample->onRender();
		}

		return 0;

	case WM_DESTROY: 
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

