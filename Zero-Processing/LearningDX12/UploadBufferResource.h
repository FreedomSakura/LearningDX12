#pragma once

#include "ToolFunc.h"

/// <summary>
/// 一个通用类，用于创建上传堆和资源
/// </summary>
/// <typeparam name="T"></typeparam>
template<class T>
class UploadBufferResource
{
public:
	UploadBufferResource(ComPtr<ID3D12Device> d3dDevice, UINT elementCount, bool isConstantBuffer);
	~UploadBufferResource();

public:
	void CopyData(int elementIndex, const T& Data);
	ComPtr<ID3D12Resource> Resource() const;

private:
	bool m_IsConstantBuffer;
	UINT m_elementByteSize;
	// 很可能不是这个类型！
	ComPtr<ID3D12Resource> m_uploadBuffer;
	BYTE* m_mappedData;
};

// 奇怪的函数，用来钳制常量缓冲区大小（返回256的倍数！）
// 为什么是256？好像说是硬件原因，不同硬件不一样？
inline UINT CalcConstantBufferByteSize(UINT byteSize)
{
	return (byteSize + 255) & ~255;
}


template<class T>
UploadBufferResource<T>::UploadBufferResource(ComPtr<ID3D12Device> d3dDevice, UINT elementCount, bool isConstantBuffer)
	: m_IsConstantBuffer(isConstantBuffer)
{
	m_elementByteSize = sizeof(T);//如果不是常量缓冲区，则直接计算缓存大小

	if (isConstantBuffer)
		m_elementByteSize = CalcConstantBufferByteSize(sizeof(T));//如果是常量缓冲区，则以256的倍数计算缓存大小

	// 创建上传堆和资源
	ThrowIfFailed(d3dDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(m_elementByteSize * elementCount),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_uploadBuffer)
		)
	);

	// 返回欲更新资源的指针
	ThrowIfFailed(m_uploadBuffer->Map(
		0,	//子资源索引，对于缓冲区来说，他的子资源就是自己
		nullptr,			//对整个资源进行映射
		reinterpret_cast<void**>(&m_mappedData)));//返回待映射资源数据的目标内存块	
}

template<class T>
UploadBufferResource<T>::~UploadBufferResource()
{
	if (m_uploadBuffer != nullptr)
		m_uploadBuffer->Unmap(0, nullptr); //取消映射

	//释放映射内存
	m_mappedData = nullptr;
}


template<class T>
void UploadBufferResource<T>::CopyData(int elementIndex, const T& Data)
{
	memcpy(&m_mappedData[elementIndex * m_elementByteSize], &Data, sizeof(T));
}

template<class T>
ComPtr<ID3D12Resource> UploadBufferResource<T>::Resource() const
{
	return m_uploadBuffer;
}