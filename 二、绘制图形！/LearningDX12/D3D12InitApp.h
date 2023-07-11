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
	virtual void Update() override;	// ���¾���ʵ�����Ǹ��� ������������
	virtual void OnResize() override;
	virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y) override;

	// ����ͼ����Ҫ��ӵĹ��߲���
	bool CreateCBV();
	void BuildRootSignature(); // ������ǩ��
	void BuildByteCodeAndInputLayout(); // ���벼�����������shader�ֽ���
	void BuildGeometry(); // ���������壨׼���ö�����������ݣ�
	void BuildPSO(); // ����PSO��PipeLineStateObject������ ��֮ǰ����ĸ��ֶ����󶨵���Ⱦ��ˮ���ϣ�

	// ��ȡ���㻺���������� & ����������������
	D3D12_VERTEX_BUFFER_VIEW GetVbv() const;
	D3D12_INDEX_BUFFER_VIEW GetIbv() const;

private:
	// CPU�ڴ�
	ComPtr<ID3DBlob> m_vertexBufferCPU;
	ComPtr<ID3DBlob> m_indexBufferCPU;
	// Ĭ�϶�
	ComPtr<ID3D12Resource> m_vertexBufferGPU;
	ComPtr<ID3D12Resource> m_indexBufferGPU;
	
	UINT m_vbByteSize = 0;
	UINT m_ibByteSize = 0;
	// �ϴ�����Դ
	ComPtr<ID3D12Resource> m_vertexBufferUploader = nullptr;
	ComPtr<ID3D12Resource> m_indexBufferUploader = nullptr;
	// ������������������
	ComPtr<ID3D12DescriptorHeap> m_cbvHeap;
	// ������������Դ����
	std::unique_ptr<UploadBufferResource<ObjectConstants>> m_objCB = nullptr;

	ComPtr<ID3D12RootSignature> m_rootSignature; // ��ǩ��
	ComPtr<ID3D12PipelineState> m_PSO = nullptr; // ��ˮ��״̬����

	// Shader����
	// ���벼������������UnityLab��� : Position��
	std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayoutDesc;
	// Shader�����õ����ֽ���
	ComPtr<ID3DBlob> m_vsBytecode;
	ComPtr<ID3DBlob> m_psBytecode;

	// ����
	XMFLOAT4X4 m_proj; // ͶӰ����
};

