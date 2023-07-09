#include "D3DAPP.h"

//1�������豸
void D3DAPP::CreateDevice()
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
void D3DAPP::CreateFence()
{
	ThrowIfFailed(m_d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
}

//3����ȡ��������С�����ǻ�ȡ���ֻ�������Ԫ�صĴ�С��������Ȼ�������һ��pixel�Ĵ�С�Ƕ��٣���
void D3DAPP::GetDescriptorSize()
{
	UINT rtvDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	UINT dsvDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	UINT cbv_srv_uavDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

//4������MSAA����
void D3DAPP::SetMSAA()
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
void D3DAPP::CreateCommandObject()
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
void D3DAPP::CreateSwapChain(HWND hwnd)
{
	m_swapChain.Reset();

	DXGI_SWAP_CHAIN_DESC swapChainDesc;// �����������ṹ��
	swapChainDesc.BufferDesc.Width = 1280;
	swapChainDesc.BufferDesc.Height = 720;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // ����������ʾ��ʽ
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;	//ˢ���ʵķ���
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;	//ˢ���ʵķ�ĸ
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED; // ����ɨ��VS����ɨ��
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED; //ͼ�������Ļ�����죨δָ���ģ�
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // ��������Ⱦ����̨������������Ϊ����Ŀ�꣩
	swapChainDesc.OutputWindow = hwnd; // ��Ⱦ���ڵľ��;
	swapChainDesc.SampleDesc.Count = 1; // ���������
	swapChainDesc.SampleDesc.Quality = 0; // ���������
	swapChainDesc.Windowed = true; // �Ƿ񴰿ڻ�
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.BufferCount = 2; // ��̨������������˫���壡��
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	// ����DXGI�ӿ��µĹ����ഴ��������
	ThrowIfFailed(m_dxgiFactory->CreateSwapChain(m_cmdQueue.Get(), &swapChainDesc, m_swapChain.GetAddressOf()));
}

//7�������������ѣ�DescriptorHeap)
void D3DAPP::CreateDescriptorHeap() {
	// �ȴ���RTV
	D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc;
	rtvDescriptorHeapDesc.NumDescriptors = 2;
	rtvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvDescriptorHeapDesc.NodeMask = 0;

	ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

	// ��ȡrtv�������Ĵ�С -> ����ȡ�ͻ��ò���...
	// û�����У�һֱ��cmdList->Close()��ʱ���׳��쳣˵һ�������б���ִ�ж��������ʲô��...
	m_rtvDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

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
void D3DAPP::CreateRTV()
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
void D3DAPP::CreateDSV()
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


	//// ���������������󣬽�����������б���������У�Ҳ����CPU����GPU�Ĺ���
	//ThrowIfFailed(m_cmdList->Close());
	//ID3D12CommandList* cmdLists[] = { m_cmdList.Get() };
	//m_cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
}

//9��ʵ��Χ����ʵ��CPU��GPU��ͬ����
// ˼·����ʼ��Χ��ֵΪ0����DX12APIά���� -> �ٴ���һ����ǰΧ��ֵҲ��0��ʼ�������Լ���д������ά����
// CPU�������GPU, m_currentFence++��CPUΧ��++��;
// GPU������CPU�����������Χ��ֵ++��GPUΧ��++��;
// Ȼ���ж�CPUΧ��ֵ��GPUΧ��ֵ�Ĵ�С����ȷ��GPU�Ƿ�����Χ���㣻��δ���У���ȴ����к󴥷��¼�
void D3DAPP::FlushCmdQueue()
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
void D3DAPP::CreateViewPortAndScissorRect()
{
	//�ӿ�����
	m_viewPort.TopLeftX = 0;
	m_viewPort.TopLeftY = 0;
	m_viewPort.Width = 1280;
	m_viewPort.Height = 720;
	m_viewPort.MaxDepth = 1.0f;
	m_viewPort.MinDepth = 0.0f;
	//�ü��������ã�����������ض������޳���
	//ǰ����Ϊ���ϵ����꣬������Ϊ���µ�����
	m_scissorRect.left = 0;
	m_scissorRect.top = 0;
	m_scissorRect.right = 1280;
	m_scissorRect.bottom = 720;
}

bool D3DAPP::InitD3DPipeline(HWND hwnd) {
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
	CreateSwapChain(hwnd);

	CreateDescriptorHeap();
	CreateRTV();
	CreateDSV();

	CreateViewPortAndScissorRect();

	return true;
}

// ���ƣ�����һ�ξ���һ֡����
//���ܣ���������Դ���õ���Ⱦ��ˮ���ϣ������շ�����������
bool D3DAPP::Draw() {
	//1������������б���������ڴ�
	ThrowIfFailed(m_cmdAllocator->Reset()); // �ظ�ʹ�ü�¼���������ڴ�
	ThrowIfFailed(m_cmdList->Reset(m_cmdAllocator.Get(), nullptr)); // ���������б����ڴ�

	//2������̨������Դ�ӳ���״̬ת������ȾĿ��״̬����׼������ͼ����Ⱦ��
	UINT& ref_mCurrentBackBuffer = m_currentBackBuffer;
	// ת����ԴΪ��̨��������Դ
	CD3DX12_RESOURCE_BARRIER resBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_swapChainBuffer[ref_mCurrentBackBuffer].Get(),
		D3D12_RESOURCE_STATE_PRESENT, // ��ǰ״̬��ǰ̨������������״̬
		D3D12_RESOURCE_STATE_RENDER_TARGET // ��ǰ״̬����̨����������ȾĿ��״̬
	);

	m_cmdList->ResourceBarrier(
		1,
		&resBarrier
	);

	//3������
	m_cmdList->RSSetViewports(1, &m_viewPort);
	m_cmdList->RSSetScissorRects(1, &m_scissorRect);

	//4�������̨������ & ��Ȼ�����
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
		ref_mCurrentBackBuffer,
		m_rtvDescriptorSize);
	// ���RT����ɫΪ���죬���Ҳ����òü�����
	m_cmdList->ClearRenderTargetView(rtvHandle, DirectX::Colors::DarkRed, 0, nullptr);
	// �����Ȼ���
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
	m_cmdList->ClearDepthStencilView(
		dsvHandle, // DSV���������
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
		1.0f, // Ĭ�����ֵ
		0,    // Ĭ��ģ��ֵ
		0,    // �ü���������
		nullptr // �ü�����ָ��
	);

	//5��ָ����Ҫ��Ⱦ�Ļ�������ָ��RTV & DSV��
	m_cmdList->OMSetRenderTargets(
		1,    // ���󶨵�RTV����
		&rtvHandle,
		true, // RTV�����ڶ��ڴ�����������ŵ�
		&dsvHandle
	);

	//6����Ⱦ��ɣ�����̨��������״̬��Ϊ����״̬����������ǰ̨��������ʾ
	resBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_swapChainBuffer[ref_mCurrentBackBuffer].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, // ��ǰ״̬����̨����������ȾĿ��״̬
		D3D12_RESOURCE_STATE_PRESENT // ��ǰ״̬��ǰ̨������������״̬
	);

	m_cmdList->ResourceBarrier(
		1,
		&resBarrier
	);

	//7���������ļ�¼�ر������б�
	ThrowIfFailed(m_cmdList->Close());

	//8����CPU�����׼���ã�����ִ�е������б����GPU���������
	ID3D12CommandList* commandLists[] = { m_cmdList.Get() }; // ���������������б�����
	m_cmdQueue->ExecuteCommandLists(_countof(commandLists), commandLists); // ������������б����������

	//9������ǰ��̨������������0��1��1��0��
	ThrowIfFailed(m_swapChain->Present(0, 0));
	ref_mCurrentBackBuffer = (ref_mCurrentBackBuffer + 1) % 2;

	//10������Χ��ֵ������Χ����ˢ��������У�ʹ��CPU��GPUͬ��
	FlushCmdQueue();
}