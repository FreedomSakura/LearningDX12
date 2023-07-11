#include "D3D12App.h"


LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	return D3D12App::GetApp()->MsgProc(hWnd, message, wParam, lParam);
}

D3D12App* D3D12App::m_App = nullptr;
D3D12App* D3D12App::GetApp()
{
	return m_App;
}

// win32���
bool D3D12App::Init(HINSTANCE hInstance, int nShowCmd) {
	if (!InitWindow(hInstance, nShowCmd))
		return false;
	if (!InitD3DPipeline())
		return false;

	OnResize();

	return true;
}

bool D3D12App::InitWindow(HINSTANCE hInstance, int nShowCmd) {
	//���ڳ�ʼ�������ṹ��(WNDCLASS)
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;	//����������߸ı䣬�����»��ƴ���
	wc.lpfnWndProc = MainWndProc;	//ָ�����ڹ���
	wc.cbClsExtra = 0;	//�����������ֶ���Ϊ��ǰӦ�÷��������ڴ�ռ䣨���ﲻ���䣬������0��
	wc.cbWndExtra = 0;	//�����������ֶ���Ϊ��ǰӦ�÷��������ڴ�ռ䣨���ﲻ���䣬������0��
	wc.hInstance = hInstance;	//Ӧ�ó���ʵ���������WinMain���룩
	wc.hIcon = LoadIcon(0, IDC_ARROW);	//ʹ��Ĭ�ϵ�Ӧ�ó���ͼ��
	wc.hCursor = LoadCursor(0, IDC_ARROW);	//ʹ�ñ�׼�����ָ����ʽ
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);	//ָ���˰�ɫ������ˢ���
	wc.lpszMenuName = 0;	//û�в˵���
	wc.lpszClassName = L"MainWnd";	//������
	//������ע��ʧ��
	if (!RegisterClass(&wc))
	{
		//��Ϣ����������1����Ϣ���������ھ������ΪNULL������2����Ϣ����ʾ���ı���Ϣ������3�������ı�������4����Ϣ����ʽ
		MessageBox(0, L"RegisterClass Failed", 0, 0);
		return 0;
	}

	//������ע��ɹ�
	RECT R;	//�ü�����
	R.left = 0;
	R.top = 0;
	R.right = 1280;
	R.bottom = 720;
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);	//���ݴ��ڵĿͻ�����С���㴰�ڵĴ�С
	int width = R.right - R.left;
	int hight = R.bottom - R.top;

	//��������,���ز���ֵ
	m_hwnd = CreateWindow(L"MainWnd", L"DX12Initialize", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, hight, 0, 0, hInstance, 0);
	//���ڴ���ʧ��
	if (!m_hwnd)
	{
		MessageBox(0, L"CreatWindow Failed", 0, 0);
		return 0;
	}
	//���ڴ����ɹ�,����ʾ�����´���
	ShowWindow(m_hwnd, nShowCmd);
	UpdateWindow(m_hwnd);

	// ˳����һ�¿���̨
	//AllocConsole();

	return true;
}

// ��ʼ��D3D��Ⱦ����
bool D3D12App::InitD3DPipeline() {
	// ��D3D12���Բ�
#if defined(DEBUG) | defined(_DEBUG)
	ComPtr<ID3D12Debug> debugController;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
	debugController->EnableDebugLayer();
#endif

	CreateDevice();
	CreateFence();
	GetDescriptorSize();
	SetMSAA();
	CreateCommandObject();
	CreateSwapChain();

	CreateDescriptorHeap();
	CreateRTV();
	CreateDSV();

	CreateViewPortAndScissorRect();

	return true;
}

int D3D12App::Run()
{
	MSG msg = { 0 };

	//ÿ��ѭ����ʼ��Ҫ���ü�ʱ��
	m_gt.Reset();

	// ����Ϣѭ��:
	while (msg.message != WM_QUIT)
	{
		// �д�����Ϣ�ʹ���
		//if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// ����ͼ���ִ����Ⱦ & ��Ϸ�߼�
		else
		{
			m_gt.Tick();	//����ÿ��֡���ʱ��

			if (!m_gt.IsStoped())//���������ͣ״̬�����ǲ�������Ϸ
			{
				CalculateFrameState();
				Update();
				Draw();
			}
			//�������ͣ״̬��������100��
			else
			{
				Sleep(100);
			}
		}
	}

	return (int)msg.wParam;
}

