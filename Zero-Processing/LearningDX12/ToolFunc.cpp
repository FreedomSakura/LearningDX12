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

// 创建上传堆 & 默认堆
ComPtr<ID3D12Resource> ToolFunc::CreateDefaultBuffer(ComPtr<ID3D12Device> d3dDevice, ComPtr<ID3D12GraphicsCommandList> cmdList, UINT64 byteSize, const void* initData, ComPtr<ID3D12Resource>& uploadBuffer) {
	// 创建上传堆，作用是：写入CPU内存数据，并传输给默认堆
	CD3DX12_HEAP_PROPERTIES properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	ThrowIfFailed(
		d3dDevice->CreateCommittedResource(
			&properties, // 创建类型为 上传堆 的堆
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(byteSize),	// 构造函数的奇怪写法！
			D3D12_RESOURCE_STATE_GENERIC_READ,			// 上传堆里的资源需要复制给默认堆，故是可读状态
			nullptr,
			IID_PPV_ARGS(&uploadBuffer)
		)
	);

	// 创建默认堆，作为上传堆的数据传输对象
	ComPtr<ID3D12Resource> defaultBuffer;
	CD3DX12_HEAP_PROPERTIES properties2 = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(
		d3dDevice->CreateCommittedResource(
			&properties2,
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
			D3D12_RESOURCE_STATE_COMMON,		// 默认堆为最终存储数据的地方，所以暂时初始化为普通状态
			nullptr,
			IID_PPV_ARGS(&defaultBuffer)
		)
	);

	// 将资源从COMMON状态转换到COPY_DEST状态（默认堆此时作为接收数据的目标）
	CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		defaultBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_COPY_DEST
	);

	cmdList->ResourceBarrier(
		1,
		&resourceBarrier
	);

	// 将数据从CPU内存拷贝到GPU缓存
	D3D12_SUBRESOURCE_DATA subResourceData;
	subResourceData.pData = initData;
	subResourceData.RowPitch = byteSize;
	subResourceData.SlicePitch = subResourceData.RowPitch;
	// 将数据从CPU内存拷贝至上传堆，再从上传堆拷贝至默认堆
	UpdateSubresources<1>(cmdList.Get(), defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);


	// 再将默认堆资源从COPY_DEST转为GENERIC_READ状态（只提供给着色器访问！）
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


// 编译着色器
ComPtr<ID3DBlob> ToolFunc::CompileShader(
	const std::wstring& fileName, 
	const D3D_SHADER_MACRO* defines, 
	const std::string& enteryPoint, 
	const std::string& target) 
{
	//若处于调试模式，则使用调试标志
	UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
	//用调试模式来编译着色器 | 指示编译器跳过优化阶段
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif // defined(DEBUG) || defined(_DEBUG)

	HRESULT hr = S_OK;

	ComPtr<ID3DBlob> byteCode = nullptr;
	ComPtr<ID3DBlob> errors;
	hr = D3DCompileFromFile(
		fileName.c_str(), //hlsl源文件名
		defines,	//高级选项，指定为空指针
		D3D_COMPILE_STANDARD_FILE_INCLUDE,	//高级选项，可以指定为空指针
		enteryPoint.c_str(),	//着色器的入口点函数名
		target.c_str(),		//指定所用着色器类型和版本的字符串
		compileFlags,	//指示对着色器断代码应当如何编译的标志
		0,	//高级选项
		&byteCode,	//编译好的字节码
		&errors);	//错误信息

	if (errors != nullptr)
	{
		OutputDebugStringA((char*)errors->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	return byteCode;
}