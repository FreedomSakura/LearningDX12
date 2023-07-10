#include "D3D12App.h"

bool D3D12App::Draw() {
	return true;
}

// win32相关
bool D3D12App::Init(HINSTANCE hInstance, int nShowCmd) {
	InitWindow(hInstance, nShowCmd);
	InitD3DPipeline();

	return true;
}

bool D3D12App::InitWindow(HINSTANCE hInstance, int nShowCmd) {
	//窗口初始化描述结构体(WNDCLASS)
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;	//当工作区宽高改变，则重新绘制窗口
	wc.lpfnWndProc = MainWndProc;	//指定窗口过程
	wc.cbClsExtra = 0;	//借助这两个字段来为当前应用分配额外的内存空间（这里不分配，所以置0）
	wc.cbWndExtra = 0;	//借助这两个字段来为当前应用分配额外的内存空间（这里不分配，所以置0）
	wc.hInstance = hInstance;	//应用程序实例句柄（由WinMain传入）
	wc.hIcon = LoadIcon(0, IDC_ARROW);	//使用默认的应用程序图标
	wc.hCursor = LoadCursor(0, IDC_ARROW);	//使用标准的鼠标指针样式
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);	//指定了白色背景画刷句柄
	wc.lpszMenuName = 0;	//没有菜单栏
	wc.lpszClassName = L"MainWnd";	//窗口名
	//窗口类注册失败
	if (!RegisterClass(&wc))
	{
		//消息框函数，参数1：消息框所属窗口句柄，可为NULL。参数2：消息框显示的文本信息。参数3：标题文本。参数4：消息框样式
		MessageBox(0, L"RegisterClass Failed", 0, 0);
		return 0;
	}

	//窗口类注册成功
	RECT R;	//裁剪矩形
	R.left = 0;
	R.top = 0;
	R.right = 1280;
	R.bottom = 720;
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);	//根据窗口的客户区大小计算窗口的大小
	int width = R.right - R.left;
	int hight = R.bottom - R.top;

	//创建窗口,返回布尔值
	m_hwnd = CreateWindow(L"MainWnd", L"DX12Initialize", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, hight, 0, 0, hInstance, 0);
	//窗口创建失败
	if (!m_hwnd)
	{
		MessageBox(0, L"CreatWindow Failed", 0, 0);
		return 0;
	}
	//窗口创建成功,则显示并更新窗口
	ShowWindow(m_hwnd, nShowCmd);
	UpdateWindow(m_hwnd);

	return true;
}

//1、创建设备
void D3D12App::CreateDevice()
{
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&m_dxgiFactory)));

	ThrowIfFailed(
		D3D12CreateDevice(
			nullptr,// 默认设备，也就是主显示器
			D3D_FEATURE_LEVEL_12_0,// 应用程序需要硬件支持的最低DX版本
			IID_PPV_ARGS(&m_d3dDevice) // 返回创建的设备
		)
	);

}

//2、创建围栏fence，用于同步CPU和GPU
void D3D12App::CreateFence()
{
	ThrowIfFailed(m_d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
}

//3、获取描述符大小（就是获取各种缓冲区的元素的大小，比如深度缓冲区的一个pixel的大小是多少？）
void D3D12App::GetDescriptorSize()
{
	m_rtvDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_dsvDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	m_cbv_srv_uavDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

//4、设置MSAA属性
void D3D12App::SetMSAA()
{
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaQualityLevels;
	msaaQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;	// UNORM是归一化处理的无符号整数
	msaaQualityLevels.SampleCount = 1;
	msaaQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msaaQualityLevels.NumQualityLevels = 0;// 不开启MSAA，采样数设置为0
	//当前图形驱动对MSAA多重采样的支持（注意：第二个参数即是输入又是输出）
	ThrowIfFailed(m_d3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msaaQualityLevels, sizeof(msaaQualityLevels)));
	//NumQualityLevels在Check函数里会进行设置
	//如果支持MSAA，则Check函数返回的NumQualityLevels > 0
	//expression为假（即为0），则终止程序运行，并打印一条出错信息
	assert(msaaQualityLevels.NumQualityLevels > 0);
}

//5、创建命令队列、命令列表、命令分配器
// 三者的关系： CPU创建命令列表 -> 将关联在命令分配器上的命令传入命令列表 -> 最后将命令传入命令队列给GPU处理
// 这里只是做了创建三者的工作而已
void D3D12App::CreateCommandObject()
{
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	ThrowIfFailed(m_d3dDevice->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&m_cmdQueue)));

	ThrowIfFailed(m_d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_cmdAllocator)));//&cmdAllocator等价于cmdAllocator.GetAddressOf

	ThrowIfFailed(
		m_d3dDevice->CreateCommandList(
			0,// 掩码值为0，单GPU
			D3D12_COMMAND_LIST_TYPE_DIRECT,// 命令列表类型
			m_cmdAllocator.Get(),//命令分配器接口指针
			nullptr,// 流水线状态对象PSO，这里不绘制，所以nullptr
			IID_PPV_ARGS(&m_cmdList)// 返回创建的命令列表
		)
	);
	m_cmdList->Close();// 重置命令列表前必须将其关闭
}

