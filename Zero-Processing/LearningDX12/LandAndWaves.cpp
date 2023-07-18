#include "LandAndWaves.h"

void LandAndWaves::BuildGeometry()
{
	GeometryGenerator geometryGenerator;
	GeometryGenerator::MeshData grid = geometryGenerator.CreateGrid(160.0f, 160.0f, 50, 50);

	// 封装grid的顶点、索引数据
	SubmeshGeometry gridSubmesh;
	gridSubmesh.indexCount = (UINT)grid.Indices32.size();
	gridSubmesh.baseVertexLocation = 0;
	gridSubmesh.startIndexLocation = 0;

	// 创建一个超级大的顶点缓存，并将所有物体的顶点数据存进去
	size_t totalVertexCount = grid.Vertices.size();
	std::vector<Vertex> vertices(totalVertexCount);	//给定顶点数组大小
	int k = 0;
	for (int i = 0; i < grid.Vertices.size(); i++)
	{
		vertices[i].Pos = grid.Vertices[i].Position;
		vertices[i].Pos.y = GetHillsHeight(vertices[i].Pos.x, vertices[i].Pos.z);

		//根据顶点不同的y值，给予不同的顶点色(不同海拔对应的颜色)
		if (vertices[i].Pos.y < -10.0f)
		{
			vertices[i].Color = XMFLOAT4(1.0f, 0.96f, 0.62f, 1.0f);
		}
		else if (vertices[i].Pos.y < 5.0f)
		{
			vertices[i].Color = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
		}
		else if (vertices[i].Pos.y < 12.0f)
		{
			vertices[i].Color = XMFLOAT4(0.1f, 0.48f, 0.19f, 1.0f);
		}
		else if (vertices[i].Pos.y < 20.0f)
		{
			vertices[i].Color = XMFLOAT4(0.45f, 0.39f, 0.34f, 1.0f);
		}
		else
		{
			vertices[i].Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		}
	}

	// 这里引入新的MeshGeometry来存放顶点&索引数据，并为后续动态顶点变化引入结构支持
	auto geo = std::make_unique<MeshGeometry>();
	geo->name = "landGeo";

	//创建索引缓存
	std::vector<std::uint16_t> indices = grid.GetIndices16();

	// 计算顶点缓存 & 索引缓存大小
	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);
	geo->m_vbByteSize = vbByteSize;
	geo->m_ibByteSize = ibByteSize;
	geo->m_vByteStride = sizeof(Vertex);
	geo->m_indexFormat = DXGI_FORMAT_R16_UINT;

	// 将顶点缓存&索引缓存的位置定位到GPU上
	ThrowIfFailed(D3DCreateBlob(vbByteSize, geo->m_vertexBufferCPU.GetAddressOf()));
	ThrowIfFailed(D3DCreateBlob(ibByteSize, geo->m_indexBufferCPU.GetAddressOf()));
	CopyMemory(geo->m_vertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
	CopyMemory(geo->m_indexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);
	geo->m_vertexBufferGPU = ToolFunc::CreateDefaultBuffer(m_d3dDevice.Get(), m_cmdList.Get(), vbByteSize, vertices.data(), geo->m_vertexBufferUploader);
	geo->m_indexBufferGPU = ToolFunc::CreateDefaultBuffer(m_d3dDevice.Get(), m_cmdList.Get(), ibByteSize, indices.data(), geo->m_indexBufferUploader);

	// 存放在BuildGeometry阶段确定的各个物体的DrawCall参数，用于后续DrawCall使用
	geo->m_mapDrawArgs.insert(std::make_pair("landGrid", gridSubmesh));
	// 将整个MeshGeometry存入一个总的map中供App维护，下属的顶点属性、顶点资源什么的由物体的RenderItem自己维护
	m_geometries["landGeo"] = std::move(geo);

	BuildLakeIndexBuffer();
}

void LandAndWaves::BuildRenderItem()
{
	// “山川”的渲染项
	auto landRitem = std::make_unique<RenderItem>();
	landRitem->world = MathHelper::Identity4x4();
	landRitem->objCBIndex = 0;
	landRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	landRitem->geo = m_geometries["landGeo"].get();
	landRitem->indexCount = landRitem->geo->m_mapDrawArgs["landGrid"].indexCount;
	landRitem->baseVertexLocation = landRitem->geo->m_mapDrawArgs["landGrid"].baseVertexLocation;
	landRitem->startIndexLocation = landRitem->geo->m_mapDrawArgs["landGrid"].startIndexLocation;
	m_allRItems.push_back(std::move(landRitem));

	// “湖泊”的渲染项
	auto lakeRitem = std::make_unique<RenderItem>();
	lakeRitem->world = MathHelper::Identity4x4();
	lakeRitem->objCBIndex = 1;
	lakeRitem->geo = m_geometries["lakeGeo"].get();
	lakeRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	lakeRitem->indexCount = lakeRitem->geo->m_mapDrawArgs["lakeGrid"].indexCount;
	lakeRitem->baseVertexLocation = lakeRitem->geo->m_mapDrawArgs["lakeGrid"].baseVertexLocation;
	lakeRitem->startIndexLocation = lakeRitem->geo->m_mapDrawArgs["lakeGrid"].startIndexLocation;

	m_waveRitem = lakeRitem.get();

	m_allRItems.push_back(std::move(lakeRitem)); // push_back后，智能指针自动释放内存
}

