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
	m_cmdList->ClearRenderTargetView(rtvHandle, DirectX::Colors::DarkRed, 0, nullptr);
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
