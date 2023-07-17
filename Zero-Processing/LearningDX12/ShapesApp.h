#pragma once

#include <d3dcompiler.h>
#include <map>
#include <unordered_map>

#include "D3D12App.h"
#include "UploadBufferResource.h"
#include "GeometryGenerator.h"
#include "RenderItem.h"
#include "FrameResources.h"

using namespace DirectX;

struct ObjectConstants;
struct PassConstants;

class ShapesApp : public D3D12App
{
public:
    ShapesApp() {};
    ~ShapesApp() {};

public:
	virtual bool Init(HINSTANCE hInstance, int nShowCmd) override;

protected:
    virtual bool Draw() override;
	virtual void Update() override;	// 更新矩阵（实际上是更新 常量缓冲区）
	virtual void OnResize() override;
	virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y) override;

	// 绘制图形需要添加的管线操作
	bool CreateCBV();
	virtual void BuildRootSignature(); // 创建根签名
	void BuildByteCodeAndInputLayout(); // 输入布局描述与编译shader字节码
	virtual void BuildGeometry(); // 构建几何体（准备好顶点和索引数据）
	void BuildPSO(); // 构建PSO（PipeLineStateObject）—— 将之前定义的各种东西绑定到渲染流水线上！

	// 构建渲染项！（实例化物体！）
	virtual void BuildRenderItem();
	virtual void DrawRenderItems(); // 绘制多个物体
	virtual void BuildFrameResources(); // 构建帧资源

	float GetHillsHeight(float x, float z);

protected:
	// 常量缓冲区描述符堆
	ComPtr<ID3D12DescriptorHeap> m_cbvHeap;
	// 常量缓冲区资源对象
	std::unique_ptr<UploadBufferResource<ObjectConstants>> m_objCB = nullptr;
	std::unique_ptr<UploadBufferResource<PassConstants>> m_passCB = nullptr;

	ComPtr<ID3D12RootSignature> m_rootSignature; // 根签名
	ComPtr<ID3D12PipelineState> m_PSO = nullptr; // 流水线状态对象

	// Shader部分
	// 输入布局描述（类似UnityLab里的 : Position）
	std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayoutDesc;
	// Shader编译后得到的字节码
	ComPtr<ID3DBlob> m_vsBytecode;
	ComPtr<ID3DBlob> m_psBytecode;

	// 矩阵
	XMFLOAT4X4 m_world = MathHelper::Identity4x4();
	XMFLOAT4X4 m_proj = MathHelper::Identity4x4(); // 投影矩阵

	// 用来存放在BuildGeometry阶段确定的各个物体的DrawCall参数，用于后续DrawCall使用
	std::map<std::string, SubmeshGeometry> m_mapDrawArgs;
	// 渲染项
	std::vector<std::unique_ptr<RenderItem>> m_allRItems;
	// 帧资源数量
	int m_frameResourceCount = 3;
	std::vector<std::unique_ptr<FrameResources>> m_frameResourcesList;
	int m_currFrameResourceIndex = 0;
	FrameResources* m_currFrameResource = nullptr;

	// 动态顶点缓冲区
	std::map<std::string, std::unique_ptr<MeshGeometry>> m_geometries;
};

