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

	// �������㻺������������vbv�� & ������������������ibv��
	bool CreateVBV();
	bool CreateIBV();
	bool CreateCBV();
	// ������ǩ��
	void BuildRootSignature();
	// ���벼�����������shader�ֽ���
	void BuildByteCodeAndInputLayout();
	// ���������壨׼���ö�����������ݣ�
	void BuildGeometry();
	// ����PSO��PipeLineStateObject������ ��֮ǰ����ĸ��ֶ����󶨵���Ⱦ��ˮ���ϣ�
	void BuildPSO();

private:
	// �ϴ���
	ComPtr<ID3DBlob> m_vertexBufferCPU;
	ComPtr<ID3DBlob> m_indexBufferCPU;
	// Ĭ�϶�
	ComPtr<ID3D12Resource> m_vertexBufferGPU;
	ComPtr<ID3D12Resource> m_indexBufferGPU;

	UINT m_vbByteSize;
	UINT m_ibByteSize;

	ComPtr<ID3D12Resource> m_vertexBufferUploader = nullptr;
	ComPtr<ID3D12Resource> m_indexBufferUploader = nullptr;

	// ��ǩ��
	ComPtr<ID3D12RootSignature> m_rootSignature;

	// ���벼������������UnityLab��� : Position��
	//std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayoutDesc = {
	//	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	//	{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	//};
	// Shader����
	// ���벼������������UnityLab��� : Position��
	std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayoutDesc = { };
	// Shader�����õ����ֽ���
	ComPtr<ID3DBlob> m_vsBytecode;
	ComPtr<ID3DBlob> m_psBytecode;

	// ��ˮ��״̬����
	ComPtr<ID3D12PipelineState> m_PSO; // ��ʱ����֪����ʲô���͵�

};

