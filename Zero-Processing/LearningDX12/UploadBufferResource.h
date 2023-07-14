#pragma once

#include "ToolFunc.h"

// ��ֵĺ���������ǯ�Ƴ�����������С������256�ı�������
// Ϊʲô��256������˵��Ӳ��ԭ�򣬲�ͬӲ����һ����
inline UINT CalcConstantBufferByteSize(UINT byteSize)
{
	return (byteSize + 255) & ~255;
}


/// <summary>
/// һ��ͨ���࣬���ڴ����ϴ��Ѻ���Դ
/// </summary>
/// <typeparam name="T"></typeparam>
template<class T>
class UploadBufferResource
{
public:
	UploadBufferResource(ComPtr<ID3D12Device> d3dDevice, UINT elementCount, bool isConstantBuffer) :
		m_IsConstantBuffer(isConstantBuffer)
	{
		m_elementByteSize = sizeof(T);//������ǳ�������������ֱ�Ӽ��㻺���С

		if (isConstantBuffer)
			m_elementByteSize = CalcConstantBufferByteSize(sizeof(T));//����ǳ���������������256�ı������㻺���С

		// �����ϴ��Ѻ���Դ
		ThrowIfFailed(d3dDevice->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(m_elementByteSize * elementCount),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&m_uploadBuffer)
			)
		);

		// ������������Դ��ָ��
		ThrowIfFailed(m_uploadBuffer->Map(
			0,	//����Դ���������ڻ�������˵����������Դ�����Լ�
			nullptr,			//��������Դ����ӳ��
			reinterpret_cast<void**>(&m_mappedData)));//���ش�ӳ����Դ���ݵ�Ŀ���ڴ��	
	}

	~UploadBufferResource()
	{
		if (m_uploadBuffer != nullptr)
			m_uploadBuffer->Unmap(0, nullptr); //ȡ��ӳ��

		//�ͷ�ӳ���ڴ�
		m_mappedData = nullptr;
	}

public:
	void CopyData(int elementIndex, const T& Data)
	{
		memcpy(&m_mappedData[elementIndex * m_elementByteSize], &Data, sizeof(T));
	}

	ID3D12Resource* Resource() const 
	{
		return m_uploadBuffer.Get();
	}

private:
	// �ܿ��ܲ���������ͣ�
	ComPtr<ID3D12Resource> m_uploadBuffer;
	BYTE* m_mappedData = nullptr;

	bool m_IsConstantBuffer = false;
	UINT m_elementByteSize = 0;
};

