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
#pragma comment(lib, "dxguid.lib")

// DX12�����ࡪ����װ��һЩͨ�õĺ���
#include "ToolFunc.h"
#include "GameTime.h"
//#include "WndProc.h"


// ComPtr��������
using namespace Microsoft::WRL;
using namespace DirectX;

class D3D12App
{
protected:
	D3D12App() {
		assert(m_App == nullptr);
		m_App = this;
	};
	virtual ~D3D12App() {};

public:
	static D3D12App* GetApp();

	virtual bool Init(HINSTANCE hInstance, int nShowCmd);
	bool InitWindow(HINSTANCE hInstance, int nShowCmd);
	bool InitD3DPipeline();
	int Run();
	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	
protected:
	virtual bool Draw() = 0;
	virtual void Update() = 0;
	virtual void OnResize();
	// ���������
	virtual void OnMouseDown(WPARAM btnState, int x, int y);
	virtual void OnMouseUp(WPARAM btnState, int x, int y);
	virtual void OnMouseMove(WPARAM btnState, int x, int y);

	// ��ʼ��D3D�����õĺ���
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

	// Draw()����Ҫ�õ��ĺ���
	void FlushCmdQueue();		// ͬ��CPU��GPU
	void CalculateFrameState();	// ����֡��


protected:
	static D3D12App* m_App;

	HWND m_hwnd = nullptr;

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
	// ����������
	UINT m_rtvDescriptorSize = 0;
	UINT m_dsvDescriptorSize = 0;
	UINT m_cbv_srv_uavDescriptorSize = 0;
	UINT m_currentBackBuffer = 0;
	// CPUΧ��
	UINT m_currentFence = 0;

	// ��ʱ����
	GameTime m_gt;

	// ����ƶ�������任
	POINT m_lastMousePos;
	float m_theta = 1.5f * XM_PI;
	float m_phi = XM_PIDIV4;
	float m_radius = 5.0f;

	// App��״̬
	bool m_isAppPaused = false;
	bool m_isMinimized = false;
	bool m_isMaximized = false;
	bool m_isResizing = false;
	// �ӿڲ���
	UINT m_clientWidth = 1280;
	UINT m_clientHeight = 720;
};
