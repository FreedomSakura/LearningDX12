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
struct SubmeshGeometry
{
	UINT indexCount;
	UINT startIndexLocation;
	UINT baseVertexLocation;
};

class D3D12InitApp : public D3D12App
{
public:
    D3D12InitApp() {};
    ~D3D12InitApp() {};

public:
	virtual bool Init(HINSTANCE hInstance, int nShowCmd) override;

private:
    virtual bool Draw() override;
	virtual void Update() override;	// 更新矩阵（实际上是更新 常量缓冲区）
	virtual void OnResize() override;
	virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y) override;

	// 绘制图形需要添加的管线操作
	bool CreateCBV();
	void BuildRootSignature(); // 创建根签名
	void BuildByteCodeAndInputLayout(); // 输入布局描述与编译shader字节码
	void BuildGeometry(); // 构建几何体（准备好顶点和索引数据）
	void BuildPSO(); // 构建PSO（PipeLineStateObject）—— 将之前定义的各种东西绑定到渲染流水线上！

	// 构建渲染项！（实例化物体！）
	void BuildRenderItem();
	void DrawRenderItems(); // 绘制多个物体
	void BuildFrameResources(); // 构建帧资源

	// 获取顶点缓冲区描述符 & 索引缓冲区描述符
	D3D12_VERTEX_BUFFER_VIEW GetVbv() const;
	D3D12_INDEX_BUFFER_VIEW GetIbv() const;

private:
	// CPU内存
	ComPtr<ID3DBlob> m_vertexBufferCPU;
	ComPtr<ID3DBlob> m_indexBufferCPU;
	// 默认堆
	ComPtr<ID3D12Resource> m_vertexBufferGPU;
	ComPtr<ID3D12Resource> m_indexBufferGPU;
	
	UINT m_vbByteSize = 0;
	UINT m_ibByteSize = 0;
	// 上传堆资源
	ComPtr<ID3D12Resource> m_vertexBufferUploader = nullptr;
	ComPtr<ID3D12Resource> m_indexBufferUploader = nullptr;
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
	std::vector<std::unique_ptr<RenderItem>> m_allRItem;
	// 帧资源数量
	int m_frameResourceCount = 3;
	std::vector<std::unique_ptr<FrameResources>> m_frameResourcesList;
	int m_currFrameResourceIndex = 0;
	FrameResources* m_currFrameResource = nullptr;
};

