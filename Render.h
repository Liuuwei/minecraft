#pragma once
#include "Win32Application.h"

#include <MyDirectx/MyDirectx.h>

class Render {
public:
	Render(UINT width, UINT height, std::wstring title);
	UINT getWidth() const { return width_; }
	UINT getheight() const { return height_; }
	const WCHAR* getTitle() const { return title_.c_str(); }

	void onInit();
	void onRender();

	
	
	HWND hwnd_;
private:
	UINT width_;
	UINT height_;
	std::wstring title_;

	MyDirectx myDirectx_;
};
