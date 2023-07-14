#pragma once

#include "ToolFunc.h"

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
};