#pragma once

#include "ShapesApp.h"
#include "Waves.h"

class LandAndWaves : public ShapesApp
{
public:
	LandAndWaves() {};
	~LandAndWaves() {};

public:
	virtual bool Init(HINSTANCE hInstance, int nShowCmd) override;
	
protected:
	// ��Ⱦ����
	virtual bool Draw() override;
	virtual void BuildGeometry() override;
	virtual void BuildRenderItem() override;
	virtual void BuildRootSignature() override;
	virtual void DrawRenderItems() override;
	virtual void BuildFrameResources() override;

public:
	// ���⺯��
	// ����Wave�����������������㻺�����Ѿ�������m_waves���ˣ�
	void BuildLakeIndexBuffer();
	void UpdateWaves(const GameTime& gt);

protected:
	UINT m_objCBByteSize = 0;
	UINT m_passCBByteSize = 0;

	std::unique_ptr<Waves> m_waves = nullptr;
	RenderItem* m_waveRitem = nullptr;
};