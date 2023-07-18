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
	// 渲染管线
	virtual bool Draw() override;
	virtual void BuildGeometry() override;
	virtual void BuildRenderItem() override;
	virtual void BuildRootSignature() override;
	virtual void DrawRenderItems() override;
	virtual void BuildFrameResources() override;

public:
	// 特殊函数
	// 构建Wave的索引缓冲区（顶点缓冲区已经保存在m_waves中了）
	void BuildLakeIndexBuffer();
	void UpdateWaves(const GameTime& gt);

protected:
	UINT m_objCBByteSize = 0;
	UINT m_passCBByteSize = 0;

	std::unique_ptr<Waves> m_waves = nullptr;
	RenderItem* m_waveRitem = nullptr;
};