//6、创建交换链
void D3D12App::CreateSwapChain()
{
	m_swapChain.Reset();

	DXGI_SWAP_CHAIN_DESC swapChainDesc;// 交换链描述结构体
	swapChainDesc.BufferDesc.Width = 1280;
	swapChainDesc.BufferDesc.Height = 720;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 缓冲区的显示格式
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;	//刷新率的分子
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;	//刷新率的分母
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED; // 逐行扫描VS隔行扫描
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED; //图像相对屏幕的拉伸（未指定的）
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 将数据渲染至后台缓冲区（即作为缓冲目标）
	swapChainDesc.OutputWindow = m_hwnd; // 渲染窗口的句柄;
	swapChainDesc.SampleDesc.Count = 1; // 多采样数量
	swapChainDesc.SampleDesc.Quality = 0; // 多采样质量
	swapChainDesc.Windowed = true; // 是否窗口化
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.BufferCount = 2; // 后台缓冲区数量（双缓冲！）
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
	// 利用DXGI接口下的工厂类创建交换链
	ThrowIfFailed(m_dxgiFactory->CreateSwapChain(m_cmdQueue.Get(), &swapChainDesc, m_swapChain.GetAddressOf()));
}

//7、创建描述符堆（DescriptorHeap)
void D3D12App::CreateDescriptorHeap() {
	// 先创建RTV
	D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc;
	rtvDescriptorHeapDesc.NumDescriptors = 2;
	rtvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvDescriptorHeapDesc.NodeMask = 0;

	ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

	// 获取rtv描述符的大小 -> 不获取就会变得不幸...（其实这一步应该在前面第三步获取描述符大小时就做了！）
	// 没有这行，一直在cmdList->Close()的时候抛出异常说一个命令列表不能执行多个交换链什么的...
	//m_rtvDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// 再创建DSV
	D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc;
	dsvDescriptorHeapDesc.NumDescriptors = 1;
	dsvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvDescriptorHeapDesc.NodeMask = 0;

	ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&dsvDescriptorHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));
}

//8、创建描述符
//（1）RTV描述符
// 从RTV堆中拿到RTV句柄 -> 从交换链上拿到RT资源 -> 创建RTV将RT资源和RTV句柄关联起来 -> 根据RTV大小在堆中做偏移，以便获取下个RTV 
void D3D12App::CreateRTV()
{
	// 获取rtv句柄
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

	for (int i = 0; i < 2; i++)
	{
		// 从交换链上获取后台缓冲区资源，并写入rtv缓冲区
		m_swapChain->GetBuffer(i, IID_PPV_ARGS(m_swapChainBuffer[i].GetAddressOf()));
		// 创建RTV
		m_d3dDevice->CreateRenderTargetView(
			m_swapChainBuffer[i].Get(),
			nullptr,
			rtvHeapHandle
		);
		// 
		rtvHeapHandle.Offset(1, m_rtvDescriptorSize);
	}
}


