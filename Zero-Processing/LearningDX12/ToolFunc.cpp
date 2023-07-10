#include "ToolFunc.h"

DxException::DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber) :
	ErrorCode(hr),
	FunctionName(functionName),
	Filename(filename),
	LineNumber(lineNumber)
{
}

std::wstring DxException::ToString()const
{
	// Get the string description of the error code.
	_com_error err(ErrorCode);
	std::wstring msg = err.ErrorMessage();

	return FunctionName + L" failed in " + Filename + L"; line " + std::to_wstring(LineNumber) + L"; error: " + msg;
}

// �����ϴ��� & Ĭ�϶�
ComPtr<ID3D12Resource> ToolFunc::CreateDefaultBuffer(ComPtr<ID3D12Device> d3dDevice, ComPtr<ID3D12GraphicsCommandList> cmdList, UINT64 byteSize, const void* initData, ComPtr<ID3D12Resource>& uploadBuffer) {
	// �����ϴ��ѣ������ǣ�д��CPU�ڴ����ݣ��������Ĭ�϶�
	CD3DX12_HEAP_PROPERTIES properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	ThrowIfFailed(
		d3dDevice->CreateCommittedResource(
			&properties, // ��������Ϊ �ϴ��� �Ķ�
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(byteSize),	// ���캯�������д����
			D3D12_RESOURCE_STATE_GENERIC_READ,			// �ϴ��������Դ��Ҫ���Ƹ�Ĭ�϶ѣ����ǿɶ�״̬
			nullptr,
			IID_PPV_ARGS(&uploadBuffer)
		)
	);

	// ����Ĭ�϶ѣ���Ϊ�ϴ��ѵ����ݴ������
	ComPtr<ID3D12Resource> defaultBuffer;
	CD3DX12_HEAP_PROPERTIES properties2 = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(
		d3dDevice->CreateCommittedResource(
			&properties2,
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
			D3D12_RESOURCE_STATE_COMMON,		// Ĭ�϶�Ϊ���մ洢���ݵĵط���������ʱ��ʼ��Ϊ��ͨ״̬
			nullptr,
			IID_PPV_ARGS(&defaultBuffer)
		)
	);

	// ����Դ��COMMON״̬ת����COPY_DEST״̬��Ĭ�϶Ѵ�ʱ��Ϊ�������ݵ�Ŀ�꣩
	CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		defaultBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_COPY_DEST
	);

	cmdList->ResourceBarrier(
		1,
		&resourceBarrier
	);

	// �����ݴ�CPU�ڴ濽����GPU����
	D3D12_SUBRESOURCE_DATA subResourceData;
	subResourceData.pData = initData;
	subResourceData.RowPitch = byteSize;
	subResourceData.SlicePitch = subResourceData.RowPitch;
	// �����ݴ�CPU�ڴ濽�����ϴ��ѣ��ٴ��ϴ��ѿ�����Ĭ�϶�
	UpdateSubresources<1>(cmdList.Get(), defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);


	// �ٽ�Ĭ�϶���Դ��COPY_DESTתΪGENERIC_READ״̬��ֻ�ṩ����ɫ�����ʣ���
	resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_GENERIC_READ
	);
	cmdList->ResourceBarrier(
		1,
		&resourceBarrier
	);

	return defaultBuffer;
}


// ������ɫ��
ComPtr<ID3DBlob> ToolFunc::CompileShader(
	const std::wstring& fileName, 
	const D3D_SHADER_MACRO* defines, 
	const std::string& enteryPoint, 
	const std::string& target) 
{
	//�����ڵ���ģʽ����ʹ�õ��Ա�־
	UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
	//�õ���ģʽ��������ɫ�� | ָʾ�����������Ż��׶�
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif // defined(DEBUG) || defined(_DEBUG)

	HRESULT hr = S_OK;

	ComPtr<ID3DBlob> byteCode = nullptr;
	ComPtr<ID3DBlob> errors;
	hr = D3DCompileFromFile(
		fileName.c_str(), //hlslԴ�ļ���
		defines,	//�߼�ѡ�ָ��Ϊ��ָ��
		D3D_COMPILE_STANDARD_FILE_INCLUDE,	//�߼�ѡ�����ָ��Ϊ��ָ��
		enteryPoint.c_str(),	//��ɫ������ڵ㺯����
		target.c_str(),		//ָ��������ɫ�����ͺͰ汾���ַ���
		compileFlags,	//ָʾ����ɫ���ϴ���Ӧ����α���ı�־
		0,	//�߼�ѡ��
		&byteCode,	//����õ��ֽ���
		&errors);	//������Ϣ

	if (errors != nullptr)
	{
		OutputDebugStringA((char*)errors->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	return byteCode;
}