LRESULT D3D12App::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	//��Ϣ����
	switch (msg)
	{
		//��갴������ʱ�Ĵ����������ң�
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		//wParamΪ�������������룬lParamΪϵͳ�����Ĺ����Ϣ
		OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
		//��갴��̧��ʱ�Ĵ����������ң�
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
		//����ƶ��Ĵ���
	case WM_MOUSEMOVE:
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
		//�����ڱ�����ʱ����ֹ��Ϣѭ��
	case WM_DESTROY:
		PostQuitMessage(0);	//��ֹ��Ϣѭ����������WM_QUIT��Ϣ
		return 0;
	case WM_SIZE:
		m_clientWidth = LOWORD(lParam);
		m_clientHeight = HIWORD(lParam);
		if (m_d3dDevice)
		{
			//�����С��,����ͣ��Ϸ��������С�������״̬
			if (wParam == SIZE_MINIMIZED)
			{
				m_isAppPaused = true;
				m_isMinimized = true;
				m_isMaximized = false;
			}
			else if (wParam == SIZE_MAXIMIZED)
			{
				m_isAppPaused = false;
				m_isMinimized = false;
				m_isMaximized = true;
				OnResize();
			}
			else if (wParam == SIZE_RESTORED)
			{
				if (m_isMinimized)
				{
					m_isAppPaused = false;
					m_isMinimized = false;
					OnResize();
				}

				else if (m_isMaximized)
				{
					m_isAppPaused = false;
					m_isMaximized = false;
					OnResize();
				}

				else if (m_isResizing)
				{

				}
				else
				{
					OnResize();
				}
			}
		}

		return 0;
	default:
		break;
	}
	//������û�д������Ϣת����Ĭ�ϵĴ��ڹ���
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

// ���������
void D3D12App::OnMouseDown(WPARAM btnState, int x, int y) {
	m_lastMousePos.x = x;	//���µ�ʱ���¼����x����
	m_lastMousePos.y = y;	//���µ�ʱ���¼����y����

	SetCapture(m_hwnd);	//�����ڵ�ǰ�̵߳�ָ�������������겶��
}

void D3D12App::OnMouseUp(WPARAM btnState, int x, int y) {
	ReleaseCapture();
}

void D3D12App::OnMouseMove(WPARAM btnState, int x, int y) {
	if ((btnState & MK_LBUTTON) != 0)//������������״̬
	{
		//�������ƶ����뻻��ɻ��ȣ�0.25Ϊ������ֵ
		float dx = XMConvertToRadians(static_cast<float>(m_lastMousePos.x - x) * 0.25f);
		float dy = XMConvertToRadians(static_cast<float>(m_lastMousePos.y - y) * 0.25f);
		//�������û���ɿ�ǰ���ۼƻ���
		m_theta += dx;
		m_phi += dy;
		//���ƽǶ�phi�ķ�Χ�ڣ�0.1�� Pi-0.1��
		m_theta = MathHelper::Clamp(m_theta, 0.1f, 3.1416f - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0)//������Ҽ�����״̬
	{
		//�������ƶ����뻻������Ŵ�С��0.005Ϊ������ֵ
		float dx = 0.005f * static_cast<float>(x - m_lastMousePos.x);
		float dy = 0.005f * static_cast<float>(y - m_lastMousePos.y);
		//����������������������ӷ�Χ�뾶
		m_radius += dx - dy;
		//���ƿ��ӷ�Χ�뾶
		m_radius = MathHelper::Clamp(m_radius, 1.0f, 20.0f);
	}
	//����ǰ������긳ֵ������һ��������ꡱ��Ϊ��һ���������ṩ��ǰֵ
	m_lastMousePos.x = x;
	m_lastMousePos.y = y;
}

void D3D12App::OnResize() {
	assert(m_d3dDevice);
	assert(m_swapChain);
	assert(m_cmdAllocator);
	FlushCmdQueue();//�ı���Դǰ��ͬ��

	//ThrowIfFailed(m_cmdList->Reset(m_cmdAllocator.Get(), nullptr));

	//�ͷ�֮ǰ����Դ��Ϊ�������´�������׼��
	for (int i = 0; i < 2; i++)
		m_swapChainBuffer[i].Reset();
	m_depthStencilBuffer.Reset();
	//���µ�����̨��������Դ�Ĵ�С
	ThrowIfFailed(m_swapChain->ResizeBuffers(
		2,
		m_clientWidth,
		m_clientHeight,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING));
	//��̨��������������
	m_currentBackBuffer = 0;

	CreateRTV();
	CreateDSV();
	CreateViewPortAndScissorRect();
}


// ��ʼ��D3D�����õĺ���
//1�������豸
void D3D12App::CreateDevice()
{
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&m_dxgiFactory)));

	ThrowIfFailed(
		D3D12CreateDevice(
			nullptr,// Ĭ���豸��Ҳ��������ʾ��
			D3D_FEATURE_LEVEL_12_0,// Ӧ�ó�����ҪӲ��֧�ֵ����DX�汾
			IID_PPV_ARGS(&m_d3dDevice) // ���ش������豸
		)
	);

}

