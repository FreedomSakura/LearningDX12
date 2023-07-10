#include "D3D12InitApp.h"

//���嶥��ṹ��
struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};
//ʵ��������ṹ�岢���
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
	//ǰ
	0, 1, 2,
	0, 2, 3,

	//��
	4, 6, 5,
	4, 7, 6,

	//��
	4, 5, 1,
	4, 1, 0,

	//��
	3, 2, 6,
	3, 6, 7,

	//��
	1, 5, 6,
	1, 6, 2,

	//��
	4, 0, 3,
	4, 3, 7
};


//��������ĳ�������
struct ObjectConstants
{
	//��ʼ������ռ�任���ü��ռ����Identity4x4()�ǵ�λ������Ҫ����MathHelperͷ�ļ�
	XMFLOAT4X4 worldViewProj = MathHelper::Identity4x4();
};


// ���ƣ�����һ�ξ���һ֡����
//���ܣ���������Դ���õ���Ⱦ��ˮ���ϣ������շ�����������
bool D3D12InitApp::Draw() {
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
	m_cmdList->ClearRenderTargetView(rtvHandle, DirectX::Colors::AliceBlue, 0, nullptr);
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

	// 7��8������ʵ����һ�����ر�Command List��Ӧ�������������Command Queue�У�
	//7���������ļ�¼�ر������б�
	ThrowIfFailed(m_cmdList->Close());

	//8����CPU�����׼���ã�����ִ�е������б����GPU���������
	ID3D12CommandList* commandLists[] = { m_cmdList.Get() }; // ���������������б�����
	m_cmdQueue->ExecuteCommandLists(_countof(commandLists), commandLists); // ������������б����������

	//9������ǰ��̨������������0��1��1��0��
	//ThrowIfFailed(m_swapChain->Present(0, 0)); -> �ڶ���������Flag����ѡ��򿪴�ֱͬ����������������Щ����
	ThrowIfFailed(m_swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING));
	ref_mCurrentBackBuffer = (ref_mCurrentBackBuffer + 1) % 2;

	//10������Χ��ֵ������Χ����ˢ��������У�ʹ��CPU��GPUͬ��
	FlushCmdQueue();

	return true;
}


// �����������������Ĵ��붼�е����⣬���죨7-10���ٸģ�
// �������㻺������������vbv�� & ������������������ibv��
bool D3D12InitApp::CreateVBV() {
	m_vertexBufferGPU = ToolFunc::CreateDefaultBuffer(m_d3dDevice, m_cmdList, sizeof(vertices), vertices.data(), m_vertexBufferUploader);
	
	// ���������ݰ�����Ⱦ��ˮ��
	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = m_vertexBufferGPU->GetGPUVirtualAddress();	// ���㻺������Դ�������ַ
	vbv.SizeInBytes = sizeof(Vertex) * 8;
	vbv.StrideInBytes = sizeof(Vertex);
	
	// ���ö��㻺����
	m_cmdList->IASetVertexBuffers(0, 1, &vbv);
}

bool D3D12InitApp::CreateIBV() {
	ThrowIfFailed(D3DCreateBlob(m_ibByteSize, &m_indexBufferCPU));	//�������������ڴ�ռ�

	CopyMemory(m_indexBufferCPU->GetBufferPointer(), indices.data(), m_ibByteSize);	//���������ݿ���������ϵͳ�ڴ���

	m_indexBufferGPU = ToolFunc::CreateDefaultBuffer(m_d3dDevice, m_cmdList, m_ibByteSize, indices.data(), m_indexBufferUploader);

	D3D12_INDEX_BUFFER_VIEW ibv;
	ibv.BufferLocation = m_indexBufferGPU->GetGPUVirtualAddress();
	ibv.Format = DXGI_FORMAT_R16_UINT;
	ibv.SizeInBytes = m_ibByteSize;
	//��������������
	m_cmdList->IASetIndexBuffer(&ibv);
}


