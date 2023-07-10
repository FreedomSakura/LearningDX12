#pragma once

#include <iostream>

// DX12������
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl.h>

#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
#include <comdef.h>

#include "DX12Utils/d3dx12.h"
#include "DX12Utils/MathHelper.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")

// DX12�����ࡪ����װ��һЩͨ�õĺ���
#include "ToolFunc.h"
#include "GameTime.h"
#include "WndProc.h"


// ComPtr��������
using namespace Microsoft::WRL;

class D3D12App
{
protected:
	D3D12App() {};
	virtual ~D3D12App() {};

public:
	virtual bool Init(HINSTANCE hInstance, int nShowCmd);
	bool InitWindow(HINSTANCE hInstance, int nShowCmd);
	bool InitD3DPipeline();

	int Run();
	virtual bool Draw() = 0;

	// Init()��ĺ���
	void CreateDevice();
	void CreateFence();
	void GetDescriptorSize();
	void SetMSAA();
	void CreateCommandObject();
	void CreateSwapChain();
	void CreateDescriptorHeap();
	void CreateRTV();
	void CreateDSV();
	void CreateViewPortAndScissorRect();

	// Draw()��ĺ�����
	void FlushCmdQueue();		// ͬ��CPU��GPU
	void CalculateFrameState();	// ����֡��


protected:
	HWND m_hwnd = 0;

	ComPtr<IDXGIFactory4> m_dxgiFactory;
	ComPtr<ID3D12Device> m_d3dDevice;
	ComPtr<ID3D12Fence> m_fence;
	ComPtr<ID3D12CommandQueue> m_cmdQueue;
	ComPtr<ID3D12CommandAllocator> m_cmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> m_cmdList;
	ComPtr<IDXGISwapChain> m_swapChain;
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap; // ��ȾĿ����������
	ComPtr<ID3D12DescriptorHeap> m_dsvHeap; // ���ģ����������
	ComPtr<ID3D12Resource> m_swapChainBuffer[2]; // RTV�Ļ��������ӽ������ϻ�ȡ��������˫���壡��
	ComPtr<ID3D12Resource> m_depthStencilBuffer; // DSV�Ļ��������ӽ������ϻ�ȡ��

	// �ӿ� & �ü�����
	D3D12_VIEWPORT m_viewPort;
	D3D12_RECT m_scissorRect;

	UINT m_rtvDescriptorSize = 0;
	UINT m_dsvDescriptorSize = 0;
	UINT m_cbv_srv_uavDescriptorSize = 0;
	UINT m_currentFence = 0;

	UINT m_currentBackBuffer = 0;

	// ��ʱ����
	GameTime m_gt;

};