//2������Χ��fence������ͬ��CPU��GPU
void D3D12App::CreateFence()
{
	ThrowIfFailed(m_d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
}

//3����ȡ��������С�����ǻ�ȡ���ֻ�������Ԫ�صĴ�С��������Ȼ�������һ��pixel�Ĵ�С�Ƕ��٣���
void D3D12App::GetDescriptorSize()
{
	m_rtvDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_dsvDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	m_cbv_srv_uavDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

//4������MSAA����
void D3D12App::SetMSAA()
{
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaQualityLevels;
	msaaQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;	// UNORM�ǹ�һ��������޷�������
	msaaQualityLevels.SampleCount = 1;
	msaaQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msaaQualityLevels.NumQualityLevels = 0;// ������MSAA������������Ϊ0
	//��ǰͼ��������MSAA���ز�����֧�֣�ע�⣺�ڶ������������������������
	ThrowIfFailed(m_d3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msaaQualityLevels, sizeof(msaaQualityLevels)));
	//NumQualityLevels��Check��������������
	//���֧��MSAA����Check�������ص�NumQualityLevels > 0
	//expressionΪ�٣���Ϊ0��������ֹ�������У�����ӡһ��������Ϣ
	assert(msaaQualityLevels.NumQualityLevels > 0);
}

//5������������С������б����������
// ���ߵĹ�ϵ�� CPU���������б� -> ������������������ϵ�����������б� -> ��������������и�GPU����
// ����ֻ�����˴������ߵĹ�������
void D3D12App::CreateCommandObject()
{
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	ThrowIfFailed(m_d3dDevice->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&m_cmdQueue)));

	ThrowIfFailed(m_d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_cmdAllocator)));//&cmdAllocator�ȼ���cmdAllocator.GetAddressOf

	ThrowIfFailed(
		m_d3dDevice->CreateCommandList(
			0,// ����ֵΪ0����GPU
			D3D12_COMMAND_LIST_TYPE_DIRECT,// �����б�����
			m_cmdAllocator.Get(),//����������ӿ�ָ��
			nullptr,// ��ˮ��״̬����PSO�����ﲻ���ƣ�����nullptr
			IID_PPV_ARGS(&m_cmdList)// ���ش����������б�
		)
	);
	m_cmdList->Close();// ���������б�ǰ���뽫��ر�
}

//6������������
void D3D12App::CreateSwapChain()
{
	m_swapChain.Reset();

	DXGI_SWAP_CHAIN_DESC swapChainDesc;// �����������ṹ��
	swapChainDesc.BufferDesc.Width = m_clientWidth;
	swapChainDesc.BufferDesc.Height = m_clientHeight;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // ����������ʾ��ʽ
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;	//ˢ���ʵķ���
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;	//ˢ���ʵķ�ĸ
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED; // ����ɨ��VS����ɨ��
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED; //ͼ�������Ļ�����죨δָ���ģ�
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // ��������Ⱦ����̨������������Ϊ����Ŀ�꣩
	swapChainDesc.OutputWindow = m_hwnd; // ��Ⱦ���ڵľ��;
	swapChainDesc.SampleDesc.Count = 1; // ���������
	swapChainDesc.SampleDesc.Quality = 0; // ���������
	swapChainDesc.Windowed = true; // �Ƿ񴰿ڻ�
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.BufferCount = 2; // ��̨������������˫���壡��
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
	// ����DXGI�ӿ��µĹ����ഴ��������
	ThrowIfFailed(m_dxgiFactory->CreateSwapChain(m_cmdQueue.Get(), &swapChainDesc, m_swapChain.GetAddressOf()));
}