//（2）DSV描述符
void D3D12App::CreateDSV()
{
	//在CPU中创建好深度模板数据资源
	D3D12_RESOURCE_DESC dsvResourceDesc;
	dsvResourceDesc.Alignment = 0;	//指定对齐
	dsvResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;	//指定资源维度（类型）为TEXTURE2D
	dsvResourceDesc.DepthOrArraySize = 1;	//纹理深度为1
	dsvResourceDesc.Width = 1280;	//资源宽
	dsvResourceDesc.Height = 720;	//资源高
	dsvResourceDesc.MipLevels = 1;	//MIPMAP层级数量
	dsvResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;	//指定纹理布局（这里不指定）
	dsvResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;	//深度模板资源的Flag
	dsvResourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;	//24位深度，8位模板,还有个无类型的格式DXGI_FORMAT_R24G8_TYPELESS也可以使用
	dsvResourceDesc.SampleDesc.Count = 1;	//多重采样数量
	//dsvResourceDesc.SampleDesc.Quality = msaaQualityLevels.NumQualityLevels - 1;	//多重采样质量
	dsvResourceDesc.SampleDesc.Quality = 0;	//多重采样质量
	CD3DX12_CLEAR_VALUE optClear;	//清除资源的优化值，提高清除操作的执行速度（CreateCommittedResource函数中传入）
	optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;//24位深度，8位模板,还有个无类型的格式DXGI_FORMAT_R24G8_TYPELESS也可以使用
	optClear.DepthStencil.Depth = 1;	//初始深度值为1
	optClear.DepthStencil.Stencil = 0;	//初始模板值为0

	// 创建一个资源和一个堆，并将资源提交至堆中（将深度模板数据提交至GPU显存中）
	// 将CPU中的DSV资源绑定到GPU上
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

	// 创建DSV(必须填充DSV属性结构体，和创建RTV不同，RTV是通过句柄)
	//D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	//dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	//dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	//dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	//dsvDesc.Texture2D.MipSlice = 0;
	m_d3dDevice->CreateDepthStencilView(
		m_depthStencilBuffer.Get(),
		nullptr, //D3D12_DEPTH_STENCIL_VIEW_DESC类型指针，可填&dsvDesc（见上注释代码），
				 //由于在创建深度模板资源时已经定义深度模板数据属性，所以这里可以指定为空指针
		m_dsvHeap->GetCPUDescriptorHandleForHeapStart()
	);

	// 标记DS资源的状态（因为深度缓冲区一会儿可以写入，一会儿又是只读的...[可被调配]） -> 将资源从初始化状态转为深度缓冲区
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_depthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, //转换前状态（创建时的状态，即CreateCommittedResource函数中定义的状态）
		D3D12_RESOURCE_STATE_DEPTH_WRITE //转换后状态为可写入的深度图，还有一个D3D12_RESOURCE_STATE_DEPTH_READ是只可读的深度图
	);
	m_cmdList->ResourceBarrier(
		1,	//Barrier屏障个数
		&barrier
	);
}

//9、实现围栏（实现CPU与GPU的同步）
// 思路：初始化围栏值为0（由DX12API维护） -> 再创建一个当前围栏值也从0开始（我们自己编写的类来维护）
// CPU将命令传到GPU, m_currentFence++（CPU围栏++）;
// GPU处理完CPU传来的命令后，围栏值++（GPU围栏++）;
// 然后判定CPU围栏值和GPU围栏值的大小，来确定GPU是否命中围栏点；若未命中，则等待命中后触发事件
void D3D12App::FlushCmdQueue()
{
	m_currentFence++;
	// 当GPU处理完CPU发来的命令后，让GPU围栏++
	m_cmdQueue->Signal(m_fence.Get(), m_currentFence);

	if (m_fence->GetCompletedValue() < m_currentFence) { // 若小于，则说明GPU还未处理完命令
		HANDLE eventHandle = CreateEvent(nullptr, false, false, L"FenceSetDone");
		m_fence->SetEventOnCompletion(m_currentFence, eventHandle); // 当GPU围栏到达了m_currentFence的值，则调用evenetHandle
		// 等待GPU命中围栏，触发事件（阻塞当前线程直到事件触发）
		// 如果没有Set就Wait，就死锁了，Set永远不会调用，所以也就没线程可以唤醒这个线程）
		WaitForSingleObject(eventHandle, INFINITE);

		CloseHandle(eventHandle);
	}
}

//10、设置视口和裁剪矩形
void D3D12App::CreateViewPortAndScissorRect()
{
	//视口设置
	m_viewPort.TopLeftX = 0;
	m_viewPort.TopLeftY = 0;
	m_viewPort.Width = 1280;
	m_viewPort.Height = 720;
	m_viewPort.MaxDepth = 1.0f;
	m_viewPort.MinDepth = 0.0f;
	//裁剪矩形设置（矩形外的像素都将被剔除）
	//前两个为左上点坐标，后两个为右下点坐标
	m_scissorRect.left = 0;
	m_scissorRect.top = 0;
	m_scissorRect.right = 1280;
	m_scissorRect.bottom = 720;
}

