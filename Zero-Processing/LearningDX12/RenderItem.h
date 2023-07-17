#pragma once

#include "ToolFunc.h"

struct SubmeshGeometry
{
	UINT indexCount;
	UINT startIndexLocation;
	UINT baseVertexLocation;
};

struct MeshGeometry;

// 渲染项
struct RenderItem
{
	RenderItem() = default;

	// 该几何体的世界矩阵
	DirectX::XMFLOAT4X4 world = MathHelper::Identity4x4();

	// 该几何体的常量数据在objConstantBuffer中的索引
	UINT objCBIndex = -1;

	// 该几何体的图元拓扑类型
	D3D12_PRIMITIVE_TOPOLOGY primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// 该几何体的绘制三参数
	UINT indexCount = 0;
	UINT startIndexLocation = 0;
	UINT baseVertexLocation = 0;

	MeshGeometry* geo = nullptr;
};

// 存放该模型的所有的顶点&索引资源
struct MeshGeometry
{
	std::string name;

	// CPU内存
	ComPtr<ID3DBlob> m_vertexBufferCPU;
	ComPtr<ID3DBlob> m_indexBufferCPU;
	// 上传堆资源
	ComPtr<ID3D12Resource> m_vertexBufferUploader = nullptr;
	ComPtr<ID3D12Resource> m_indexBufferUploader = nullptr;
	// 默认堆
	ComPtr<ID3D12Resource> m_vertexBufferGPU;
	ComPtr<ID3D12Resource> m_indexBufferGPU;

	UINT m_vbByteSize = 0;
	UINT m_vByteStride = 0;
	UINT m_ibByteSize = 0;
	DXGI_FORMAT m_indexFormat = DXGI_FORMAT_R16_UINT;

	// 不同子物体绘制三参数
	std::unordered_map<std::string, SubmeshGeometry> m_mapDrawArgs;

	D3D12_VERTEX_BUFFER_VIEW GetVbv()const
	{
		D3D12_VERTEX_BUFFER_VIEW vbv;
		vbv.BufferLocation = m_vertexBufferGPU->GetGPUVirtualAddress();//顶点缓冲区资源虚拟地址
		vbv.SizeInBytes = m_vbByteSize;	//顶点缓冲区大小（所有顶点数据大小）
		vbv.StrideInBytes = m_vByteStride;	//每个顶点元素所占用的字节数

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

	// 等上传堆资源传至默认堆后，释放上传堆里的内存
	void DisposeUploaders()
	{
		m_vertexBufferCPU = nullptr;
		m_indexBufferCPU = nullptr;
	}
};