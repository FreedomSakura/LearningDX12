#include "D3D12InitApp.h"

//定义顶点结构体
struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};
//实例化顶点结构体并填充
std::array<Vertex, 8> vertices =
{
	Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White) }),
	Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black) }),
	Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) }),
	Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
	Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) }),
	Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow) }),
	Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan) }),
	Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) })
};

std::array<std::uint16_t, 36> indices =
{
	//前
	0, 1, 2,
	0, 2, 3,

	//后
	4, 6, 5,
	4, 7, 6,

	//左
	4, 5, 1,
	4, 1, 0,

	//右
	3, 2, 6,
	3, 6, 7,

	//上
	1, 5, 6,
	1, 6, 2,

	//下
	4, 0, 3,
	4, 3, 7
};

//单个物体的常量数据
struct ObjectConstants
{
	//初始化物体空间变换到裁剪空间矩阵，Identity4x4()是单位矩阵，需要包含MathHelper头文件
	XMFLOAT4X4 worldViewProj = MathHelper::Identity4x4();
};

// 绘制（调用一次就是一帧！）
//功能：将各种资源设置到渲染流水线上，并最终发出绘制命令
bool D3D12InitApp::Draw() {
	//1、重置命令和列表，复用相关内存
	ThrowIfFailed(m_cmdAllocator->Reset()); // 重复使用记录命令的相关内存
	ThrowIfFailed(m_cmdList->Reset(m_cmdAllocator.Get(), nullptr)); // 复用命令列表及其内存

	//2、将后台缓冲资源从呈现状态转换到渲染目标状态（即准备接收图像渲染）
	UINT& ref_mCurrentBackBuffer = m_currentBackBuffer;
	// 转换资源为后台缓冲区资源
	CD3DX12_RESOURCE_BARRIER resBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_swapChainBuffer[ref_mCurrentBackBuffer].Get(),
		D3D12_RESOURCE_STATE_PRESENT, // 当前状态：前台缓冲区，呈现状态
		D3D12_RESOURCE_STATE_RENDER_TARGET // 当前状态：后台缓冲区，渲染目标状态
	);

	m_cmdList->ResourceBarrier(
		1,
		&resBarrier
	);

	//3、设置
	m_cmdList->RSSetViewports(1, &m_viewPort);
	m_cmdList->RSSetScissorRects(1, &m_scissorRect);

	//4、清除后台缓冲区 & 深度缓冲区
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
		ref_mCurrentBackBuffer,
		m_rtvDescriptorSize);
	// 清除RT背景色为淡蓝色，并且不设置裁剪矩形
	m_cmdList->ClearRenderTargetView(rtvHandle, DirectX::Colors::AliceBlue, 0, nullptr);
	// 清除深度缓冲
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
	m_cmdList->ClearDepthStencilView(
		dsvHandle, // DSV描述符句柄
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
		1.0f, // 默认深度值
		0,    // 默认模板值
		0,    // 裁剪矩形数量
		nullptr // 裁剪矩形指针
	);



	//5、指定将要渲染的缓冲区（指定RTV & DSV）
	m_cmdList->OMSetRenderTargets(
		1,    // 待绑定的RTV数量
		&rtvHandle,
		true, // RTV对象在堆内存中是连续存放的
		&dsvHandle
	);


	///////////////////////////////////////////////////////////////
	// 应该是在这里添加渲染代码
	///////////////////////////////////////////////////////////////
	// 设置CBV描述符堆
	ID3D12DescriptorHeap* descriHeaps[] = { m_cbvHeap.Get() };
	m_cmdList->SetDescriptorHeaps(_countof(descriHeaps), descriHeaps);
	// 设置根签名
	m_cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
	// 设置顶点缓冲区
	m_cmdList->IASetVertexBuffers(0, 1, &GetVbv());
	//m_cmdList->IASetIndexBuffer(&GetIbv());
	// 将图元拓扑传入流水线
	m_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// 设置根描述符表
	m_cmdList->SetGraphicsRootDescriptorTable(
		0, // 根参数的起始索引
		m_cbvHeap->GetGPUDescriptorHandleForHeapStart()
	);

	// 绘制顶点！！！！！
	m_cmdList->DrawIndexedInstanced(
		sizeof(indices), // 每个实例要绘制的索引数
		1, // 实例化个数
		0, // 起始索引位置
		0, // 子物体起始索引在全局索引中的位置
		0  // 实例化的高级技术，暂时设为0 
	);


	// 更新一下矩阵？
	Update();




	//6、渲染完成，将后台缓冲区的状态改为呈现状态，将其推向前台缓冲区显示
	resBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_swapChainBuffer[ref_mCurrentBackBuffer].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, // 当前状态：后台缓冲区，渲染目标状态
		D3D12_RESOURCE_STATE_PRESENT // 当前状态：前台缓冲区，呈现状态
	);

	m_cmdList->ResourceBarrier(
		1,
		&resBarrier
	);

	// 7、8两步其实算是一步！关闭Command List后应当将其中命令传到Command Queue中！
	//7、完成命令的记录关闭命令列表
	ThrowIfFailed(m_cmdList->Close());

	//8、等CPU将命令都准备好，将待执行的命令列表加入GPU的命令队列
	ID3D12CommandList* commandLists[] = { m_cmdList.Get() }; // 声明并定义命令列表数组
	m_cmdQueue->ExecuteCommandLists(_countof(commandLists), commandLists); // 将命令从命令列表传至命令队列

	//9、交换前后台缓冲区索引（0变1，1变0）
	//ThrowIfFailed(m_swapChain->Present(0, 0)); -> 第二个参数是Flag，我选择打开垂直同步，所以有下面那些代码
	ThrowIfFailed(m_swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING));
	ref_mCurrentBackBuffer = (ref_mCurrentBackBuffer + 1) % 2;

	//10、设置围栏值，命中围栏就刷新命令队列，使得CPU和GPU同步
	FlushCmdQueue();

	return true;
}