bool D3D12InitApp::CreateCBV() {
	UINT objConstSize = CalcConstantBufferByteSize(sizeof(ObjectConstants));
	//����CBV��
	ComPtr<ID3D12DescriptorHeap> cbvHeap;
	D3D12_DESCRIPTOR_HEAP_DESC cbHeapDesc;
	cbHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbHeapDesc.NumDescriptors = 1;
	cbHeapDesc.NodeMask = 0;
	ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&cbHeapDesc, IID_PPV_ARGS(&cbvHeap)));

	//���岢�������ĳ�����������Ȼ��õ����׵�ַ
	std::unique_ptr<UploadBufferResource<ObjectConstants>> objCB = nullptr;
	//elementCountΪ1��1�������峣������Ԫ�أ���isConstantBufferΪture���ǳ�����������
	objCB = std::make_unique<UploadBufferResource<ObjectConstants>>(m_d3dDevice.Get(), 1, true);
	//��ó����������׵�ַ
	D3D12_GPU_VIRTUAL_ADDRESS address;
	address = objCB->Resource()->GetGPUVirtualAddress();
	//ͨ������������Ԫ��ƫ��ֵ�������յ�Ԫ�ص�ַ
	int cbElementIndex = 0;	//����������Ԫ���±�
	address += cbElementIndex * objConstSize;
	//����CBV������
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = address;
	cbvDesc.SizeInBytes = objConstSize;
	m_d3dDevice->CreateConstantBufferView(&cbvDesc, cbvHeap->GetCPUDescriptorHandleForHeapStart());
}

// ������ǩ��
void D3D12InitApp::BuildRootSignature() {
	// ������������������������������������
	CD3DX12_ROOT_PARAMETER slotRootParameter[1];
	// �����ɵ���CBV����ɵ���������
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV, // ����������
		1,// ����������
		0 // ���������󶨵ļĴ����ۺţ��е���TEXTURE0��������
	);

	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);

	// ��ǩ����һ�����������
	CD3DX12_ROOT_SIGNATURE_DESC rootSig(
		1, // ����������
		slotRootParameter, // ������ָ��
		0,
		nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	// �õ����Ĵ�����������һ����ǩ�����ò�λָ��һ�����е�������������������������
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(
		&rootSig, 
		D3D_ROOT_SIGNATURE_VERSION_1, 
		&serializedRootSig, 
		&errorBlob
	);


	if (!errorBlob)
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

// ������&�������ݸ��Ƶ�CPUϵͳ�ڴ棬��ʹ��CreateDefaultBuffer���临�Ƶ�GPU������
void D3D12InitApp::BuildGeometry() {
	// �����ڴ�ռ�
	ThrowIfFailed(D3DCreateBlob(m_vbByteSize, &m_vertexBufferCPU));
	ThrowIfFailed(D3DCreateBlob(m_ibByteSize, &m_indexBufferCPU));
	// ���Ƶ�CPUϵͳ�ڴ�
	CopyMemory(m_vertexBufferCPU->GetBufferPointer(), vertices.data(), m_vbByteSize);
	CopyMemory(m_indexBufferCPU->GetBufferPointer(), indices.data(), m_ibByteSize);
	// �������ݵ�GPU������
	m_vertexBufferGPU = ToolFunc::CreateDefaultBuffer(m_d3dDevice.Get(), m_cmdList.Get(), m_vbByteSize, vertices.data(), m_vertexBufferUploader);
	m_indexBufferGPU = ToolFunc::CreateDefaultBuffer(m_d3dDevice.Get(), m_cmdList.Get(), m_ibByteSize, indices.data(), m_indexBufferUploader);
}

// ����PSO��PipeLineStateObject��
// ��֮ǰ����ĸ��ֶ����󶨵���Ⱦ��ˮ���ϣ�
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
	psoDesc.SampleMask = UINT_MAX;	//0xffffffff,ȫ��������û������
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;	//��һ�����޷�������
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;	//��ʹ��4XMSAA
	psoDesc.SampleDesc.Quality = 0;	////��ʹ��4XMSAA

	
	ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_PSO)));
}

void D3D12InitApp::BuildByteCodeAndInputLayout() {
	// ���벼��
	m_inputLayoutDesc =
	{
		  { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		  { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// ��.hlsl�ļ�����Ϊ�ֽ���
	HRESULT hr = S_OK;
	m_vsBytecode = ToolFunc::CompileShader(L"Shaders\\color.hlsl", nullptr, "VS", "vs_5_0");
	m_psBytecode = ToolFunc::CompileShader(L"Shaders\\color.hlsl", nullptr, "PS", "ps_5_0");
}