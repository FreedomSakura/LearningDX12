#pragma once

#include "ToolFunc.h"

struct SubmeshGeometry
{
	UINT indexCount;
	UINT startIndexLocation;
	UINT baseVertexLocation;
};

struct MeshGeometry;

// ��Ⱦ��
struct RenderItem
{
	RenderItem() = default;

	// �ü�������������
	DirectX::XMFLOAT4X4 world = MathHelper::Identity4x4();

	// �ü�����ĳ���������objConstantBuffer�е�����
	UINT objCBIndex = -1;

	// �ü������ͼԪ��������
	D3D12_PRIMITIVE_TOPOLOGY primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// �ü�����Ļ���������
	UINT indexCount = 0;
	UINT startIndexLocation = 0;
	UINT baseVertexLocation = 0;

	MeshGeometry* geo = nullptr;
};

// ��Ÿ�ģ�͵����еĶ���&������Դ
struct MeshGeometry
{
	std::string name;

	// CPU�ڴ�
	ComPtr<ID3DBlob> m_vertexBufferCPU;
	ComPtr<ID3DBlob> m_indexBufferCPU;
	// �ϴ�����Դ
	ComPtr<ID3D12Resource> m_vertexBufferUploader = nullptr;
	ComPtr<ID3D12Resource> m_indexBufferUploader = nullptr;
	// Ĭ�϶�
	ComPtr<ID3D12Resource> m_vertexBufferGPU;
	ComPtr<ID3D12Resource> m_indexBufferGPU;

	UINT m_vbByteSize = 0;
	UINT m_vByteStride = 0;
	UINT m_ibByteSize = 0;
	DXGI_FORMAT m_indexFormat = DXGI_FORMAT_R16_UINT;

	// ��ͬ���������������
	std::unordered_map<std::string, SubmeshGeometry> m_mapDrawArgs;

	D3D12_VERTEX_BUFFER_VIEW GetVbv()const
	{
		D3D12_VERTEX_BUFFER_VIEW vbv;
		vbv.BufferLocation = m_vertexBufferGPU->GetGPUVirtualAddress();//���㻺������Դ�����ַ
		vbv.SizeInBytes = m_vbByteSize;	//���㻺������С�����ж������ݴ�С��
		vbv.StrideInBytes = m_vByteStride;	//ÿ������Ԫ����ռ�õ��ֽ���

		return vbv;
	}

	D3D12_INDEX_BUFFER_VIEW GetIbv() const
	{
		D3D12_INDEX_BUFFER_VIEW ibv;
		ibv.BufferLocation = m_indexBufferGPU->GetGPUVirtualAddress();
		ibv.Format = m_indexFormat;
		ibv.SizeInBytes = m_ibByteSize;

		return ibv;
	}

	// ���ϴ�����Դ����Ĭ�϶Ѻ��ͷ��ϴ�������ڴ�
	void DisposeUploaders()
	{
		m_vertexBufferCPU = nullptr;
		m_indexBufferCPU = nullptr;
	}
};