#include "D3D12InitApp.h"

struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};

// 常量缓冲区中要存放的数据
struct ObjectConstants
{
	XMFLOAT4X4 world = MathHelper::Identity4x4();
};
struct PassConstants
{
	XMFLOAT4X4 viewProj = MathHelper::Identity4x4();
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

	
	BuildRootSignature();
	BuildByteCodeAndInputLayout();
	BuildGeometry();
	BuildRenderItem();
	CreateCBV();
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
	PassConstants passConstants;

	float x = m_radius * sinf(m_phi) * cosf(m_theta);
	float y = m_radius * cosf(m_phi);
	float z = m_radius * sinf(m_phi) * sinf(m_theta);
	//float r = 5.0f;
	//x *= sinf(m_gt.TotalTime());
	//z = sqrt(r * r - x * x);
	// 相机矩阵
	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX v = XMMatrixLookAtLH(pos, target, up);

	// 每个物体都要有一个常量缓冲区（主要装world矩阵）
	for (auto& e : m_allRItem)
	{
		m_world = e->world;
		// 构建世界矩阵
		XMMATRIX w = XMLoadFloat4x4(&m_world);
		//w *= XMMatrixTranslation(2.0f, 0.0f, 0.0f);
		//将XMMATRIX复制给XMFLOAT4X4
		XMStoreFloat4x4(&objConstants.world, XMMatrixTranspose(w));
		// 将数据拷贝至GPU缓存
		m_objCB->CopyData(e->objCBIndex, objConstants);
	}

	// 全局需要一个常量缓冲区（装viewproj矩阵）
	// 拿到投影矩阵
	XMMATRIX p = XMLoadFloat4x4(&m_proj);
	p = XMMatrixPerspectiveFovLH(0.25f * 3.1416f, m_clientWidth / m_clientHeight, 1.0f, 1000.0f);
	// 矩阵计算
	XMMATRIX VP_Matrix = v * p;
	XMStoreFloat4x4(&passConstants.viewProj, XMMatrixTranspose(VP_Matrix));
	m_passCB->CopyData(0, passConstants);
}

