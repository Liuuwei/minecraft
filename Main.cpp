#include <D3Dcompiler.h>

#include "HelloWorld.h"
#include "Win32Application.h"

#include <MyDirectx/Desktop.h>

#include <pix3.h>

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    PIXLoadLatestWinPixGpuCapturerLibrary();
    // HelloWorld sample(1920, 1080, L"minecraft");
    // HelloWorld sample(1600, 900, L"minecraft");
    HelloWorld sample(1280, 720, L"minecraft");
    // return Win32Application::run(&sample, hInstance, nCmdShow);
    
    Win32Application::run(&sample, hInstance, nCmdShow);
}