// 这三个缓冲区创建的代码都有点问题，明天（7-10）再改！
// 创建顶点缓冲区描述符（vbv） & 索引缓冲区描述符（ibv）
bool D3D12InitApp::CreateVBV() {
	m_vertexBufferGPU = ToolFunc::CreateDefaultBuffer(m_d3dDevice, m_cmdList, sizeof(vertices), vertices.data(), m_vertexBufferUploader);
	
	// 将顶点数据绑定至渲染流水线
	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = m_vertexBufferGPU->GetGPUVirtualAddress();	// 顶点缓冲区资源的虚拟地址
	vbv.SizeInBytes = sizeof(Vertex) * 8;
	vbv.StrideInBytes = sizeof(Vertex);
	
	// 设置顶点缓冲区
	m_cmdList->IASetVertexBuffers(0, 1, &vbv);

	return true;
}

bool D3D12InitApp::CreateIBV() {
	ThrowIfFailed(D3DCreateBlob(m_ibByteSize, &m_indexBufferCPU));	//创建索引数据内存空间

	CopyMemory(m_indexBufferCPU->GetBufferPointer(), indices.data(), m_ibByteSize);	//将索引数据拷贝至索引系统内存中

	m_indexBufferGPU = ToolFunc::CreateDefaultBuffer(m_d3dDevice, m_cmdList, m_ibByteSize, indices.data(), m_indexBufferUploader);

	D3D12_INDEX_BUFFER_VIEW ibv;
	ibv.BufferLocation = m_indexBufferGPU->GetGPUVirtualAddress();
	ibv.Format = DXGI_FORMAT_R16_UINT;
	ibv.SizeInBytes = m_ibByteSize;
	//设置索引缓冲区
	m_cmdList->IASetIndexBuffer(&ibv);

	return true;
}

