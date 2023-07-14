#pragma once

#include "ToolFunc.h"

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
};