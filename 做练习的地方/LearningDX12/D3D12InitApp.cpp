#include "D3D12InitApp.h"

struct VPosData
{
	XMFLOAT3 Pos;
};

struct VColorData
{
	XMFLOAT4 Color;
};

// 常量缓冲区中要存放的数据
struct ObjectConstants
{
	XMFLOAT4X4 worldViewProj = MathHelper::Identity4x4();
	float gTime = 0.0f;
};

bool D3D12InitApp::Init(HINSTANCE hInstance, int nShowCmd) {
#if defined(DEBUG) | defined(_DEBUG)
	ComPtr<ID3D12Debug> debugController;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
	debugController->EnableDebugLayer();
#endif

	if (!D3D12App::Init(hInstance, nShowCmd))
		return false;

	ThrowIfFailed(m_cmdList->Reset(m_cmdAllocator.Get(), nullptr));

	CreateCBV();
	BuildRootSignature();
	BuildByteCodeAndInputLayout();
	BuildGeometry();
	BuildPSO();

	// 记得关闭命令队列！
	ThrowIfFailed(m_cmdList->Close());
	ID3D12CommandList* cmdLists[] = { m_cmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	FlushCmdQueue();

	return true;
}

void D3D12InitApp::Update()
{
	ObjectConstants objConstants;
	float x = m_radius * sinf(m_phi) * cosf(m_theta);
	float y = m_radius * cosf(m_phi);
	float z = m_radius * sinf(m_phi) * sinf(m_theta);
	float r = 5.0f;
	//x *= sinf(m_gt.TotalTime());
	//z = sqrt(r * r - x * x);
	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();

	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	//XMVECTOR up = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
	XMMATRIX v = XMMatrixLookAtLH(pos, target, up);

	//����ͶӰ����
	XMMATRIX p = XMMatrixPerspectiveFovLH(0.25f * 3.1416f, 1280.0f / 720.0f, 1.0f, 1000.0f);
	//XMStoreFloat4x4(&proj, p);
	//XMMATRIX w = XMLoadFloat4x4(&world);
	XMMATRIX WVP_Matrix = v * p;
	XMStoreFloat4x4(&objConstants.worldViewProj, XMMatrixTranspose(WVP_Matrix));
	m_objCB->CopyData(0, objConstants);
	// 将时间传到m_objCB中
	//objConstants.gTime = m_gt.TotalTime();
}

void D3D12InitApp::OnMouseDown(WPARAM btnState, int x, int y) {
	m_lastMousePos.x = x;	
	m_lastMousePos.y = y;	

	SetCapture(m_hwnd);
}

void D3D12InitApp::OnMouseUp(WPARAM btnState, int x, int y) {
	ReleaseCapture();
}

void D3D12InitApp::OnMouseMove(WPARAM btnState, int x, int y) {
	if ((btnState & MK_LBUTTON) != 0)
	{
		float dx = XMConvertToRadians(static_cast<float>(m_lastMousePos.x - x) * 0.25f);
		float dy = XMConvertToRadians(static_cast<float>(m_lastMousePos.y - y) * 0.25f);
		m_theta += dx;
		m_phi += dy;
		m_theta = MathHelper::Clamp(m_theta, 0.1f, 3.1416f - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		float dx = 0.005f * static_cast<float>(x - m_lastMousePos.x);
		float dy = 0.005f * static_cast<float>(y - m_lastMousePos.y);
		m_radius += dx - dy;
		m_radius = MathHelper::Clamp(m_radius, 1.0f, 20.0f);
	}
	m_lastMousePos.x = x;
	m_lastMousePos.y = y;
}

void D3D12InitApp::OnResize() {
	D3D12App::OnResize();

	XMMATRIX p = XMMatrixPerspectiveFovLH(0.25f * 3.1416f, static_cast<float>(m_clientWidth) / m_clientHeight, 1.0f, 1000.0f);
	XMStoreFloat4x4(&m_proj, p);
}

bool D3D12InitApp::Draw() {
	//1、重置命令列表 & 命令分配器
	ThrowIfFailed(m_cmdAllocator->Reset()); 
	ThrowIfFailed(m_cmdList->Reset(m_cmdAllocator.Get(), m_PSO.Get())); // 记得传入PSO

	UINT& ref_mCurrentBackBuffer = m_currentBackBuffer;
	//2、切换后台缓冲区状态（设置资源屏障）
	CD3DX12_RESOURCE_BARRIER resBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_swapChainBuffer[ref_mCurrentBackBuffer].Get(),
		D3D12_RESOURCE_STATE_PRESENT,		// 状态：呈现
		D3D12_RESOURCE_STATE_RENDER_TARGET  // 状态：渲染目标
	);

	m_cmdList->ResourceBarrier(
		1,
		&resBarrier
	);

	//3、设置视口和裁剪矩形
	m_cmdList->RSSetViewports(1, &m_viewPort);
	m_cmdList->RSSetScissorRects(1, &m_scissorRect);

	//4、创建RTV描述符堆
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
		ref_mCurrentBackBuffer,
		m_rtvDescriptorSize);
	// 清空渲染目标
	m_cmdList->ClearRenderTargetView(rtvHandle, DirectX::Colors::AliceBlue, 0, nullptr);
	// 深度模板描述符堆
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
	m_cmdList->ClearDepthStencilView(
		dsvHandle, // DSV句柄
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
		1.0f, // 
		0,    // 
		0,    // 
		nullptr // 
	);



	//5、将渲染目标和深度模板 设置到命令队列上
	m_cmdList->OMSetRenderTargets(
		1,    //待绑定的RTV数量
		&rtvHandle,//指向RTV数组的指针
		true, //RTV对象在堆内存中是连续存放的
		&dsvHandle//指向DSV的指针
	);


	///////////////////////////////////////////////////////////////
	// 渲染图形的代码
	///////////////////////////////////////////////////////////////

	// 创建C描述符堆
	ID3D12DescriptorHeap* descriHeaps[] = { m_cbvHeap.Get() };
	m_cmdList->SetDescriptorHeaps(_countof(descriHeaps), descriHeaps);
	// 获取根签名
	m_cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
	// 设置顶点缓冲&索引缓冲
	m_cmdList->IASetVertexBuffers(0, 1, &GetVpv());
	m_cmdList->IASetVertexBuffers(1, 1, &GetVcv());
	m_cmdList->IASetIndexBuffer(&GetIbv());
	// 设置图元
	m_cmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//m_cmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
	// 设置根描述符表
	m_cmdList->SetGraphicsRootDescriptorTable(
		0,
		m_cbvHeap->GetGPUDescriptorHandleForHeapStart()
	);

	Update();

	// DrawCall
	m_cmdList->DrawIndexedInstanced(
		36, // 每个实例要绘制的索引数
		1, // 实例化个数
		0, // 起始索引位置
		0, // 子物体起始索引在全局索引中的位置
		0  
	);





	//6、绘制完毕，切换后台缓冲区为呈现状态
	resBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_swapChainBuffer[ref_mCurrentBackBuffer].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT 
	);

	m_cmdList->ResourceBarrier(
		1,
		&resBarrier
	);

	// 7、关闭命令列表，并将命令列表中的命令传送到命令队列中
	ThrowIfFailed(m_cmdList->Close());

	ID3D12CommandList* commandLists[] = { m_cmdList.Get() }; 
	m_cmdQueue->ExecuteCommandLists(_countof(commandLists), commandLists); 

	//ThrowIfFailed(m_swapChain->Present(0, 0)); // 垂直同步模式，只有60帧
	ThrowIfFailed(m_swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING));
	ref_mCurrentBackBuffer = (ref_mCurrentBackBuffer + 1) % 2;

	//10、围栏同步CPU和GPU
	FlushCmdQueue();

	return true;
}