void LandAndWaves::BuildRootSignature()
{
	// 使用根描述符创建根参数
	CD3DX12_ROOT_PARAMETER slotRootParameter[2];
	slotRootParameter[0].InitAsConstantBufferView(0);
	slotRootParameter[1].InitAsConstantBufferView(1);

	/// 用根参数创建根签名
	// 根签名由一组根参数构成
	CD3DX12_ROOT_SIGNATURE_DESC rootSig(
		2, //根参数的数量
		slotRootParameter, //根参数指针
		0,
		nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	// 序列化
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

bool LandAndWaves::Init(HINSTANCE hInstance, int nShowCmd)
{
#if defined(DEBUG) | defined(_DEBUG)
	ComPtr<ID3D12Debug> debugController;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
	debugController->EnableDebugLayer();
#endif

	if (!D3D12App::Init(hInstance, nShowCmd))
		return false;

	ThrowIfFailed(m_cmdList->Reset(m_cmdAllocator.Get(), nullptr));

	// 初始化波浪类
	m_waves = std::make_unique<Waves>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);

	BuildRootSignature();
	BuildByteCodeAndInputLayout();
	BuildGeometry();
	BuildRenderItem();
	BuildFrameResources();
	// 因为使用根描述符创建根参数，所以不需要原来的CreateCBV()了
	BuildPSO();

	// 记得关闭命令队列！
	ThrowIfFailed(m_cmdList->Close());
	ID3D12CommandList* cmdLists[] = { m_cmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	FlushCmdQueue();

	return true;
}

void LandAndWaves::DrawRenderItems()
{
	UINT objectCount = (UINT)m_allRItems.size();//物体总个数（包括实例）
	UINT objConstSize = CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT passConstSize = CalcConstantBufferByteSize(sizeof(PassConstants));
	m_objCBByteSize = objConstSize;
	m_passCBByteSize = passConstSize;

	std::vector<RenderItem*> ritems;
	for (auto& e : m_allRItems)
		ritems.push_back(e.get());

	for (size_t i = 0; i < ritems.size(); ++i)
	{
		auto ritem = ritems[i];

		m_cmdList->IASetVertexBuffers(0, 1, &ritem->geo->GetVbv());
		m_cmdList->IASetIndexBuffer(&ritem->geo->GetIbv());
		m_cmdList->IASetPrimitiveTopology(ritem->primitiveType);

		// 设置根描述符，将根描述符与资源绑定
		auto objCB = m_currFrameResource->m_objCB->Resource();
		auto objCBAddress = objCB->GetGPUVirtualAddress();
		objCBAddress += ritem->objCBIndex * m_objCBByteSize;
		m_cmdList->SetGraphicsRootConstantBufferView(
			0, // 根参数的起始索引
			objCBAddress
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

bool LandAndWaves::Draw()
{
	//1、重置命令列表 & 命令分配器
//auto currCmdAllocator = m_currFrameResource->m_cmdAllocator;	// 获取当前帧资源的命令分配器
	ThrowIfFailed(m_currFrameResource->m_cmdAllocator->Reset());
	ThrowIfFailed(m_cmdList->Reset(m_currFrameResource->m_cmdAllocator.Get(), m_PSO.Get())); // 记得传入PSO

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
	// 绑定根签名
	m_cmdList->SetGraphicsRootSignature(m_rootSignature.Get());

	// 设置全局的常量缓冲区（装viewproj矩阵）
	auto passCB = m_currFrameResource->m_passCB->Resource();
	m_cmdList->SetGraphicsRootConstantBufferView(
		1, 
		passCB->GetGPUVirtualAddress()
	);

	UpdateWaves(m_gt);

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

	// 更新常量缓冲区
	Update();

	// 7、关闭命令列表，并将命令列表中的命令传送到命令队列中
	ThrowIfFailed(m_cmdList->Close());

	ID3D12CommandList* commandLists[] = { m_cmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	//ThrowIfFailed(m_swapChain->Present(0, 0)); // 垂直同步模式，只有60帧
	ThrowIfFailed(m_swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING));
	ref_mCurrentBackBuffer = (ref_mCurrentBackBuffer + 1) % 2;


	//10、围栏同步CPU和GPU
	//FlushCmdQueue();
	m_currFrameResource->m_fenceCPU = ++m_currentFence;
	// Signal：在命令队列执行完所有指令后，把GPU_Fence变成我们传入的m_currentFence
	m_cmdQueue->Signal(m_fence.Get(), m_currentFence);

	return true;
}

void LandAndWaves::BuildFrameResources() {
	for (int i = 0; i < m_frameResourceCount; ++i) {
		m_frameResourcesList.push_back(std::make_unique<FrameResources>(
				m_d3dDevice.Get(),
				1,	// passCount
				(UINT)m_allRItems.size(),	// objCount
				m_waves->VertexCount()
			)
		);
	}
}

void LandAndWaves::BuildLakeIndexBuffer()
{
	//初始化索引列表（每个三角形3个索引）
	std::vector<std::uint16_t> indices(3 * m_waves->TriangleCount());
	assert(m_waves->VertexCount() < 0x0000ffff);//顶点索引数大于65536则中止程序

	//填充索引列表
	int m = m_waves->RowCount();
	int n = m_waves->ColumnCount();
	int k = 0;
	for (int i = 0; i < m - 1; i++)
	{
		for (int j = 0; j < n - 1; j++)
		{
			indices[k] = i * n + j;
			indices[k + 1] = i * n + j + 1;
			indices[k + 2] = (i + 1) * n + j;

			indices[k + 3] = (i + 1) * n + j;
			indices[k + 4] = i * n + j + 1;
			indices[k + 5] = (i + 1) * n + j + 1;

			k += 6;
		}
	}
	//计算顶点和索引缓存大小
	UINT vbByteSize = m_waves->VertexCount() * sizeof(Vertex);
	UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->name = "lakeGeo";

	geo->m_vertexBufferCPU = nullptr;
	geo->m_vertexBufferGPU = nullptr;
	//创建索引的CPU系统内存
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->m_indexBufferCPU));
	//将索引列表存入CPU系统内存
	CopyMemory(geo->m_indexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);
	//将索引数据通过上传堆传至默认堆
	geo->m_indexBufferGPU = ToolFunc::CreateDefaultBuffer(m_d3dDevice.Get(),
		m_cmdList.Get(),
		ibByteSize,
		indices.data(),
		geo->m_indexBufferUploader);

	//赋值MeshGeomety中相关属性
	geo->m_vbByteSize = vbByteSize;
	geo->m_vByteStride = sizeof(Vertex);
	geo->m_indexFormat = DXGI_FORMAT_R16_UINT;
	geo->m_ibByteSize = ibByteSize;

	SubmeshGeometry LakeSubmesh;
	LakeSubmesh.baseVertexLocation = 0;
	LakeSubmesh.startIndexLocation = 0;
	LakeSubmesh.indexCount = (UINT)indices.size();
	//使用grid几何体
	geo->m_mapDrawArgs["lakeGrid"] = LakeSubmesh;
	//湖泊的MeshGeometry
	m_geometries["lakeGeo"] = std::move(geo);
}

void LandAndWaves::UpdateWaves(const GameTime& gt)
{
	static float t_base = 0.0f;
	if ((m_gt.TotalTime() - t_base) >= 0.25f)
	{
		t_base += 0.25f;	//0.25秒生成一个波浪
		//随机生成横坐标
		int i = MathHelper::Rand(4, m_waves->RowCount() - 5);
		//随机生成纵坐标
		int j = MathHelper::Rand(4, m_waves->ColumnCount() - 5);
		//随机生成波的半径
		float r = MathHelper::RandF(0.2f, 0.5f);//float用RandF函数
		//使用波动方程函数生成波纹
		m_waves->Disturb(i, j, r);
	}

	//每帧更新波浪模拟（即更新顶点坐标）
	m_waves->Update(gt.DeltaTime());

	//将更新的顶点坐标存入GPU上传堆中
	auto currWavesVB = m_currFrameResource->m_waveVB.get();
	for (int i = 0; i < m_waves->VertexCount(); i++)
	{
		Vertex v;
		v.Pos = m_waves->Position(i);
		v.Color = XMFLOAT4(DirectX::Colors::Blue);

		currWavesVB->CopyData(i, v);
	}
	//赋值湖泊的GPU上的顶点缓存
	m_waveRitem->geo->m_vertexBufferGPU = currWavesVB->Resource();
}