#include <D3Dcompiler.h>

#include "HelloWorld.h"
#include "Win32Application.h"

#include <pix3.h>

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    PIXLoadLatestWinPixGpuCapturerLibrary();
    HelloWorld sample(1280, 720, L"D3D12 Hello World");
    return Win32Application::run(&sample, hInstance, nCmdShow);
}