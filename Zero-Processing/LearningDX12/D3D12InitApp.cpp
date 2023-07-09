#include "D3D12InitApp.h"


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
	// 清除RT背景色为暗红，并且不设置裁剪矩形
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
}

// 创建上传堆 & 默认堆
ComPtr<ID3D12Resource> D3D12InitApp::CreateDefaultBuffer(UINT64 byteSize, const void* initData, ComPtr<ID3D12Resource>& uploadBuffer) {
	// 创建上传堆，作用是：写入CPU内存数据，并传输给默认堆
	ThrowIfFailed(
		m_d3dDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // 创建类型为 上传堆 的堆
			D3D12_HEAP_FLAG_NONE,	
			&CD3DX12_RESOURCE_DESC::Buffer(byteSize),	// 构造函数的奇怪写法！
			D3D12_RESOURCE_STATE_GENERIC_READ,			// 上传堆里的资源需要复制给默认堆，故是可读状态
			nullptr,
			IID_PPV_ARGS(&uploadBuffer)
		)
	);

	// 创建默认堆，作为上传堆的数据传输对象
	ComPtr<ID3D12Resource> defaultBuffer;
	ThrowIfFailed(
		m_d3dDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
			D3D12_RESOURCE_STATE_COMMON,		// 默认堆为最终存储数据的地方，所以暂时初始化为普通状态
			nullptr,
			IID_PPV_ARGS(&defaultBuffer)
		)
	);

	// 将资源从COMMON状态转换到COPY_DEST状态（默认堆此时作为接收数据的目标）
	m_cmdList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			defaultBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_COPY_DEST
		)
	);

	// 将数据从CPU内存拷贝到GPU缓存
	D3D12_SUBRESOURCE_DATA subResourceData;
	subResourceData.pData = initData;
	subResourceData.RowPitch = byteSize;
	subResourceData.SlicePitch = subResourceData.RowPitch;
	// 将数据从CPU内存拷贝至上传堆，再从上传堆拷贝至默认堆
	UpdateSubresources<1>(m_cmdList.Get(), defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);


	// 再将默认堆资源从COPY_DEST转为GENERIC_READ状态（只提供给着色器访问！）
	m_cmdList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_GENERIC_READ
		)
	);

	return defaultBuffer;
}

// 这三个缓冲区创建的代码都有点问题，明天（7-10）再改！
// 创建顶点缓冲区描述符（vbv） & 索引缓冲区描述符（ibv）
bool D3D12InitApp::CreateVBV() {
	ComPtr<ID3D12Resource> vertexBufferUploader = nullptr;
	vertexBufferGPU = D3D12InitApp::CreateDefaultBuffer(sizeof(vertices), vertices, vertexBufferUploader);
	
	// 将顶点数据绑定至渲染流水线
	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = vertexBufferGPU->GetGPUVirtualAddress();	// 顶点缓冲区资源的虚拟地址
	vbv.SizeInBytes = sizeof(Vertex) * 8;
	vbv.StrideInBytes = sizeof(Vertex);
	
	// 设置顶点缓冲区
	m_cmdList->IASetVertexBuffers(0, 1, &vbv);
}

bool D3D12InitApp::CreateIBV() {
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &indexBufferCpu));	//创建索引数据内存空间

	CopyMemory(indexBufferCpu->GetBufferPointer(), indices.data(), ibByteSize);	//将索引数据拷贝至索引系统内存中

	indexBufferGpu = ToolFunc::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), ibByteSize, indices.data(), indexBufferUploader);

	D3D12_INDEX_BUFFER_VIEW ibv;
	ibv.BufferLocation = indexBufferGpu->GetGPUVirtualAddress();
	ibv.Format = DXGI_FORMAT_R16_UINT;
	ibv.SizeInBytes = ibByteSize;
	//设置索引缓冲区
	cmdList->IASetIndexBuffer(&ibv());
}


bool D3D12InitApp::CreateCBV() {
	UINT objConstSize = CalcConstantBufferByteSize(sizeof(ObjectConstants));
	//创建CBV堆
	ComPtr<ID3D12DescriptorHeap> cbvHeap;
	D3D12_DESCRIPTOR_HEAP_DESC cbHeapDesc;
	cbHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbHeapDesc.NumDescriptors = 1;
	cbHeapDesc.NodeMask = 0;
	ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&cbHeapDesc, IID_PPV_ARGS(&cbvHeap)));

	//定义并获得物体的常量缓冲区，然后得到其首地址
	std::unique_ptr<UploadBufferResource<ObjectConstants>> objCB = nullptr;
	//elementCount为1（1个子物体常量缓冲元素），isConstantBuffer为ture（是常量缓冲区）
	objCB = std::make_unique<UploadBufferResource<ObjectConstants>>(m_d3dDevice.Get(), 1, true);
	//获得常量缓冲区首地址
	D3D12_GPU_VIRTUAL_ADDRESS address;
	address = objCB->Resource()->GetGPUVirtualAddress();
	//通过常量缓冲区元素偏移值计算最终的元素地址
	int cbElementIndex = 0;	//常量缓冲区元素下标
	address += cbElementIndex * objConstSize;
	//创建CBV描述符
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = address;
	cbvDesc.SizeInBytes = objConstSize;
	m_d3dDevice->CreateConstantBufferView(&cbvDesc, cbvHeap->GetCPUDescriptorHandleForHeapStart());
}