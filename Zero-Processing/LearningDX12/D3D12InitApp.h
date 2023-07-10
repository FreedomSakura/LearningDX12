#pragma once

#include "D3D12App.h"

using namespace DirectX;

class D3D12InitApp : public D3D12App
{
public:
    D3D12InitApp() {};
    ~D3D12InitApp() {};
private:
    virtual bool Draw() override;

	// 创建顶点缓冲区描述符（vbv） & 索引缓冲区描述符（ibv）
	bool CreateVBV();
	bool CreateIBV();
	bool CreateCBV();
	// 创建根签名
	void BuildRootSignature();
	// 输入布局描述与编译shader字节码
	void BuildByteCodeAndInputLayout();
	// 构建几何体（准备好顶点和索引数据）
	void BuildGeometry();
	// 构建PSO（PipeLineStateObject）—— 将之前定义的各种东西绑定到渲染流水线上！
	void BuildPSO();

private:
	// 上传堆
	ComPtr<ID3DBlob> m_vertexBufferCPU;
	ComPtr<ID3DBlob> m_indexBufferCPU;
	// 默认堆
	ComPtr<ID3D12Resource> m_vertexBufferGPU;
	ComPtr<ID3D12Resource> m_indexBufferGPU;

	UINT m_vbByteSize;
	UINT m_ibByteSize;

	ComPtr<ID3D12Resource> m_vertexBufferUploader = nullptr;
	ComPtr<ID3D12Resource> m_indexBufferUploader = nullptr;

	// 根签名
	ComPtr<ID3D12RootSignature> m_rootSignature;

	// 输入布局描述（类似UnityLab里的 : Position）
	//std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayoutDesc = {
	//	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	//	{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	//};
	// Shader部分
	// 输入布局描述（类似UnityLab里的 : Position）
	std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayoutDesc = { };
	// Shader编译后得到的字节码
	ComPtr<ID3DBlob> m_vsBytecode;
	ComPtr<ID3DBlob> m_psBytecode;

	// 流水线状态对象
	ComPtr<ID3D12PipelineState> m_PSO; // 暂时还不知道是什么类型的

};

