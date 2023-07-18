#include "LandAndWaves.h"

void LandAndWaves::BuildGeometry()
{
	GeometryGenerator geometryGenerator;
	GeometryGenerator::MeshData grid = geometryGenerator.CreateGrid(160.0f, 160.0f, 50, 50);

	// ��װgrid�Ķ��㡢��������
	SubmeshGeometry gridSubmesh;
	gridSubmesh.indexCount = (UINT)grid.Indices32.size();
	gridSubmesh.baseVertexLocation = 0;
	gridSubmesh.startIndexLocation = 0;

	// ����һ��������Ķ��㻺�棬������������Ķ������ݴ��ȥ
	size_t totalVertexCount = grid.Vertices.size();
	std::vector<Vertex> vertices(totalVertexCount);	//�������������С
	int k = 0;
	for (int i = 0; i < grid.Vertices.size(); i++)
	{
		vertices[i].Pos = grid.Vertices[i].Position;
		vertices[i].Pos.y = GetHillsHeight(vertices[i].Pos.x, vertices[i].Pos.z);

		//���ݶ��㲻ͬ��yֵ�����費ͬ�Ķ���ɫ(��ͬ���ζ�Ӧ����ɫ)
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

	// ���������µ�MeshGeometry����Ŷ���&�������ݣ���Ϊ������̬����仯����ṹ֧��
	auto geo = std::make_unique<MeshGeometry>();
	geo->name = "landGeo";

	//������������
	std::vector<std::uint16_t> indices = grid.GetIndices16();

	// ���㶥�㻺�� & ���������С
	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);
	geo->m_vbByteSize = vbByteSize;
	geo->m_ibByteSize = ibByteSize;
	geo->m_vByteStride = sizeof(Vertex);
	geo->m_indexFormat = DXGI_FORMAT_R16_UINT;

	// �����㻺��&���������λ�ö�λ��GPU��
	ThrowIfFailed(D3DCreateBlob(vbByteSize, geo->m_vertexBufferCPU.GetAddressOf()));
	ThrowIfFailed(D3DCreateBlob(ibByteSize, geo->m_indexBufferCPU.GetAddressOf()));
	CopyMemory(geo->m_vertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
	CopyMemory(geo->m_indexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);
	geo->m_vertexBufferGPU = ToolFunc::CreateDefaultBuffer(m_d3dDevice.Get(), m_cmdList.Get(), vbByteSize, vertices.data(), geo->m_vertexBufferUploader);
	geo->m_indexBufferGPU = ToolFunc::CreateDefaultBuffer(m_d3dDevice.Get(), m_cmdList.Get(), ibByteSize, indices.data(), geo->m_indexBufferUploader);

	// �����BuildGeometry�׶�ȷ���ĸ��������DrawCall���������ں���DrawCallʹ��
	geo->m_mapDrawArgs.insert(std::make_pair("landGrid", gridSubmesh));
	// ������MeshGeometry����һ���ܵ�map�й�Appά���������Ķ������ԡ�������Դʲô���������RenderItem�Լ�ά��
	m_geometries["landGeo"] = std::move(geo);

	BuildLakeIndexBuffer();
}

void LandAndWaves::BuildRenderItem()
{
	// ��ɽ��������Ⱦ��
	auto landRitem = std::make_unique<RenderItem>();
	landRitem->world = MathHelper::Identity4x4();
	landRitem->objCBIndex = 0;
	landRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	landRitem->geo = m_geometries["landGeo"].get();
	landRitem->indexCount = landRitem->geo->m_mapDrawArgs["landGrid"].indexCount;
	landRitem->baseVertexLocation = landRitem->geo->m_mapDrawArgs["landGrid"].baseVertexLocation;
	landRitem->startIndexLocation = landRitem->geo->m_mapDrawArgs["landGrid"].startIndexLocation;
	m_allRItems.push_back(std::move(landRitem));

	// ������������Ⱦ��
	auto lakeRitem = std::make_unique<RenderItem>();
	lakeRitem->world = MathHelper::Identity4x4();
	lakeRitem->objCBIndex = 1;
	lakeRitem->geo = m_geometries["lakeGeo"].get();
	lakeRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	lakeRitem->indexCount = lakeRitem->geo->m_mapDrawArgs["lakeGrid"].indexCount;
	lakeRitem->baseVertexLocation = lakeRitem->geo->m_mapDrawArgs["lakeGrid"].baseVertexLocation;
	lakeRitem->startIndexLocation = lakeRitem->geo->m_mapDrawArgs["lakeGrid"].startIndexLocation;

	m_waveRitem = lakeRitem.get();

	m_allRItems.push_back(std::move(lakeRitem)); // push_back������ָ���Զ��ͷ��ڴ�
}

void LandAndWaves::BuildRootSignature()
{
	// ʹ�ø�����������������
	CD3DX12_ROOT_PARAMETER slotRootParameter[2];
	slotRootParameter[0].InitAsConstantBufferView(0);
	slotRootParameter[1].InitAsConstantBufferView(1);

	/// �ø�����������ǩ��
	// ��ǩ����һ�����������
	CD3DX12_ROOT_SIGNATURE_DESC rootSig(
		2, //������������
		slotRootParameter, //������ָ��
		0,
		nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	// ���л�
	//�õ����Ĵ�����������һ����ǩ�����ò�λָ��һ�������е�������������������������
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

	// ��ʼ��������
	m_waves = std::make_unique<Waves>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);

	BuildRootSignature();
	BuildByteCodeAndInputLayout();
	BuildGeometry();
	BuildRenderItem();
	BuildFrameResources();
	// ��Ϊʹ�ø����������������������Բ���Ҫԭ����CreateCBV()��
	BuildPSO();

	// �ǵùر�������У�
	ThrowIfFailed(m_cmdList->Close());
	ID3D12CommandList* cmdLists[] = { m_cmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	FlushCmdQueue();

	return true;
}

void LandAndWaves::DrawRenderItems()
{
	UINT objectCount = (UINT)m_allRItems.size();//�����ܸ���������ʵ����
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

		// ���ø�����������������������Դ��
		auto objCB = m_currFrameResource->m_objCB->Resource();
		auto objCBAddress = objCB->GetGPUVirtualAddress();
		objCBAddress += ritem->objCBIndex * m_objCBByteSize;
		m_cmdList->SetGraphicsRootConstantBufferView(
			0, // ����������ʼ����
			objCBAddress
		);

		// ���ƶ���
		m_cmdList->DrawIndexedInstanced(
			ritem->indexCount, // ÿ��ʵ��Ҫ���Ƶ�������
			1, // ʵ��������
			ritem->startIndexLocation, // ��ʼ����λ��
			ritem->baseVertexLocation, // ��������ʼ������ȫ�������е�λ��
			0 // ʵ�����߼���������ʱ����Ϊ0
		);

	}
}

bool LandAndWaves::Draw()
{
	//1�����������б� & ���������
//auto currCmdAllocator = m_currFrameResource->m_cmdAllocator;	// ��ȡ��ǰ֡��Դ�����������
	ThrowIfFailed(m_currFrameResource->m_cmdAllocator->Reset());
	ThrowIfFailed(m_cmdList->Reset(m_currFrameResource->m_cmdAllocator.Get(), m_PSO.Get())); // �ǵô���PSO

	UINT& ref_mCurrentBackBuffer = m_currentBackBuffer;
	//2���л���̨������״̬��������Դ���ϣ�
	CD3DX12_RESOURCE_BARRIER resBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_swapChainBuffer[ref_mCurrentBackBuffer].Get(),
		D3D12_RESOURCE_STATE_PRESENT,		// ״̬������
		D3D12_RESOURCE_STATE_RENDER_TARGET  // ״̬����ȾĿ��
	);

	m_cmdList->ResourceBarrier(
		1,
		&resBarrier
	);

	//3�������ӿںͲü�����
	m_cmdList->RSSetViewports(1, &m_viewPort);
	m_cmdList->RSSetScissorRects(1, &m_scissorRect);

	//4������RTV��������
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
		ref_mCurrentBackBuffer,
		m_rtvDescriptorSize);
	// �����ȾĿ��
	m_cmdList->ClearRenderTargetView(rtvHandle, DirectX::Colors::AliceBlue, 0, nullptr);
	// ���ģ����������
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
	m_cmdList->ClearDepthStencilView(
		dsvHandle, // DSV���
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
		1.0f, // 
		0,    // 
		0,    // 
		nullptr // 
	);



	//5������ȾĿ������ģ�� ���õ����������
	m_cmdList->OMSetRenderTargets(
		1,    //���󶨵�RTV����
		&rtvHandle,//ָ��RTV�����ָ��
		true, //RTV�����ڶ��ڴ�����������ŵ�
		&dsvHandle//ָ��DSV��ָ��
	);


	///////////////////////////////////////////////////////////////
	// ��Ⱦͼ�εĴ���
	///////////////////////////////////////////////////////////////
	// �󶨸�ǩ��
	m_cmdList->SetGraphicsRootSignature(m_rootSignature.Get());

	// ����ȫ�ֵĳ�����������װviewproj����
	auto passCB = m_currFrameResource->m_passCB->Resource();
	m_cmdList->SetGraphicsRootConstantBufferView(
		1, 
		passCB->GetGPUVirtualAddress()
	);

	UpdateWaves(m_gt);

	// ������Ⱦ��
	DrawRenderItems();


	//6��������ϣ��л���̨������Ϊ����״̬
	resBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_swapChainBuffer[ref_mCurrentBackBuffer].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	);

	m_cmdList->ResourceBarrier(
		1,
		&resBarrier
	);

	// ���³���������
	Update();

	// 7���ر������б����������б��е�����͵����������
	ThrowIfFailed(m_cmdList->Close());

	ID3D12CommandList* commandLists[] = { m_cmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	//ThrowIfFailed(m_swapChain->Present(0, 0)); // ��ֱͬ��ģʽ��ֻ��60֡
	ThrowIfFailed(m_swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING));
	ref_mCurrentBackBuffer = (ref_mCurrentBackBuffer + 1) % 2;


	//10��Χ��ͬ��CPU��GPU
	//FlushCmdQueue();
	m_currFrameResource->m_fenceCPU = ++m_currentFence;
	// Signal�����������ִ��������ָ��󣬰�GPU_Fence������Ǵ����m_currentFence
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
	//��ʼ�������б�ÿ��������3��������
	std::vector<std::uint16_t> indices(3 * m_waves->TriangleCount());
	assert(m_waves->VertexCount() < 0x0000ffff);//��������������65536����ֹ����

	//��������б�
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
	//���㶥������������С
	UINT vbByteSize = m_waves->VertexCount() * sizeof(Vertex);
	UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->name = "lakeGeo";

	geo->m_vertexBufferCPU = nullptr;
	geo->m_vertexBufferGPU = nullptr;
	//����������CPUϵͳ�ڴ�
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->m_indexBufferCPU));
	//�������б����CPUϵͳ�ڴ�
	CopyMemory(geo->m_indexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);
	//����������ͨ���ϴ��Ѵ���Ĭ�϶�
	geo->m_indexBufferGPU = ToolFunc::CreateDefaultBuffer(m_d3dDevice.Get(),
		m_cmdList.Get(),
		ibByteSize,
		indices.data(),
		geo->m_indexBufferUploader);

	//��ֵMeshGeomety���������
	geo->m_vbByteSize = vbByteSize;
	geo->m_vByteStride = sizeof(Vertex);
	geo->m_indexFormat = DXGI_FORMAT_R16_UINT;
	geo->m_ibByteSize = ibByteSize;

	SubmeshGeometry LakeSubmesh;
	LakeSubmesh.baseVertexLocation = 0;
	LakeSubmesh.startIndexLocation = 0;
	LakeSubmesh.indexCount = (UINT)indices.size();
	//ʹ��grid������
	geo->m_mapDrawArgs["lakeGrid"] = LakeSubmesh;
	//������MeshGeometry
	m_geometries["lakeGeo"] = std::move(geo);
}

