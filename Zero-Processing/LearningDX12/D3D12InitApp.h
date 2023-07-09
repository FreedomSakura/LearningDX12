#pragma once

#include "D3D12App.h"
#include "UploadBufferResource.h"

using namespace DirectX;

class D3D12InitApp : public D3D12App
{
public:
    D3D12InitApp() {};
    ~D3D12InitApp() {};
private:
    virtual bool Draw() override;
	// �����ϴ��� & Ĭ�϶�
	ComPtr<ID3D12Resource> CreateDefaultBuffer(UINT64 byteSize, const void* initData, ComPtr<ID3D12Resource>& uploadBuffer);

	// �������㻺������������vbv�� & ������������������ibv��
	bool CreateVBV();
	bool CreateIBV();
	bool CreateCBV();
};

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

// ��ֵĺ���������ǯ�Ƴ�����������С������256�ı�������
// Ϊʲô��256������˵��Ӳ��ԭ�򣬲�ͬӲ����һ����
inline UINT CalcConstantBufferByteSize(UINT byteSize)
{
	return (byteSize + 255) & ~255;
}