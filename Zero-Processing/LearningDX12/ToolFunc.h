#pragma once
#include <Windows.h>
#include <wrl.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
#include <string>
#include <memory>
#include <algorithm>
#include <vector>
#include <array>
#include <unordered_map>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <windowsx.h>
#include <comdef.h>
#include "DX12Utils/d3dx12.h"
#include "DX12Utils/MathHelper.h"

#include <iostream>

using namespace Microsoft::WRL;

//////////////////////////////////////////////////////////////////////////
// DX12�����ࡪ����װ��һЩͨ�õĺ���
//////////////////////////////////////////////////////////////////////////


//AnsiToWString������ת���ɿ��ַ����͵��ַ�����wstring��
//��Windowsƽ̨�ϣ�����Ӧ�ö�ʹ��wstring��wchar_t������ʽ�����ַ���ǰ+L
inline std::wstring AnsiToWString(const std::string& str)
{
	WCHAR buffer[512];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
	return std::wstring(buffer);
}
//DxException��
class DxException
{
public:
	DxException() = default;
	DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber);

	std::wstring ToString()const;

	HRESULT ErrorCode = S_OK;
	std::wstring FunctionName;
	std::wstring Filename;
	int LineNumber = -1;
};

//�궨��ThrowIfFailed
#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                              \
{                                                                     \
    HRESULT hr__ = (x);                                               \
    std::wstring wfn = AnsiToWString(__FILE__);                       \
    if(FAILED(hr__)) { throw DxException(hr__, L#x, wfn, __LINE__); } \
}
#endif


class ToolFunc
{
public:
	// �����ϴ��� & Ĭ�϶�
	static ComPtr<ID3D12Resource> CreateDefaultBuffer(ComPtr<ID3D12Device> d3dDevice, ComPtr<ID3D12GraphicsCommandList> cmdList, UINT64 byteSize, const void* initData, ComPtr<ID3D12Resource>& uploadBuffer);
	// ������ɫ��
	static ComPtr<ID3DBlob> CompileShader(const std::wstring& fileName, const D3D_SHADER_MACRO* defines, const std::string& enteryPoint, const std::string& target);

};