//7�������������ѣ�DescriptorHeap)
void D3D12App::CreateDescriptorHeap() {
	// �ȴ���RTV
	D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc;
	rtvDescriptorHeapDesc.NumDescriptors = 2;
	rtvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvDescriptorHeapDesc.NodeMask = 0;

	ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

	// ��ȡrtv�������Ĵ�С -> ����ȡ�ͻ��ò���...����ʵ��һ��Ӧ����ǰ���������ȡ��������Сʱ�����ˣ���
	// û�����У�һֱ��cmdList->Close()��ʱ���׳��쳣˵һ�������б���ִ�ж��������ʲô��...
	//m_rtvDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// �ٴ���DSV
	D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc;
	dsvDescriptorHeapDesc.NumDescriptors = 1;
	dsvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvDescriptorHeapDesc.NodeMask = 0;

	ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&dsvDescriptorHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));
}

//8������������
//��1��RTV������
// ��RTV�����õ�RTV��� -> �ӽ��������õ�RT��Դ -> ����RTV��RT��Դ��RTV����������� -> ����RTV��С�ڶ�����ƫ�ƣ��Ա��ȡ�¸�RTV 
void D3D12App::CreateRTV()
{
	// ��ȡrtv���
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

	for (int i = 0; i < 2; i++)
	{
		// �ӽ������ϻ�ȡ��̨��������Դ����д��rtv������
		m_swapChain->GetBuffer(i, IID_PPV_ARGS(m_swapChainBuffer[i].GetAddressOf()));
		// ����RTV
		m_d3dDevice->CreateRenderTargetView(
			m_swapChainBuffer[i].Get(),
			nullptr,
			rtvHeapHandle
		);
		// 
		rtvHeapHandle.Offset(1, m_rtvDescriptorSize);
	}
}


//��2��DSV������
void D3D12App::CreateDSV()
{
	//��CPU�д��������ģ��������Դ
	D3D12_RESOURCE_DESC dsvResourceDesc;
	dsvResourceDesc.Alignment = 0;	//ָ������
	dsvResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;	//ָ����Դά�ȣ����ͣ�ΪTEXTURE2D
	dsvResourceDesc.DepthOrArraySize = 1;	//�������Ϊ1
	dsvResourceDesc.Width = 1280;	//��Դ��
	dsvResourceDesc.Height = 720;	//��Դ��
	dsvResourceDesc.MipLevels = 1;	//MIPMAP�㼶����
	dsvResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;	//ָ�������֣����ﲻָ����
	dsvResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;	//���ģ����Դ��Flag
	dsvResourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;	//24λ��ȣ�8λģ��,���и������͵ĸ�ʽDXGI_FORMAT_R24G8_TYPELESSҲ����ʹ��
	dsvResourceDesc.SampleDesc.Count = 1;	//���ز�������
	//dsvResourceDesc.SampleDesc.Quality = msaaQualityLevels.NumQualityLevels - 1;	//���ز�������
	dsvResourceDesc.SampleDesc.Quality = 0;	//���ز�������
	CD3DX12_CLEAR_VALUE optClear;	//�����Դ���Ż�ֵ��������������ִ���ٶȣ�CreateCommittedResource�����д��룩
	optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;//24λ��ȣ�8λģ��,���и������͵ĸ�ʽDXGI_FORMAT_R24G8_TYPELESSҲ����ʹ��
	optClear.DepthStencil.Depth = 1;	//��ʼ���ֵΪ1
	optClear.DepthStencil.Stencil = 0;	//��ʼģ��ֵΪ0

	// ����һ����Դ��һ���ѣ�������Դ�ύ�����У������ģ�������ύ��GPU�Դ��У�
	// ��CPU�е�DSV��Դ�󶨵�GPU��
	CD3DX12_HEAP_PROPERTIES properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(
		m_d3dDevice->CreateCommittedResource(
			//&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			&properties,
			D3D12_HEAP_FLAG_NONE,
			&dsvResourceDesc,
			D3D12_RESOURCE_STATE_COMMON,
			&optClear,
			IID_PPV_ARGS(&m_depthStencilBuffer)
		)
	);

	// ����DSV(�������DSV���Խṹ�壬�ʹ���RTV��ͬ��RTV��ͨ�����)
	//D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	//dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	//dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	//dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	//dsvDesc.Texture2D.MipSlice = 0;
	m_d3dDevice->CreateDepthStencilView(
		m_depthStencilBuffer.Get(),
		nullptr, //D3D12_DEPTH_STENCIL_VIEW_DESC����ָ�룬����&dsvDesc������ע�ʹ��룩��
				 //�����ڴ������ģ����Դʱ�Ѿ��������ģ���������ԣ������������ָ��Ϊ��ָ��
		m_dsvHeap->GetCPUDescriptorHandleForHeapStart()
	);

	// ���DS��Դ��״̬����Ϊ��Ȼ�����һ�������д�룬һ�������ֻ����...[�ɱ�����]�� -> ����Դ�ӳ�ʼ��״̬תΪ��Ȼ�����
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_depthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, //ת��ǰ״̬������ʱ��״̬����CreateCommittedResource�����ж����״̬��
		D3D12_RESOURCE_STATE_DEPTH_WRITE //ת����״̬Ϊ��д������ͼ������һ��D3D12_RESOURCE_STATE_DEPTH_READ��ֻ�ɶ������ͼ
	);
	m_cmdList->ResourceBarrier(
		1,	//Barrier���ϸ���
		&barrier
	);
}

