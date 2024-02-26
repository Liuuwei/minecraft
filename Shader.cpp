#include "Shader.h"

#include <D3Dcompiler.h>

#include <MyDirectx/MySampleHelp.h>

Shader::Shader(const wchar_t* vs) {
	ThrowIfFailed(D3DCompileFromFile(vs, nullptr, nullptr, "main", "vs_5_1", compileFlags_, 0, &vs_, nullptr));
}


Shader::Shader(const wchar_t* vs, const wchar_t* ps) {
	ThrowIfFailed(D3DCompileFromFile(vs, nullptr, nullptr, "main", "vs_5_1", compileFlags_, 0, &vs_, nullptr));

	ThrowIfFailed(D3DCompileFromFile(ps, nullptr, nullptr, "main", "ps_5_1", compileFlags_, 0, &ps_, nullptr));
}


Shader::Shader(const wchar_t* vs, const wchar_t* gs, const wchar_t* ps) {
	ThrowIfFailed(D3DCompileFromFile(vs, nullptr, nullptr, "main", "vs_5_1", compileFlags_, 0, &vs_, nullptr));

	ThrowIfFailed(D3DCompileFromFile(gs, nullptr, nullptr, "main", "gs_5_1", compileFlags_, 0, &gs_, nullptr));

	ThrowIfFailed(D3DCompileFromFile(ps, nullptr, nullptr, "main", "ps_5_1", compileFlags_, 0, &ps_, nullptr));
}

CD3DX12_SHADER_BYTECODE Shader::vertex() const {
	return {vs_->GetBufferPointer(), vs_->GetBufferSize()};
}

CD3DX12_SHADER_BYTECODE Shader::geometry() const {
	return {gs_->GetBufferPointer(), gs_->GetBufferSize()};
}

CD3DX12_SHADER_BYTECODE Shader::pixel() const {
	return {ps_->GetBufferPointer(), ps_->GetBufferSize()};
}

