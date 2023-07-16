#pragma once

#include "ShapesApp.h"

class LandAndWaves : public ShapesApp
{
public:
	LandAndWaves() {};
	~LandAndWaves() {};

public:
	virtual bool Init(HINSTANCE hInstance, int nShowCmd) override;
protected:
	virtual bool Draw() override;
	virtual void BuildGeometry() override;
	virtual void BuildRenderItem() override;
	virtual void BuildRootSignature() override;
	virtual void DrawRenderItems() override;

protected:
	UINT m_objCBByteSize = 0;
	UINT m_passCBByteSize = 0;
};