//9��ʵ��Χ����ʵ��CPU��GPU��ͬ����
// ˼·����ʼ��Χ��ֵΪ0����DX12APIά���� -> �ٴ���һ����ǰΧ��ֵҲ��0��ʼ�������Լ���д������ά����
// CPU�������GPU, m_currentFence++��CPUΧ��++��;
// GPU������CPU�����������Χ��ֵ++��GPUΧ��++��;
// Ȼ���ж�CPUΧ��ֵ��GPUΧ��ֵ�Ĵ�С����ȷ��GPU�Ƿ�����Χ���㣻��δ���У���ȴ����к󴥷��¼�
void D3D12App::FlushCmdQueue()
{
	m_currentFence++;
	// ��GPU������CPU�������������GPUΧ��++
	m_cmdQueue->Signal(m_fence.Get(), m_currentFence);

	if (m_fence->GetCompletedValue() < m_currentFence) { // ��С�ڣ���˵��GPU��δ����������
		HANDLE eventHandle = CreateEvent(nullptr, false, false, L"FenceSetDone");
		m_fence->SetEventOnCompletion(m_currentFence, eventHandle); // ��GPUΧ��������m_currentFence��ֵ�������evenetHandle
		// �ȴ�GPU����Χ���������¼���������ǰ�߳�ֱ���¼�������
		// ���û��Set��Wait���������ˣ�Set��Զ������ã�����Ҳ��û�߳̿��Ի�������̣߳�
		WaitForSingleObject(eventHandle, INFINITE);

		CloseHandle(eventHandle);
	}
}

//10�������ӿںͲü�����
void D3D12App::CreateViewPortAndScissorRect()
{
	//�ӿ�����
	m_viewPort.TopLeftX = 0;
	m_viewPort.TopLeftY = 0;
	m_viewPort.Width = m_clientWidth;
	m_viewPort.Height = m_clientHeight;
	m_viewPort.MaxDepth = 1.0f;
	m_viewPort.MinDepth = 0.0f;
	//�ü��������ã�����������ض������޳���
	//ǰ����Ϊ���ϵ����꣬������Ϊ���µ�����
	m_scissorRect.left = 0;
	m_scissorRect.top = 0;
	m_scissorRect.right = m_clientWidth;
	m_scissorRect.bottom = m_clientHeight;
}

void D3D12App::CalculateFrameState()
{
	static int frameCnt = 0;	//��֡��
	static float timeElapsed = 0.0f;	//���ŵ�ʱ��
	frameCnt++;	//ÿ֡++������һ����伴ΪFPSֵ
	//����ģ��
	/*std::wstring text = std::to_wstring(gt.TotalTime());
	std::wstring windowText = text;
	SetWindowText(mhMainWnd, windowText.c_str());*/
	//�ж�ģ��
	if (m_gt.TotalTime() - timeElapsed >= 1.0f)	//һ��>=0��˵���պù�һ��
	{
		float fps = (float)frameCnt;//ÿ�����֡
		float mspf = 1000.0f / fps;	//ÿ֡���ٺ���

		std::wstring fpsStr = std::to_wstring(fps);//תΪ���ַ�
		std::wstring mspfStr = std::to_wstring(mspf);
		//��֡������ʾ�ڴ�����
		std::wstring windowText = L"D3D12Init    fps:" + fpsStr + L"    " + L"mspf" + mspfStr;
		SetWindowText(m_hwnd, windowText.c_str());

		//Ϊ������һ��֡��ֵ������
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

