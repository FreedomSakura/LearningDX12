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
	// 创建上传堆 & 默认堆
	ComPtr<ID3D12Resource> CreateDefaultBuffer(UINT64 byteSize, const void* initData, ComPtr<ID3D12Resource>& uploadBuffer);

	// 创建顶点缓冲区描述符（vbv） & 索引缓冲区描述符（ibv）
	bool CreateVBV();
	bool CreateIBV();
	bool CreateCBV();
};

//定义顶点结构体
struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};
//实例化顶点结构体并填充
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
	//前
	0, 1, 2,
	0, 2, 3,

	//后
	4, 6, 5,
	4, 7, 6,

	//左
	4, 5, 1,
	4, 1, 0,

	//右
	3, 2, 6,
	3, 6, 7,

	//上
	1, 5, 6,
	1, 6, 2,

	//下
	4, 0, 3,
	4, 3, 7
};


//单个物体的常量数据
struct ObjectConstants
{
	//初始化物体空间变换到裁剪空间矩阵，Identity4x4()是单位矩阵，需要包含MathHelper头文件
	XMFLOAT4X4 worldViewProj = MathHelper::Identity4x4();
};

// 奇怪的函数，用来钳制常量缓冲区大小（返回256的倍数！）
// 为什么是256？好像说是硬件原因，不同硬件不一样？
inline UINT CalcConstantBufferByteSize(UINT byteSize)
{
	return (byteSize + 255) & ~255;
}