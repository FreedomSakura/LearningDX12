#include "D3D12InitApp.h"


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
}

// �����ϴ��� & Ĭ�϶�
ComPtr<ID3D12Resource> D3D12InitApp::CreateDefaultBuffer(UINT64 byteSize, const void* initData, ComPtr<ID3D12Resource>& uploadBuffer) {
	// �����ϴ��ѣ������ǣ�д��CPU�ڴ����ݣ��������Ĭ�϶�
	ThrowIfFailed(
		m_d3dDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // ��������Ϊ �ϴ��� �Ķ�
			D3D12_HEAP_FLAG_NONE,	
			&CD3DX12_RESOURCE_DESC::Buffer(byteSize),	// ���캯�������д����
			D3D12_RESOURCE_STATE_GENERIC_READ,			// �ϴ��������Դ��Ҫ���Ƹ�Ĭ�϶ѣ����ǿɶ�״̬
			nullptr,
			IID_PPV_ARGS(&uploadBuffer)
		)
	);

	// ����Ĭ�϶ѣ���Ϊ�ϴ��ѵ����ݴ������
	ComPtr<ID3D12Resource> defaultBuffer;
	ThrowIfFailed(
		m_d3dDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
			D3D12_RESOURCE_STATE_COMMON,		// Ĭ�϶�Ϊ���մ洢���ݵĵط���������ʱ��ʼ��Ϊ��ͨ״̬
			nullptr,
			IID_PPV_ARGS(&defaultBuffer)
		)
	);

	// ����Դ��COMMON״̬ת����COPY_DEST״̬��Ĭ�϶Ѵ�ʱ��Ϊ�������ݵ�Ŀ�꣩
	m_cmdList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			defaultBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_COPY_DEST
		)
	);

	// �����ݴ�CPU�ڴ濽����GPU����
	D3D12_SUBRESOURCE_DATA subResourceData;
	subResourceData.pData = initData;
	subResourceData.RowPitch = byteSize;
	subResourceData.SlicePitch = subResourceData.RowPitch;
	// �����ݴ�CPU�ڴ濽�����ϴ��ѣ��ٴ��ϴ��ѿ�����Ĭ�϶�
	UpdateSubresources<1>(m_cmdList.Get(), defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);


	// �ٽ�Ĭ�϶���Դ��COPY_DESTתΪGENERIC_READ״̬��ֻ�ṩ����ɫ�����ʣ���
	m_cmdList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_GENERIC_READ
		)
	);

	return defaultBuffer;
}

// �����������������Ĵ��붼�е����⣬���죨7-10���ٸģ�
// �������㻺������������vbv�� & ������������������ibv��
bool D3D12InitApp::CreateVBV() {
	ComPtr<ID3D12Resource> vertexBufferUploader = nullptr;
	vertexBufferGPU = D3D12InitApp::CreateDefaultBuffer(sizeof(vertices), vertices, vertexBufferUploader);
	
	// ���������ݰ�����Ⱦ��ˮ��
	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = vertexBufferGPU->GetGPUVirtualAddress();	// ���㻺������Դ�������ַ
	vbv.SizeInBytes = sizeof(Vertex) * 8;
	vbv.StrideInBytes = sizeof(Vertex);
	
	// ���ö��㻺����
	m_cmdList->IASetVertexBuffers(0, 1, &vbv);
}

bool D3D12InitApp::CreateIBV() {
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &indexBufferCpu));	//�������������ڴ�ռ�

	CopyMemory(indexBufferCpu->GetBufferPointer(), indices.data(), ibByteSize);	//���������ݿ���������ϵͳ�ڴ���

	indexBufferGpu = ToolFunc::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), ibByteSize, indices.data(), indexBufferUploader);

	D3D12_INDEX_BUFFER_VIEW ibv;
	ibv.BufferLocation = indexBufferGpu->GetGPUVirtualAddress();
	ibv.Format = DXGI_FORMAT_R16_UINT;
	ibv.SizeInBytes = ibByteSize;
	//��������������
	cmdList->IASetIndexBuffer(&ibv());
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