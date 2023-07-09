#pragma once

#include <iostream>

// DX12的依赖
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <DirectXColors.h>

#include "DXUtils/d3dx12.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

// DX12工具类——封装了一些通用的函数
#include "D3DUtil.h"

// ComPtr就在这里
using namespace Microsoft::WRL;


class D3DAPP
{
public:
	bool InitD3DPipeline(HWND hwnd);
	bool Draw();

	// Init()里的函数
	void CreateDevice();
	void CreateFence();
	void GetDescriptorSize();
	void SetMSAA();
	void CreateCommandObject();
	void CreateSwapChain(HWND hwnd);
	void CreateDescriptorHeap();
	void CreateRTV();
	void CreateDSV();
	void CreateViewPortAndScissorRect();

	// Draw()里的函数！
	void FlushCmdQueue();


private:
	ComPtr<IDXGIFactory4> m_dxgiFactory;
	ComPtr<ID3D12Device> m_d3dDevice;
	ComPtr<ID3D12Fence> m_fence;
	ComPtr<ID3D12CommandQueue> m_cmdQueue;
	ComPtr<ID3D12CommandAllocator> m_cmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> m_cmdList;
	ComPtr<IDXGISwapChain> m_swapChain;
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap; // 渲染目标描述符堆
	ComPtr<ID3D12DescriptorHeap> m_dsvHeap; // 深度模板描述符堆
	ComPtr<ID3D12Resource> m_swapChainBuffer[2]; // RTV的缓冲区（从交换链上获取，并且是双缓冲！）
	ComPtr<ID3D12Resource> m_depthStencilBuffer; // DSV的缓冲区（从交换链上获取）

	// 视口 & 裁剪矩形
	D3D12_VIEWPORT m_viewPort;
	D3D12_RECT m_scissorRect;

	UINT m_rtvDescriptorSize = 0;
	UINT m_dsvDescriptorSize = 0;
	UINT m_cbv_srv_uavDescriptorSize = 0;
	UINT m_currentFence = 0;
	UINT m_currentBackBuffer = 0;
};
