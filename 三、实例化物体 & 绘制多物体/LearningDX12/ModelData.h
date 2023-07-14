#pragma once

#include <DirectXMath.h>
#include <DirectXColors.h>
#include <array>

#include "DX12Utils/MathHelper.h"

using namespace DirectX;

//���嶥��ṹ��
struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};
//ʵ��������ṹ�岢���
std::array<Vertex, 8> vertices =
{
	Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White) }),
	Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black) }),
	Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) }),
	Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
	Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) }),
	Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow) }),
	Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan) }),
	Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) })
};

std::array<std::uint16_t, 36> indices =
{
	//ǰ
	0, 1, 2,
	0, 2, 3,

	//��
	4, 6, 5,
	4, 7, 6,

	//��
	4, 5, 1,
	4, 1, 0,

	//��
	3, 2, 6,
	3, 6, 7,

	//��
	1, 5, 6,
	1, 6, 2,

	//��
	4, 0, 3,
	4, 3, 7
};

//��������ĳ�������
struct ObjectConstants
{
	//��ʼ������ռ�任���ü��ռ����Identity4x4()�ǵ�λ������Ҫ����MathHelperͷ�ļ�
	XMFLOAT4X4 worldViewProj = MathHelper::Identity4x4();
};