void LandAndWaves::UpdateWaves(const GameTime& gt)
{
	static float t_base = 0.0f;
	if ((m_gt.TotalTime() - t_base) >= 0.25f)
	{
		t_base += 0.25f;	//0.25������һ������
		//������ɺ�����
		int i = MathHelper::Rand(4, m_waves->RowCount() - 5);
		//�������������
		int j = MathHelper::Rand(4, m_waves->ColumnCount() - 5);
		//������ɲ��İ뾶
		float r = MathHelper::RandF(0.2f, 0.5f);//float��RandF����
		//ʹ�ò������̺������ɲ���
		m_waves->Disturb(i, j, r);
	}

	//ÿ֡���²���ģ�⣨�����¶������꣩
	m_waves->Update(gt.DeltaTime());

	//�����µĶ����������GPU�ϴ�����
	auto currWavesVB = m_currFrameResource->m_waveVB.get();
	for (int i = 0; i < m_waves->VertexCount(); i++)
	{
		Vertex v;
		v.Pos = m_waves->Position(i);
		v.Color = XMFLOAT4(DirectX::Colors::Blue);

		currWavesVB->CopyData(i, v);
	}
	//��ֵ������GPU�ϵĶ��㻺��
	m_waveRitem->geo->m_vertexBufferGPU = currWavesVB->Resource();
}