#include "FrameResources.h"

FrameResources::FrameResources(ID3D12Device* device, UINT passCount, UINT objCount)
{
	ThrowIfFailed(device->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&m_cmdAllocator)
		)
	);

	m_objCB = std::make_unique<UploadBufferResource<ObjectConstants>>(device, objCount, true);
	m_passCB = std::make_unique<UploadBufferResource<PassConstants>>(device, passCount, true);
}

FrameResources::~FrameResources() {}


