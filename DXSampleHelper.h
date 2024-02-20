//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#pragma once
#include <stdexcept>
#include <wrl/client.h>
#include <d3d12.h>
#include <D3Dcompiler.h>
#include <wrl/wrappers/corewrappers.h>

inline std::string hrToString(HRESULT hr) {
	char str[64] = {};
	sprintf_s(str, "HRESULT of 0x%08x", static_cast<UINT>(hr));
	return std::string(str);
}

class HrException : public std::runtime_error {
public:
	HrException(HRESULT hr) : std::runtime_error(hrToString(hr)), hr_(hr) {}
	HRESULT error() const { return hr_; }
private:
	const HRESULT hr_;
};

#define SAFE_RELEASE(p) if (p) (p)->Release()

inline void ThrowIfFailed(HRESULT hr) {
	if (FAILED(hr)) {
		throw HrException(hr);
	}
}

inline void ThrowIfFailed(HRESULT hr, const wchar_t* msg) {
	if (FAILED(hr)) {
		OutputDebugString(msg);
		throw HrException(hr);
	}
}

inline void ThrowIfFailed(bool value) {
	ThrowIfFailed(value ? S_OK : E_FAIL);
}

inline void ThrowIfFailed(bool value, const wchar_t* msg) {
	ThrowIfFailed(value ? S_OK : E_FAIL, msg);
}

inline void getAssetsPath(_Out_writes_(pathSize) WCHAR* path, UINT pathSize) {
	if (path == nullptr) {
		throw std::exception();
	}

	DWORD size = GetModuleFileName(nullptr, path, pathSize);
	if (size == 0 || size == pathSize) {
		throw std::exception();
	}

	WCHAR* lastSlash = wcsrchr(path, L'\\');
	if (lastSlash) {
		*(lastSlash + 1) = L'\0';
	}
}

inline HRESULT readDataFromFile(LPCWSTR filename, byte** data, UINT* size) {
	using namespace Microsoft::WRL;

#if WINVER >= _WIN32_WINNT_WIN8
	CREATEFILE2_EXTENDED_PARAMETERS extendedParameters = {};
	extendedParameters.dwSize = sizeof(CREATEFILE2_EXTENDED_PARAMETERS);
	extendedParameters.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
	extendedParameters.dwFileAttributes = FILE_FLAG_SEQUENTIAL_SCAN;
	extendedParameters.dwSecurityQosFlags = SECURITY_ANONYMOUS;
	extendedParameters.lpSecurityAttributes = nullptr;
	extendedParameters.hTemplateFile = nullptr;

	Wrappers::FileHandle file(CreateFile2(filename, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, &extendedParameters));
#else
    Wrappers::FileHandle file(CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN | SECURITY_SQOS_PRESENT | SECURITY_ANONYMOUS, nullptr));
#endif
	if (file.Get() == INVALID_HANDLE_VALUE) {
		throw std::exception();
	}

	FILE_STANDARD_INFO fileInfo = {};
	if (!GetFileInformationByHandleEx(file.Get(), FileStandardInfo, &fileInfo, sizeof(fileInfo))) {
		throw std::exception();
	}

	if (fileInfo.EndOfFile.HighPart != 0) {
		throw std::exception();
	}

	*data = static_cast<byte*>(malloc(fileInfo.EndOfFile.LowPart));
	*size = fileInfo.EndOfFile.LowPart;

	if (!ReadFile(file.Get(), *data, fileInfo.EndOfFile.LowPart, nullptr, nullptr)) {
		throw std::exception();
	}

	return S_OK;
}

inline HRESULT readDataFromDDSFile(LPCWSTR filename, byte** data, UINT* offset, UINT* size) {
	if (FAILED(readDataFromFile(filename, data, size))) {
		return E_FAIL;
	}

	static const UINT DDS_MAGIC = 0x20534444;
	UINT magicNumber = *reinterpret_cast<const UINT*>(*data);
	if (magicNumber != DDS_MAGIC) {
		return E_FAIL;
	}

	struct DDS_PIXELFORMAT {
		UINT size;
		UINT flags;
		UINT fourCC;
		UINT rgbBitCount;
		UINT rBitMask;
		UINT gBitMask;
		UINT bBitMask;
		UINT aBitMask;
	};

	struct DDS_HEADER {
		UINT size;
		UINT flags;
		UINT height;
		UINT width;
		UINT pitchOrLinearSize;
		UINT depth;
		UINT mipMapCount;
		UINT reserved1[11];
		DDS_PIXELFORMAT ddsPixelFormat;
		UINT caps;
		UINT caps2;
		UINT caps3;
		UINT caps4;
		UINT reserved2;
	};

	auto ddsHeader = reinterpret_cast<const DDS_HEADER*>(*data + sizeof(UINT));
	if (ddsHeader->size != sizeof(DDS_HEADER) || ddsHeader->ddsPixelFormat.size != sizeof(DDS_PIXELFORMAT)) {
		return E_FAIL;
	}

	const ptrdiff_t ddsDataOffset = sizeof(UINT) + sizeof(DDS_HEADER);
	*offset = ddsDataOffset;
	*size = *size - ddsDataOffset;

	return S_OK;
}

#if defined(_DEBUG) || defined(DBG)
inline void setName(ID3D12Object* object, LPCWSTR name) {
	object->SetName(name);
}

inline void setNameIndexed(ID3D12Object* object, LPCWSTR name, UINT index) {
	WCHAR fullName[50];
	if (swprintf_s(fullName, L"%s[%u]", name, index) > 0) {
		object->SetName(fullName);
	}
}
#else
inline void setName(ID3D12Object* object, LPCWSTR name) {
	
}

inline void setNameIndexed(ID3D12Object* object, LPCWSTR name, UINT index) {
	
}
#endif

#define NAME_D3D12_OBJECT(x) setName((x).Get(), L#x)
#define NAME_D3D12_OBJECT_INDEXED(x, n) setNameIndexed((x)[n].Get(), L#x, n)

#ifdef D3D_COMPILE_STANDARD_FILE_INCLUDE
inline UINT calculateConstantBufferByteSize(UINT byteSize) {
	return (byteSize + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);
}

inline Microsoft::WRL::ComPtr<ID3DBlob> compilerShader(
	const std::wstring& filename, 
	const D3D_SHADER_MACRO* defines, 
	const std::string& entrypoint, 
	const std::string& target) {
	UINT compileFlags = 0;
#if defined(_DEBUG) || defined(DBG)
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	HRESULT hr;
	Microsoft::WRL::ComPtr<ID3DBlob> byteCode = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errors = nullptr;
	hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE, entrypoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);

	if (errors != nullptr) {
		OutputDebugStringA(static_cast<char*>(errors->GetBufferPointer()));
	}
	ThrowIfFailed(hr);

	return byteCode;
}
#endif

template<typename T>
void resetComPtrArray(T* comPtrArray) {
	for (auto& e : comPtrArray) {
		e.Reset();
	}
}

template<typename T>
void resetUniquePtrArray(T* uniquePtrArray) {
	for (auto& e : uniquePtrArray) {
		e.reset();
	}
} 