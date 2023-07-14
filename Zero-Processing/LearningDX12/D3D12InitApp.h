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

	// ������Ⱦ���ʵ�������壡��
	void BuildRenderItem();
	void DrawRenderItems(); // ���ƶ������
	void BuildFrameResources(); // ����֡��Դ

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
	std::unique_ptr<UploadBufferResource<PassConstants>> m_passCB = nullptr;

	ComPtr<ID3D12RootSignature> m_rootSignature; // ��ǩ��
	ComPtr<ID3D12PipelineState> m_PSO = nullptr; // ��ˮ��״̬����

	// Shader����
	// ���벼������������UnityLab��� : Position��
	std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayoutDesc;
	// Shader�����õ����ֽ���
	ComPtr<ID3DBlob> m_vsBytecode;
	ComPtr<ID3DBlob> m_psBytecode;

	// ����
	XMFLOAT4X4 m_world = MathHelper::Identity4x4();
	XMFLOAT4X4 m_proj = MathHelper::Identity4x4(); // ͶӰ����

	// ���������BuildGeometry�׶�ȷ���ĸ��������DrawCall���������ں���DrawCallʹ��
	std::map<std::string, SubmeshGeometry> m_mapDrawArgs;
	// ��Ⱦ��
	std::vector<std::unique_ptr<RenderItem>> m_allRItem;
	// ֡��Դ����
	int m_frameResourceCount = 3;
	std::vector<std::unique_ptr<FrameResources>> m_frameResourcesList;
	int m_currFrameResourceIndex = 0;
	FrameResources* m_currFrameResource = nullptr;
};