// 鼠标相关
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

	//XMMATRIX p = XMMatrixPerspectiveFovLH(0.25f * 3.1416f, m_clientWidth / m_clientHeight, 1.0f, 1000.0f);
	//XMStoreFloat4x4(&m_proj, p);
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
	// 绑定根签名
	m_cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
	// 更新常量缓冲区
	Update();

	//// 设置顶点缓冲&索引缓冲
	//m_cmdList->IASetVertexBuffers(0, 1, &GetVbv());
	//m_cmdList->IASetIndexBuffer(&GetIbv());
	//// 设置图元
	//m_cmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//// 绑定根描述符表
	//// 因为我们现在有两个常量缓冲区，并创建了两个根描述符表，所以我们应当绑定两次
	//int objCbvIndex = 0;
	//auto handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetGPUDescriptorHandleForHeapStart());
	//handle.Offset(objCbvIndex, m_cbv_srv_uavDescriptorSize);
	//m_cmdList->SetGraphicsRootDescriptorTable(
	//	0,
	//	handle
	//);
	//// DrawCall
	//m_cmdList->DrawIndexedInstanced(
	//	m_mapDrawArgs["box"].indexCount,// 每个实例要绘制的索引数
	//	1, // 实例化个数
	//	m_mapDrawArgs["box"].startIndexLocation, // 起始索引位置
	//	m_mapDrawArgs["box"].baseVertexLocation, // 子物体起始索引在全局索引中的位置
	//	0
	//);
	//m_cmdList->DrawIndexedInstanced(
	//	m_mapDrawArgs["grid"].indexCount,// 每个实例要绘制的索引数
	//	1, // 实例化个数
	//	m_mapDrawArgs["grid"].startIndexLocation, // 起始索引位置
	//	m_mapDrawArgs["grid"].baseVertexLocation, // 子物体起始索引在全局索引中的位置
	//	0
	//);
	//m_cmdList->DrawIndexedInstanced(
	//	m_mapDrawArgs["sphere"].indexCount,// 每个实例要绘制的索引数
	//	1, // 实例化个数
	//	m_mapDrawArgs["sphere"].startIndexLocation, // 起始索引位置
	//	m_mapDrawArgs["sphere"].baseVertexLocation, // 子物体起始索引在全局索引中的位置
	//	0
	//);
	//m_cmdList->DrawIndexedInstanced(
	//	m_mapDrawArgs["cylinder"].indexCount,// 每个实例要绘制的索引数
	//	1, // 实例化个数
	//	m_mapDrawArgs["cylinder"].startIndexLocation, // 起始索引位置
	//	m_mapDrawArgs["cylinder"].baseVertexLocation, // 子物体起始索引在全局索引中的位置
	//	0
	//);

	//int passCbvIndex = 1;
	//handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetGPUDescriptorHandleForHeapStart());
	//handle.Offset(passCbvIndex, m_cbv_srv_uavDescriptorSize);
	//m_cmdList->SetGraphicsRootDescriptorTable(
	//	1,
	//	handle
	//);

	// 设置全局的常量缓冲区（装viewproj矩阵）
	int passCbvIndex = (UINT)m_allRItem.size();
	auto passCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetGPUDescriptorHandleForHeapStart());
	passCbvHandle.Offset(passCbvIndex, m_cbv_srv_uavDescriptorSize);
	m_cmdList->SetGraphicsRootDescriptorTable(
		1, // 根参数的起始索引
		passCbvHandle
	);

	// 绘制渲染项
	DrawRenderItems();




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
	UINT objectCount = (UINT)m_allRItem.size();  // 物体总个数（包含实例）
	UINT objConstSize = CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT passConstSize = CalcConstantBufferByteSize(sizeof(PassConstants));
	
	D3D12_DESCRIPTOR_HEAP_DESC cbHeapDesc;
	cbHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbHeapDesc.NumDescriptors = objectCount + 1;  // 每个物体都要一个world矩阵，然后全局还需要一个viewproj矩阵，所以要+1
	cbHeapDesc.NodeMask = 0;
	ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&cbHeapDesc, IID_PPV_ARGS(&m_cbvHeap)));

	// 常量缓冲区1
	// 创建常量缓冲区资源（主要是world矩阵）
	m_objCB = std::make_unique<UploadBufferResource<ObjectConstants>>(m_d3dDevice.Get(), objectCount, true);
	// 为每个物体都创建一个cbv
	for (int i = 0; i < objectCount; ++i)
	{
		// 获得常量缓冲区首地址
		D3D12_GPU_VIRTUAL_ADDRESS address0;
		address0 = m_objCB->Resource()->GetGPUVirtualAddress();
		// 通过常量缓冲区元素偏移值计算最终的元素地址
		int cbElementIndex = i;	// 常量缓冲区元素下标
		address0 += cbElementIndex * objConstSize;
		// 设置CBV堆的偏移
		int heapIndex = i;
		auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetCPUDescriptorHandleForHeapStart());// 获得CBV堆首地址
		handle.Offset(heapIndex, m_cbv_srv_uavDescriptorSize);
		// 创建CBV描述符
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc0;
		cbvDesc0.BufferLocation = address0;
		cbvDesc0.SizeInBytes = objConstSize;
		m_d3dDevice->CreateConstantBufferView(&cbvDesc0, handle);
	}

	// 常量缓冲区2（主要是viewproj矩阵）
	m_passCB = std::make_unique<UploadBufferResource<PassConstants>>(m_d3dDevice.Get(), 1, true);
	D3D12_GPU_VIRTUAL_ADDRESS addressPass;
	addressPass = m_passCB->Resource()->GetGPUVirtualAddress();
	int passCbElementIndex = 0;
	addressPass += passCbElementIndex * passConstSize;
	int heapIndex = objectCount;
	auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetCPUDescriptorHandleForHeapStart());// 获得CBV堆首地址
	handle.Offset(heapIndex, m_cbv_srv_uavDescriptorSize);
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc1;
	cbvDesc1.BufferLocation = addressPass;
	cbvDesc1.SizeInBytes = passConstSize;
	m_d3dDevice->CreateConstantBufferView(&cbvDesc1, handle);

	return true;
}

