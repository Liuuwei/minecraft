#pragma once
#include <d3dcommon.h>
#include <D3Dcompiler.h>
#include <d3dx12.h>
#include <wrl/client.h>

class Shader {
public:
	Shader(const wchar_t* vs);
	Shader(const wchar_t* vs, const wchar_t* ps);
	Shader(const wchar_t* vs, const wchar_t* gs, const wchar_t* ps);

	bool hasGeometry() const { return gs_ != nullptr; }
	bool hasPixel() const { return ps_ != nullptr; }

	CD3DX12_SHADER_BYTECODE vertex() const;
	CD3DX12_SHADER_BYTECODE geometry() const;
	CD3DX12_SHADER_BYTECODE pixel() const;
private:
	Microsoft::WRL::ComPtr<ID3DBlob> vs_ = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> gs_ = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> ps_ = nullptr;

	UINT compileFlags_ = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
};