// 创建常量描述符
bool D3D12InitApp::CreateCBV() {
	UINT objConstSize = CalcConstantBufferByteSize(sizeof(ObjectConstants));
	// 创建CBV堆
	D3D12_DESCRIPTOR_HEAP_DESC cbHeapDesc;
	cbHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbHeapDesc.NumDescriptors = 1;
	cbHeapDesc.NodeMask = 0;
	ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&cbHeapDesc, IID_PPV_ARGS(&m_cbvHeap)));

	// 定义并获得物体的常量缓冲区，然后得到其首地址
	// elementCount为1（1个子物体常量缓冲元素），isConstantBuffer为ture（是常量缓冲区）
	m_objCB = std::make_unique<UploadBufferResource<ObjectConstants>>(m_d3dDevice.Get(), 1, true);
	// 获得常量缓冲区首地址
	D3D12_GPU_VIRTUAL_ADDRESS address;
	address = m_objCB->Resource()->GetGPUVirtualAddress();
	// 通过常量缓冲区元素偏移值计算最终的元素地址
	int cbElementIndex = 0;	//常量缓冲区元素下标
	address += cbElementIndex * objConstSize;
	// 创建CBV描述符
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = address;
	cbvDesc.SizeInBytes = objConstSize;
	m_d3dDevice->CreateConstantBufferView(&cbvDesc, m_cbvHeap->GetCPUDescriptorHandleForHeapStart());

	return true;
}

// 创建根签名
void D3D12InitApp::BuildRootSignature() {
	// 根参数可以是描述符表、根描述符、根常量
	CD3DX12_ROOT_PARAMETER slotRootParameter[1];
	// 创建由单个CBV所组成的描述符表
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV, // 描述符类型
		1,// 描述符数量
		0 // 描述符所绑定的寄存器槽号（有点像TEXTURE0这样？）
	);

	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);

	// 根签名由一组根参数构成
	CD3DX12_ROOT_SIGNATURE_DESC rootSig(
		1, // 根参数数量
		slotRootParameter, // 根参数指针
		0,
		nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	// 用单个寄存器槽来创建一个根签名，该槽位指向一个含有单个常量缓冲区的描述符区域
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(
		&rootSig, 
		D3D_ROOT_SIGNATURE_VERSION_1, 
		&serializedRootSig, 
		&errorBlob
	);


	if (errorBlob != nullptr)
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());

	ThrowIfFailed(hr);

	auto p = serializedRootSig;

	ThrowIfFailed(
		m_d3dDevice->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(&m_rootSignature)
		)
	);
}

// 将顶点&索引数据复制到CPU系统内存，再使用CreateDefaultBuffer将其复制到GPU缓存中
void D3D12InitApp::BuildGeometry() {
	m_vbByteSize = sizeof(Vertex) * 8;
	m_ibByteSize = sizeof(int) * 36;
	// 创建内存空间
	ThrowIfFailed(D3DCreateBlob(m_vbByteSize, m_vertexBufferCPU.GetAddressOf()));
	ThrowIfFailed(D3DCreateBlob(m_ibByteSize, m_indexBufferCPU.GetAddressOf()));
	//ThrowIfFailed(D3DCreateBlob(m_vbByteSize, &m_vertexBufferCPU));
	//ThrowIfFailed(D3DCreateBlob(m_ibByteSize, &m_indexBufferCPU));
	// 复制到CPU系统内存
	CopyMemory(m_vertexBufferCPU->GetBufferPointer(), vertices.data(), m_vbByteSize);
	CopyMemory(m_indexBufferCPU->GetBufferPointer(), indices.data(), m_ibByteSize);
	// 拷贝数据到GPU缓存中
	m_vertexBufferGPU = ToolFunc::CreateDefaultBuffer(m_d3dDevice, m_cmdList, m_vbByteSize, vertices.data(), m_vertexBufferUploader);
	m_indexBufferGPU = ToolFunc::CreateDefaultBuffer(m_d3dDevice, m_cmdList, m_ibByteSize, indices.data(), m_indexBufferUploader);
}