// 创建根签名
void D3D12InitApp::BuildRootSignature() {
	//根参数
	CD3DX12_ROOT_PARAMETER slotRootParameter[2];
	//创建由单个CBV所组成的描述符表
	CD3DX12_DESCRIPTOR_RANGE cbvTable0;
	cbvTable0.Init(
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV, //描述符类型
		1,//描述符数量
		0 //描述符所绑定的寄存器槽号
	);
	CD3DX12_DESCRIPTOR_RANGE cbvTable1;
	cbvTable1.Init(
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
		1,
		1
	);
	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable0);
	slotRootParameter[1].InitAsDescriptorTable(1, &cbvTable1);


	// 根签名由一组根参数构成
	CD3DX12_ROOT_SIGNATURE_DESC rootSig(
		2, //根参数的数量
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
	GeometryGenerator geometryGenerator;
	GeometryGenerator::MeshData box = geometryGenerator.CreateBox(1.5f, 0.5f, 1.5f, 3);
	GeometryGenerator::MeshData grid = geometryGenerator.CreateGrid(20.0f, 30.0f, 60, 40);
	GeometryGenerator::MeshData sphere = geometryGenerator.CreateGeosphere(1, 2);
	GeometryGenerator::MeshData cylinder = geometryGenerator.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);

	// 计算单个几何体顶点在总顶点数组中的偏移量
	UINT boxVertexOffset = 0;
	UINT gridVertexOffset = (UINT)box.Vertices.size();
	UINT sphereVertexOffset = (UINT)grid.Vertices.size() + gridVertexOffset;
	UINT cylinderVertexOffset = (UINT)sphere.Vertices.size() + sphereVertexOffset;

	//计算单个几何体索引在总索引数组中的偏移量,顺序为：box、grid、sphere、cylinder
	UINT boxIndexOffset = 0;
	UINT gridIndexOffset = (UINT)box.Indices32.size();
	UINT sphereIndexOffset = (UINT)grid.Indices32.size() + gridIndexOffset;
	UINT cylinderIndexOffset = (UINT)sphere.Indices32.size() + sphereIndexOffset;

	// DrawCall绘制一个物体需要有三个属性：索引数量、顶点起始索引位置、子物体起始索引在全局索引中的位置

	SubmeshGeometry boxSubmesh;
	boxSubmesh.indexCount = (UINT)box.Indices32.size();
	boxSubmesh.baseVertexLocation = boxVertexOffset;
	boxSubmesh.startIndexLocation = boxIndexOffset;

	SubmeshGeometry gridSubmesh;
	gridSubmesh.indexCount = (UINT)grid.Indices32.size();
	gridSubmesh.baseVertexLocation = gridVertexOffset;
	gridSubmesh.startIndexLocation = gridIndexOffset;

	SubmeshGeometry sphereSubmesh;
	sphereSubmesh.indexCount = (UINT)sphere.Indices32.size();
	sphereSubmesh.baseVertexLocation = sphereVertexOffset;
	sphereSubmesh.startIndexLocation = sphereIndexOffset;

	SubmeshGeometry cylinderSubmesh;
	cylinderSubmesh.indexCount = (UINT)cylinder.Indices32.size();
	cylinderSubmesh.baseVertexLocation = cylinderVertexOffset;
	cylinderSubmesh.startIndexLocation = cylinderIndexOffset;

	// 创建一个超级大的顶点缓存，并将所有物体的顶点数据存进去
	size_t totalVertexCount = box.Vertices.size() + grid.Vertices.size() + sphere.Vertices.size() + cylinder.Vertices.size();
	std::vector<Vertex> vertices(totalVertexCount);	//给定顶点数组大小
	int k = 0;
	for (int i = 0; i < box.Vertices.size(); i++, k++)
	{
		vertices[k].Pos = box.Vertices[i].Position;
		vertices[k].Color = XMFLOAT4(DirectX::Colors::Yellow);
	}
	for (int i = 0; i < grid.Vertices.size(); i++, k++)
	{
		vertices[k].Pos = grid.Vertices[i].Position;
		vertices[k].Color = XMFLOAT4(DirectX::Colors::Brown);
	}
	for (int i = 0; i < sphere.Vertices.size(); i++, k++)
	{
		vertices[k].Pos = sphere.Vertices[i].Position;
		vertices[k].Color = XMFLOAT4(DirectX::Colors::Green);
	}
	for (int i = 0; i < cylinder.Vertices.size(); i++, k++)
	{
		vertices[k].Pos = cylinder.Vertices[i].Position;
		vertices[k].Color = XMFLOAT4(DirectX::Colors::Blue);
	}

	// 同理，再创建一个超级大的索引缓存
	std::vector<std::uint16_t> indices;
	indices.insert(indices.end(), box.GetIndices16().begin(), box.GetIndices16().end());
	indices.insert(indices.end(), grid.GetIndices16().begin(), grid.GetIndices16().end());
	indices.insert(indices.end(), sphere.GetIndices16().begin(), sphere.GetIndices16().end());
	indices.insert(indices.end(), cylinder.GetIndices16().begin(), cylinder.GetIndices16().end());


	m_vbByteSize = (UINT)vertices.size() * sizeof(GeometryGenerator::Vertex);
	m_ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);
	// 创建CPU系统空间
	ThrowIfFailed(D3DCreateBlob(m_vbByteSize, m_vertexBufferCPU.GetAddressOf()));
	ThrowIfFailed(D3DCreateBlob(m_ibByteSize, m_indexBufferCPU.GetAddressOf()));
	// 拷贝顶点&索引数据到CPU系统空间
	CopyMemory(m_vertexBufferCPU->GetBufferPointer(), vertices.data(), m_vbByteSize);
	CopyMemory(m_indexBufferCPU->GetBufferPointer(), indices.data(), m_ibByteSize);
	// 在GPU上创建默认堆
	m_vertexBufferGPU = ToolFunc::CreateDefaultBuffer(m_d3dDevice.Get(), m_cmdList.Get(), m_vbByteSize, vertices.data(), m_vertexBufferUploader);
	m_indexBufferGPU = ToolFunc::CreateDefaultBuffer(m_d3dDevice.Get(), m_cmdList.Get(), m_ibByteSize, indices.data(), m_indexBufferUploader);

	//  存放在BuildGeometry阶段确定的各个物体的DrawCall参数，用于后续DrawCall使用
	//m_mapDrawArgs["box"] = boxSubmesh;
	//m_mapDrawArgs["grid"] = gridSubmesh;
	//m_mapDrawArgs["sphere"] = sphereSubmesh;
	//m_mapDrawArgs["cylinder"] = cylinderSubmesh;

	m_mapDrawArgs.insert(std::make_pair("box", boxSubmesh));
	m_mapDrawArgs.insert(std::make_pair("grid", gridSubmesh));
	m_mapDrawArgs.insert(std::make_pair("sphere", sphereSubmesh));
	m_mapDrawArgs.insert(std::make_pair("cylinder", cylinderSubmesh));

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
	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
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
		  { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

D3D12_VERTEX_BUFFER_VIEW D3D12InitApp::GetVbv() const {
	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = m_vertexBufferGPU->GetGPUVirtualAddress();
	vbv.StrideInBytes = sizeof(Vertex);
	vbv.SizeInBytes = m_vbByteSize;

	return vbv;
}

D3D12_INDEX_BUFFER_VIEW D3D12InitApp::GetIbv() const {
	D3D12_INDEX_BUFFER_VIEW ibv;
	ibv.BufferLocation = m_indexBufferGPU->GetGPUVirtualAddress();
	ibv.Format = DXGI_FORMAT_R16_UINT;
	ibv.SizeInBytes = m_ibByteSize;

	return ibv;
}

// 构建渲染项！（实例化物体！）
void D3D12InitApp::BuildRenderItem() {
	auto boxRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&(boxRitem->world), XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 0.5f, 0.0f));
	boxRitem->objCBIndex = 0;//BOX常量数据（world矩阵）在objConstantBuffer索引0上
	boxRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRitem->indexCount = m_mapDrawArgs["box"].indexCount;
	boxRitem->baseVertexLocation = m_mapDrawArgs["box"].baseVertexLocation;
	boxRitem->startIndexLocation = m_mapDrawArgs["box"].startIndexLocation;
	m_allRItem.push_back(std::move(boxRitem));

	auto gridRitem = std::make_unique<RenderItem>();
	gridRitem->world = MathHelper::Identity4x4();
	gridRitem->objCBIndex = 1;//BOX常量数据（world矩阵）在objConstantBuffer索引1上
	gridRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRitem->indexCount = m_mapDrawArgs["grid"].indexCount;
	gridRitem->baseVertexLocation = m_mapDrawArgs["grid"].baseVertexLocation;
	gridRitem->startIndexLocation = m_mapDrawArgs["grid"].startIndexLocation;
	m_allRItem.push_back(std::move(gridRitem));

	UINT fllowObjCBIndex = 2;//接下去的几何体常量数据在CB中的索引从2开始
	//将圆柱和圆的实例模型存入渲染项中
	for (int i = 0; i < 5; i++)
	{
		auto leftCylinderRitem = std::make_unique<RenderItem>();
		auto rightCylinderRitem = std::make_unique<RenderItem>();
		auto leftSphereRitem = std::make_unique<RenderItem>();
		auto rightSphereRitem = std::make_unique<RenderItem>();

		XMMATRIX leftCylWorld = XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f);
		XMMATRIX rightCylWorld = XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i * 5.0f);
		XMMATRIX leftSphereWorld = XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f);
		XMMATRIX rightSphereWorld = XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f);
		//左边5个圆柱
		XMStoreFloat4x4(&(leftCylinderRitem->world), leftCylWorld);
		//此处的索引随着循环不断加1（注意：这里是先赋值再++）
		leftCylinderRitem->objCBIndex = fllowObjCBIndex++;
		leftCylinderRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftCylinderRitem->indexCount = m_mapDrawArgs["cylinder"].indexCount;
		leftCylinderRitem->baseVertexLocation = m_mapDrawArgs["cylinder"].baseVertexLocation;
		leftCylinderRitem->startIndexLocation = m_mapDrawArgs["cylinder"].startIndexLocation;
		//右边5个圆柱
		XMStoreFloat4x4(&(rightCylinderRitem->world), rightCylWorld);
		rightCylinderRitem->objCBIndex = fllowObjCBIndex++;
		rightCylinderRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightCylinderRitem->indexCount = m_mapDrawArgs["cylinder"].indexCount;
		rightCylinderRitem->baseVertexLocation = m_mapDrawArgs["cylinder"].baseVertexLocation;
		rightCylinderRitem->startIndexLocation = m_mapDrawArgs["cylinder"].startIndexLocation;
		//左边5个球
		XMStoreFloat4x4(&(leftSphereRitem->world), leftSphereWorld);
		leftSphereRitem->objCBIndex = fllowObjCBIndex++;
		leftSphereRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftSphereRitem->indexCount = m_mapDrawArgs["sphere"].indexCount;
		leftSphereRitem->baseVertexLocation = m_mapDrawArgs["sphere"].baseVertexLocation;
		leftSphereRitem->startIndexLocation = m_mapDrawArgs["sphere"].startIndexLocation;
		//右边5个球
		XMStoreFloat4x4(&(rightSphereRitem->world), rightSphereWorld);
		rightSphereRitem->objCBIndex = fllowObjCBIndex++;
		rightSphereRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightSphereRitem->indexCount = m_mapDrawArgs["sphere"].indexCount;
		rightSphereRitem->baseVertexLocation = m_mapDrawArgs["sphere"].baseVertexLocation;
		rightSphereRitem->startIndexLocation = m_mapDrawArgs["sphere"].startIndexLocation;

		m_allRItem.push_back(std::move(leftCylinderRitem));
		m_allRItem.push_back(std::move(rightCylinderRitem));
		m_allRItem.push_back(std::move(leftSphereRitem));
		m_allRItem.push_back(std::move(rightSphereRitem));
	}
}

// 绘制多个物体
void D3D12InitApp::DrawRenderItems() 
{
	std::vector<RenderItem*> ritems;
	for (auto& e : m_allRItem)
		ritems.push_back(e.get());

	for (size_t i = 0; i < ritems.size(); ++i)
	{
		auto ritem = ritems[i];

		m_cmdList->IASetVertexBuffers(0, 1, &GetVbv());
		m_cmdList->IASetIndexBuffer(&GetIbv());
		m_cmdList->IASetPrimitiveTopology(ritem->primitiveType);

		// 设置根描述符表
		UINT objCbvIndex = ritem->objCBIndex;
		auto handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetGPUDescriptorHandleForHeapStart());
		handle.Offset(objCbvIndex, m_cbv_srv_uavDescriptorSize);
		m_cmdList->SetGraphicsRootDescriptorTable(
			0, // 根参数的起始索引
			handle
		);

		// 绘制顶点
		m_cmdList->DrawIndexedInstanced(
			ritem->indexCount, // 每个实例要绘制的索引数
			1, // 实例化个数
			ritem->startIndexLocation, // 起始索引位置
			ritem->baseVertexLocation, // 子物体起始索引在全局索引中的位置
			0 // 实例化高级技术，暂时设置为0
		);

	}
}