// 创建常量描述符堆 & 常量缓冲区资源
bool D3D12InitApp::CreateCBV() {
	UINT objConstSize = CalcConstantBufferByteSize(sizeof(ObjectConstants));
	
	D3D12_DESCRIPTOR_HEAP_DESC cbHeapDesc;
	cbHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbHeapDesc.NumDescriptors = 1;
	cbHeapDesc.NodeMask = 0;
	ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&cbHeapDesc, IID_PPV_ARGS(&m_cbvHeap)));

	// 创建常量缓冲区资源
	m_objCB = std::make_unique<UploadBufferResource<ObjectConstants>>(m_d3dDevice.Get(), 1, true);
	// 获得常量缓冲区首地址
	D3D12_GPU_VIRTUAL_ADDRESS address;
	address = m_objCB->Resource()->GetGPUVirtualAddress();
	// 通过常量缓冲区元素偏移值计算最终的元素地址
	int cbElementIndex = 0;	// 常量缓冲区元素下标
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
	//根参数可以是描述符表、根描述符、根常量
	CD3DX12_ROOT_PARAMETER slotRootParameter[1];
	//创建由单个CBV所组成的描述符表
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV, //描述符类型
		1,//描述符数量
		0 //描述符所绑定的寄存器槽号
	);

	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);

	//根签名由一组根参数构成
	CD3DX12_ROOT_SIGNATURE_DESC rootSig(
		1, //根参数的数量
		slotRootParameter, //根参数指针
		0,
		nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	//用单个寄存器槽来创建一个根签名，该槽位指向一个仅含有单个常量缓冲区的描述符区域
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(
		&rootSig, 
		D3D_ROOT_SIGNATURE_VERSION_1, 
		serializedRootSig.GetAddressOf(),
		errorBlob.GetAddressOf()
	);


	if (errorBlob != nullptr)
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());

	ThrowIfFailed(hr);

	ThrowIfFailed(
		m_d3dDevice->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(&m_rootSignature)
		)
	);
}