// 构建PSO（PipeLineStateObject）
// 将之前定义的各种东西绑定到渲染流水线上！
void D3D12InitApp::BuildPSO() {
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { m_inputLayoutDesc.data(), (UINT)m_inputLayoutDesc.size() };
	psoDesc.pRootSignature = m_rootSignature.Get();
	psoDesc.VS = { reinterpret_cast<BYTE*>(m_vsBytecode->GetBufferPointer()), m_vsBytecode->GetBufferSize() };
	psoDesc.PS = { reinterpret_cast<BYTE*>(m_psBytecode->GetBufferPointer()), m_psBytecode->GetBufferSize() };
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;	//0xffffffff,全部采样，没有遮罩
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;	//归一化的无符号整型
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;	//不使用4XMSAA
	psoDesc.SampleDesc.Quality = 0;	////不使用4XMSAA

	
	ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_PSO)));
}

void D3D12InitApp::BuildByteCodeAndInputLayout() {
	// 输入布局
	m_inputLayoutDesc =
	{
		  { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		  { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// 将.hlsl文件编译为字节码
	HRESULT hr = S_OK;
	m_vsBytecode = ToolFunc::CompileShader(L"Shaders\\color.hlsl", nullptr, "VS", "vs_5_0");
	m_psBytecode = ToolFunc::CompileShader(L"Shaders\\color.hlsl", nullptr, "PS", "ps_5_0");
}

bool D3D12InitApp::Init(HINSTANCE hInstance, int nShowCmd) {
	// 打开D3D12调试层
#if defined(DEBUG) | defined(_DEBUG)
	ComPtr<ID3D12Debug> debugController;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
	debugController->EnableDebugLayer();
#endif

	if (!D3D12App::Init(hInstance, nShowCmd))
		return false;

	//CreateVBV();
	//CreateIBV();

	CreateCBV();
	BuildRootSignature();
	BuildByteCodeAndInputLayout();
	BuildGeometry();
	BuildPSO();

	// 这里关闭命令列表会抛出异常，貌似是说已经关闭了？
	//ThrowIfFailed(m_cmdList->Close());
	ID3D12CommandList* cmdLists[] = { m_cmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	FlushCmdQueue();

	return true;
}

// 获取顶点缓冲区描述符 & 索引缓冲区描述符
D3D12_VERTEX_BUFFER_VIEW D3D12InitApp::GetVbv() const {
	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = m_vertexBufferGPU->GetGPUVirtualAddress();
	vbv.SizeInBytes = m_vbByteSize;
	vbv.StrideInBytes = sizeof(Vertex);

	return vbv;
}

D3D12_INDEX_BUFFER_VIEW D3D12InitApp::GetIbv() const {
	D3D12_INDEX_BUFFER_VIEW ibv;
	ibv.BufferLocation = m_indexBufferGPU->GetGPUVirtualAddress();
	ibv.Format = DXGI_FORMAT_R16_UINT;
	ibv.SizeInBytes = m_ibByteSize;

	return ibv;
}

void D3D12InitApp::Update()
{
	ObjectConstants objConstants;
	//构建观察矩阵
	float x = 0.0f;
	float y = 0.0f;
	float z = 5.0f;
	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX v = XMMatrixLookAtLH(pos, target, up);

	//构建投影矩阵
	XMMATRIX p = XMMatrixPerspectiveFovLH(0.25f * 3.1416f, 1280.0f / 720.0f, 1.0f, 1000.0f);
	//XMStoreFloat4x4(&proj, p);
	//构建世界矩阵
	//XMMATRIX w = XMLoadFloat4x4(&world);
	//矩阵计算
	XMMATRIX WVP_Matrix = v * p;
	//XMMATRIX赋值给XMFLOAT4X4
	XMStoreFloat4x4(&objConstants.worldViewProj, XMMatrixTranspose(WVP_Matrix));
	//将数据拷贝至GPU缓存
	m_objCB->CopyData(0, objConstants);
}