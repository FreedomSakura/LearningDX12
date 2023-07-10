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
	m_cmdList->ClearRenderTargetView(rtvHandle, DirectX::Colors::DarkRed, 0, nullptr);
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

	return true;
}