// 构建几何体
void D3D12InitApp::BuildGeometry() {
	std::array<VPosData, 8> vertices =
	{
		VPosData({ XMFLOAT3(-1.0f, -1.0f, -1.0f)}), 
		VPosData({ XMFLOAT3(-1.0f, +1.0f, -1.0f)}), 
		VPosData({ XMFLOAT3(+1.0f, +1.0f, -1.0f)}), 
		VPosData({ XMFLOAT3(+1.0f, -1.0f, -1.0f)}), 
		VPosData({ XMFLOAT3(-1.0f, -1.0f, +1.0f)}), 
		VPosData({ XMFLOAT3(-1.0f, +1.0f, +1.0f)}),
		VPosData({ XMFLOAT3(+1.0f, +1.0f, +1.0f)}), 
		VPosData({ XMFLOAT3(+1.0f, -1.0f, +1.0f)}) 
	};

	std::array<VColorData, 8> colors = {
		VColorData({ XMFLOAT4(Colors::White)   }),
		VColorData({ XMFLOAT4(Colors::Black)   }),
		VColorData({ XMFLOAT4(Colors::Red)	   }),
		VColorData({ XMFLOAT4(Colors::Green)   }),
		VColorData({ XMFLOAT4(Colors::Blue)	   }),
		VColorData({ XMFLOAT4(Colors::Yellow)  }),
		VColorData({ XMFLOAT4(Colors::Cyan)	   }),
		VColorData({ XMFLOAT4(Colors::Magenta) })
	};

	std::array<std::uint16_t, 36> indices =
	{
		// front face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// top face
		1, 5, 6,
		1, 6, 2,

		// bottom face
		4, 0, 3,
		4, 3, 7
	};



	m_vpByteSize = (UINT)vertices.size() * sizeof(VPosData);
	m_vcByteSize = (UINT)colors.size() * sizeof(VColorData);
	m_ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);
	// 创建CPU系统空间
	ThrowIfFailed(D3DCreateBlob(m_vpByteSize, m_vPosBufferCPU.GetAddressOf()));
	ThrowIfFailed(D3DCreateBlob(m_vcByteSize, m_vColorBufferCPU.GetAddressOf()));
	ThrowIfFailed(D3DCreateBlob(m_ibByteSize, m_indexBufferCPU.GetAddressOf()));
	// 拷贝顶点&索引数据到CPU系统空间
	CopyMemory(m_vPosBufferCPU->GetBufferPointer(), vertices.data(), m_vpByteSize);
	CopyMemory(m_vPosBufferCPU->GetBufferPointer(), colors.data(), m_vcByteSize);
	CopyMemory(m_indexBufferCPU->GetBufferPointer(), indices.data(), m_ibByteSize);
	// 在GPU上创建默认堆
	m_vPosBufferGPU = ToolFunc::CreateDefaultBuffer(m_d3dDevice.Get(), m_cmdList.Get(), m_vpByteSize, vertices.data(), m_vPosBufferUploader);
	m_vColorBufferGPU = ToolFunc::CreateDefaultBuffer(m_d3dDevice.Get(), m_cmdList.Get(), m_vcByteSize, colors.data(), m_vColorBufferUploader);
	m_indexBufferGPU = ToolFunc::CreateDefaultBuffer(m_d3dDevice.Get(), m_cmdList.Get(), m_ibByteSize, indices.data(), m_indexBufferUploader);
}

// 设置管线状态对象
void D3D12InitApp::BuildPSO() {
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { m_inputLayoutDesc.data(), (UINT)m_inputLayoutDesc.size() };
	psoDesc.pRootSignature = m_rootSignature.Get();
	psoDesc.VS = { reinterpret_cast<BYTE*>(m_vsBytecode->GetBufferPointer()), m_vsBytecode->GetBufferSize() };
	psoDesc.PS = { reinterpret_cast<BYTE*>(m_psBytecode->GetBufferPointer()), m_psBytecode->GetBufferSize() };
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	//psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;	
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;	
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;	
	psoDesc.SampleDesc.Quality = 0;	

	
	ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_PSO)));
}

void D3D12InitApp::BuildByteCodeAndInputLayout() {
	// 编译shader
	HRESULT hr = S_OK;
	m_vsBytecode = ToolFunc::CompileShader(L"Shaders\\color.hlsl", nullptr, "VS", "vs_5_0");
	m_psBytecode = ToolFunc::CompileShader(L"Shaders\\color.hlsl", nullptr, "PS", "ps_5_0");

	m_inputLayoutDesc =
	{
		  { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		  { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

D3D12_VERTEX_BUFFER_VIEW D3D12InitApp::GetVpv() const {
	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = m_vPosBufferGPU->GetGPUVirtualAddress();
	vbv.StrideInBytes = sizeof(VPosData);
	vbv.SizeInBytes = m_vpByteSize;

	return vbv;
}

D3D12_VERTEX_BUFFER_VIEW D3D12InitApp::GetVcv() const {
	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = m_vColorBufferGPU->GetGPUVirtualAddress();
	vbv.StrideInBytes = sizeof(VColorData);
	vbv.SizeInBytes = m_vcByteSize;

	return vbv;
}

D3D12_INDEX_BUFFER_VIEW D3D12InitApp::GetIbv() const {
	D3D12_INDEX_BUFFER_VIEW ibv;
	ibv.BufferLocation = m_indexBufferGPU->GetGPUVirtualAddress();
	ibv.Format = DXGI_FORMAT_R16_UINT;
	ibv.SizeInBytes = m_ibByteSize;

	return ibv;
}
