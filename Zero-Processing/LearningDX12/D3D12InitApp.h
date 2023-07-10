#pragma once

#include <d3dcompiler.h>

#include "D3D12App.h"
#include "UploadBufferResource.h"

//#include "ModelData.h"


using namespace DirectX;

struct ObjectConstants;

class D3D12InitApp : public D3D12App
{
public:
    D3D12InitApp() {};
    ~D3D12InitApp() {};

public:
	virtual bool Init(HINSTANCE hInstance, int nShowCmd) override;

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

	// ��ȡ���㻺���������� & ����������������
	D3D12_VERTEX_BUFFER_VIEW GetVbv() const;
	D3D12_INDEX_BUFFER_VIEW GetIbv() const;

	// ���¾���ʵ�����Ǹ��� ������������
	void Update();

private:
	// �ϴ���
	ComPtr<ID3DBlob> m_vertexBufferCPU;
	ComPtr<ID3DBlob> m_indexBufferCPU;
	// Ĭ�϶�
	ComPtr<ID3D12Resource> m_vertexBufferGPU;
	ComPtr<ID3D12Resource> m_indexBufferGPU;

	UINT m_vbByteSize = 0;
	UINT m_ibByteSize = 0;

	ComPtr<ID3D12Resource> m_vertexBufferUploader = nullptr;
	ComPtr<ID3D12Resource> m_indexBufferUploader = nullptr;

	// ������������������
	ComPtr<ID3D12DescriptorHeap> m_cbvHeap;

	// ������������Դ����
	std::unique_ptr<UploadBufferResource<ObjectConstants>> m_objCB = nullptr;

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

