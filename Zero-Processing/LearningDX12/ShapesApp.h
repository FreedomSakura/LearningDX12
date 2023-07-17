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
	virtual void Update() override;	// ���¾���ʵ�����Ǹ��� ������������
	virtual void OnResize() override;
	virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y) override;

	// ����ͼ����Ҫ��ӵĹ��߲���
	bool CreateCBV();
	virtual void BuildRootSignature(); // ������ǩ��
	void BuildByteCodeAndInputLayout(); // ���벼�����������shader�ֽ���
	virtual void BuildGeometry(); // ���������壨׼���ö�����������ݣ�
	void BuildPSO(); // ����PSO��PipeLineStateObject������ ��֮ǰ����ĸ��ֶ����󶨵���Ⱦ��ˮ���ϣ�

	// ������Ⱦ���ʵ�������壡��
	virtual void BuildRenderItem();
	virtual void DrawRenderItems(); // ���ƶ������
	virtual void BuildFrameResources(); // ����֡��Դ

	float GetHillsHeight(float x, float z);

protected:
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
	std::vector<std::unique_ptr<RenderItem>> m_allRItems;
	// ֡��Դ����
	int m_frameResourceCount = 3;
	std::vector<std::unique_ptr<FrameResources>> m_frameResourcesList;
	int m_currFrameResourceIndex = 0;
	FrameResources* m_currFrameResource = nullptr;

	// ��̬���㻺����
	std::map<std::string, std::unique_ptr<MeshGeometry>> m_geometries;
};