bool D3D12App::InitD3DPipeline() {
// 打开D3D12调试层
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

//// 绘制（调用一次就是一帧！）
////功能：将各种资源设置到渲染流水线上，并最终发出绘制命令
//bool D3D12App::Draw() {
//	//1、重置命令和列表，复用相关内存
//	ThrowIfFailed(m_cmdAllocator->Reset()); // 重复使用记录命令的相关内存
//	ThrowIfFailed(m_cmdList->Reset(m_cmdAllocator.Get(), nullptr)); // 复用命令列表及其内存
//
//	//2、将后台缓冲资源从呈现状态转换到渲染目标状态（即准备接收图像渲染）
//	UINT& ref_mCurrentBackBuffer = m_currentBackBuffer;
//	// 转换资源为后台缓冲区资源
//	CD3DX12_RESOURCE_BARRIER resBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
//		m_swapChainBuffer[ref_mCurrentBackBuffer].Get(),
//		D3D12_RESOURCE_STATE_PRESENT, // 当前状态：前台缓冲区，呈现状态
//		D3D12_RESOURCE_STATE_RENDER_TARGET // 当前状态：后台缓冲区，渲染目标状态
//	);
//
//	m_cmdList->ResourceBarrier(
//		1,
//		&resBarrier
//	);
//
//	//3、设置
//	m_cmdList->RSSetViewports(1, &m_viewPort);
//	m_cmdList->RSSetScissorRects(1, &m_scissorRect);
//
//	//4、清除后台缓冲区 & 深度缓冲区
//	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
//		ref_mCurrentBackBuffer,
//		m_rtvDescriptorSize);
//	// 清除RT背景色为暗红，并且不设置裁剪矩形
//	m_cmdList->ClearRenderTargetView(rtvHandle, DirectX::Colors::DarkRed, 0, nullptr);
//	// 清除深度缓冲
//	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
//	m_cmdList->ClearDepthStencilView(
//		dsvHandle, // DSV描述符句柄
//		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
//		1.0f, // 默认深度值
//		0,    // 默认模板值
//		0,    // 裁剪矩形数量
//		nullptr // 裁剪矩形指针
//	);
//
//	//5、指定将要渲染的缓冲区（指定RTV & DSV）
//	m_cmdList->OMSetRenderTargets(
//		1,    // 待绑定的RTV数量
//		&rtvHandle,
//		true, // RTV对象在堆内存中是连续存放的
//		&dsvHandle
//	);
//
//	//6、渲染完成，将后台缓冲区的状态改为呈现状态，将其推向前台缓冲区显示
//	resBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
//		m_swapChainBuffer[ref_mCurrentBackBuffer].Get(),
//		D3D12_RESOURCE_STATE_RENDER_TARGET, // 当前状态：后台缓冲区，渲染目标状态
//		D3D12_RESOURCE_STATE_PRESENT // 当前状态：前台缓冲区，呈现状态
//	);
//
//	m_cmdList->ResourceBarrier(
//		1,
//		&resBarrier
//	);
//
//	// 7、8两步其实算是一步！关闭Command List后应当将其中命令传到Command Queue中！
//	//7、完成命令的记录关闭命令列表
//	ThrowIfFailed(m_cmdList->Close());
//
//	//8、等CPU将命令都准备好，将待执行的命令列表加入GPU的命令队列
//	ID3D12CommandList* commandLists[] = { m_cmdList.Get() }; // 声明并定义命令列表数组
//	m_cmdQueue->ExecuteCommandLists(_countof(commandLists), commandLists); // 将命令从命令列表传至命令队列
//
//	//9、交换前后台缓冲区索引（0变1，1变0）
//	//ThrowIfFailed(m_swapChain->Present(0, 0)); -> 第二个参数是Flag，我选择打开垂直同步，所以有下面那些代码
//	ThrowIfFailed(m_swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING));
//	ref_mCurrentBackBuffer = (ref_mCurrentBackBuffer + 1) % 2;
//
//	//10、设置围栏值，命中围栏就刷新命令队列，使得CPU和GPU同步
//	FlushCmdQueue();
//}

void D3D12App::CalculateFrameState()
{
	static int frameCnt = 0;	//总帧数
	static float timeElapsed = 0.0f;	//流逝的时间
	frameCnt++;	//每帧++，经过一秒后其即为FPS值
	//调试模块
	/*std::wstring text = std::to_wstring(gt.TotalTime());
	std::wstring windowText = text;
	SetWindowText(mhMainWnd, windowText.c_str());*/
	//判断模块
	if (m_gt.TotalTime() - timeElapsed >= 1.0f)	//一旦>=0，说明刚好过一秒
	{
		float fps = (float)frameCnt;//每秒多少帧
		float mspf = 1000.0f / fps;	//每帧多少毫秒

		std::wstring fpsStr = std::to_wstring(fps);//转为宽字符
		std::wstring mspfStr = std::to_wstring(mspf);
		//将帧数据显示在窗口上
		std::wstring windowText = L"D3D12Init    fps:" + fpsStr + L"    " + L"mspf" + mspfStr;
		SetWindowText(m_hwnd, windowText.c_str());

		//为计算下一组帧数值而重置
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}


int D3D12App::Run()
{
	MSG msg = { 0 };

	//每次循环开始都要重置计时器
	m_gt.Reset();

	// 主消息循环:
	while (msg.message != WM_QUIT)
	{
		// 有窗口消息就处理
		//if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// 否则就继续执行渲染 & 游戏逻辑
		else
		{
			Draw();

			m_gt.Tick();	//计算每两帧间隔时间
			if (!m_gt.IsStoped())//如果不是暂停状态，我们才运行游戏
			{
				CalculateFrameState();
				Draw();
			}
			//如果是暂停状态，则休眠100秒
			else
			{
				Sleep(100);
			}
		}
	}

	return (int)msg.wParam;
}