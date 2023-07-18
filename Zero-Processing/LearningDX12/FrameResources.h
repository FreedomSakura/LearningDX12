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

// ������������Ҫ��ŵ�����
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

	// ÿ֡��Դ����Ҫ���������������
	ComPtr<ID3D12CommandAllocator> m_cmdAllocator;
	// ÿ֡����Ҫ��������Դ������
	std::unique_ptr<UploadBufferResource<ObjectConstants>> m_objCB = nullptr;
	std::unique_ptr<UploadBufferResource<PassConstants>> m_passCB = nullptr; 
	std::unique_ptr<UploadBufferResource<Vertex> > m_waveVB = nullptr;  // ��̬���㻺������ÿ֡����Ҫ����
	// CPUΧ��ֵ
	UINT64 m_fenceCPU = 0;
};
