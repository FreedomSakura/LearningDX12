#pragma once
#include "ToolFunc.h"
#include "UploadBufferResource.h"

using namespace DirectX::PackedVector;
using namespace DirectX;


struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};

// 常量缓冲区中要存放的数据
struct ObjectConstants
{
	XMFLOAT4X4 world = MathHelper::Identity4x4();
};
struct PassConstants
{
	XMFLOAT4X4 viewProj = MathHelper::Identity4x4();
};

struct FrameResources
{
public:
	FrameResources(ID3D12Device* device, UINT passCount, UINT objCount);
	FrameResources(ID3D12Device* device, UINT passCount, UINT objCount, UINT wavesVertCount);
	FrameResources(const FrameResources& rhs) = delete;
	FrameResources& operator=(const FrameResources& rhs) = delete;
	~FrameResources();

	// 每帧资源都需要独立的命令分配器
	ComPtr<ID3D12CommandAllocator> m_cmdAllocator;
	// 每帧都需要单独的资源缓冲区
	std::unique_ptr<UploadBufferResource<ObjectConstants>> m_objCB = nullptr;
	std::unique_ptr<UploadBufferResource<PassConstants>> m_passCB = nullptr; 
	std::unique_ptr<UploadBufferResource<Vertex> > m_waveVB = nullptr;  // 动态顶点缓冲区，每帧都需要更新
	// CPU围栏值
	UINT64 m_fenceCPU = 0;
};
