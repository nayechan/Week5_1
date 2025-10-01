#include "pch.h"
#include "Render/Culling/Public/OcclusionCuller.h"
#include "Render/Renderer/Public/DeviceResources.h"
#include "Editor/Public/Camera.h"
#include <d3dcompiler.h>
#include <immintrin.h>

IMPLEMENT_SINGLETON_CLASS_BASE(UOcclusionCuller)

UOcclusionCuller::UOcclusionCuller() = default;
UOcclusionCuller::~UOcclusionCuller() = default;

void UOcclusionCuller::Initialize(UDeviceResources* InDeviceResources)
{
	DeviceResources = InDeviceResources;

	if (!GetDevice() || !DeviceResources)
	{
		UE_LOG("OcclusionCuller 초기화 실패: Device가 null입니다.");
		return;
	}

	// 컴퓨트 쉐이더 초기화
	InitializeComputeShaders();

	// HZB 리소스 초기화
	OnResize();
}

void UOcclusionCuller::Release()
{
	ReleaseResources();
	ReleaseComputeShaders();
}

void UOcclusionCuller::ReleaseResources()
{
	// Release HZB depth copy
	if (HZBDepthCopyUAV)
	{
		HZBDepthCopyUAV->Release();
		HZBDepthCopyUAV = nullptr;
	}
	if (HZBDepthCopySRV)
	{
		HZBDepthCopySRV->Release();
		HZBDepthCopySRV = nullptr;
	}
	if (HZBDepthCopy)
	{
		HZBDepthCopy->Release();
		HZBDepthCopy = nullptr;
	}

	// Release HZB textures and views
	for (int i = 0; i < 4; ++i)
	{
		if (HZBSRVs[i])
		{
			HZBSRVs[i]->Release();
			HZBSRVs[i] = nullptr;
		}
		if (HZBUAVs[i])
		{
			HZBUAVs[i]->Release();
			HZBUAVs[i] = nullptr;
		}
		if (HZBTextures[i])
		{
			HZBTextures[i]->Release();
			HZBTextures[i] = nullptr;
		}
	}

	// HZB 캐시 무효화
	bHZBCacheValid = false;
	CachedHZBLevel3.clear();
}

void UOcclusionCuller::InitializeComputeShaders()
{
	// HZB compute shader 컴파일
	CreateComputeShaderFromFile(L"Asset/Shader/HZBComputeShader.hlsl", &HZBComputeShader);

	if (HZBComputeShader)
	{
	}
	else
	{
		UE_LOG("HZB Compute Shader 컴파일 실패!");
	}

	// Depth copy compute shader 컴파일
	CreateComputeShaderFromFile(L"Asset/Shader/DepthCopyComputeShader.hlsl", &DepthCopyComputeShader);

	if (DepthCopyComputeShader)
	{
	}
	else
	{
		UE_LOG("DepthCopy Compute Shader 컴파일 실패!");
	}

	// HZB constant buffer 생성
	D3D11_BUFFER_DESC HZBBufferDescription = {};
	HZBBufferDescription.Usage = D3D11_USAGE_DYNAMIC;
	HZBBufferDescription.ByteWidth = sizeof(FHZBConstants);
	HZBBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	HZBBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	GetDevice()->CreateBuffer(&HZBBufferDescription, nullptr, &HZBConstantBuffer);
}

void UOcclusionCuller::ReleaseComputeShaders()
{
	if (HZBComputeShader)
	{
		HZBComputeShader->Release();
		HZBComputeShader = nullptr;
	}

	if (DepthCopyComputeShader)
	{
		DepthCopyComputeShader->Release();
		DepthCopyComputeShader = nullptr;
	}

	if (HZBConstantBuffer)
	{
		HZBConstantBuffer->Release();
		HZBConstantBuffer = nullptr;
	}
}

void UOcclusionCuller::OnResize()
{
	if (!GetDevice() || !DeviceResources)
	{
		UE_LOG("HZB 초기화 실패: Device 또는 DeviceResources가 null입니다.");
		return;
	}

	// 뷰포트 크기 가져오기
	const D3D11_VIEWPORT& ViewportInfo = DeviceResources->GetViewportInfo();
	uint32 width = static_cast<uint32>(ViewportInfo.Width);
	uint32 height = static_cast<uint32>(ViewportInfo.Height);

	// 깊이 버퍼 복사본 생성 (읽기/쓰기 충돌 방지용)
	D3D11_TEXTURE2D_DESC depthCopyDesc = {};
	depthCopyDesc.Width = width;
	depthCopyDesc.Height = height;
	depthCopyDesc.MipLevels = 1;
	depthCopyDesc.ArraySize = 1;
	depthCopyDesc.Format = DXGI_FORMAT_R32_FLOAT;
	depthCopyDesc.SampleDesc.Count = 1;
	depthCopyDesc.SampleDesc.Quality = 0;
	depthCopyDesc.Usage = D3D11_USAGE_DEFAULT;
	depthCopyDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	depthCopyDesc.CPUAccessFlags = 0;
	depthCopyDesc.MiscFlags = 0;

	HRESULT hr = GetDevice()->CreateTexture2D(&depthCopyDesc, nullptr, &HZBDepthCopy);
	if (FAILED(hr))
	{
		UE_LOG("HZB 깊이 버퍼 복사본 생성 실패");
		return;
	}

	// 깊이 버퍼 복사본 SRV 생성
	D3D11_SHADER_RESOURCE_VIEW_DESC depthCopySRVDesc = {};
	depthCopySRVDesc.Format = DXGI_FORMAT_R32_FLOAT;
	depthCopySRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	depthCopySRVDesc.Texture2D.MostDetailedMip = 0;
	depthCopySRVDesc.Texture2D.MipLevels = 1;

	hr = GetDevice()->CreateShaderResourceView(HZBDepthCopy, &depthCopySRVDesc, &HZBDepthCopySRV);
	if (FAILED(hr))
	{
		UE_LOG("HZB 깊이 버퍼 복사본 SRV 생성 실패");
	}

	// 깊이 버퍼 복사본 UAV 생성
	D3D11_UNORDERED_ACCESS_VIEW_DESC depthCopyUAVDesc = {};
	depthCopyUAVDesc.Format = DXGI_FORMAT_R32_FLOAT;
	depthCopyUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	depthCopyUAVDesc.Texture2D.MipSlice = 0;

	hr = GetDevice()->CreateUnorderedAccessView(HZBDepthCopy, &depthCopyUAVDesc, &HZBDepthCopyUAV);
	if (FAILED(hr))
	{
		UE_LOG("HZB 깊이 버퍼 복사본 UAV 생성 실패");
	}

	// HZB 텍스처 생성 (4개 mip level)
	for (int mipLevel = 0; mipLevel < 4; ++mipLevel)
	{
		uint32 mipWidth = width >> mipLevel;
		uint32 mipHeight = height >> mipLevel;

		// 최소 크기 1x1 보장
		mipWidth = mipWidth > 0 ? mipWidth : 1;
		mipHeight = mipHeight > 0 ? mipHeight : 1;

		// 텍스처 설명
		D3D11_TEXTURE2D_DESC textureDesc = {};
		textureDesc.Width = mipWidth;
		textureDesc.Height = mipHeight;
		textureDesc.MipLevels = 1;
		textureDesc.ArraySize = 1;
		textureDesc.Format = DXGI_FORMAT_R32_FLOAT;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Usage = D3D11_USAGE_DEFAULT;
		textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		textureDesc.CPUAccessFlags = 0;
		textureDesc.MiscFlags = 0;

		hr = GetDevice()->CreateTexture2D(&textureDesc, nullptr, &HZBTextures[mipLevel]);
		if (FAILED(hr))
		{
			UE_LOG("HZB 텍스처 %d 생성 실패", mipLevel);
			continue;
		}

		// UAV 생성
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_R32_FLOAT;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = 0;

		hr = GetDevice()->CreateUnorderedAccessView(HZBTextures[mipLevel], &uavDesc, &HZBUAVs[mipLevel]);
		if (FAILED(hr))
		{
			UE_LOG("HZB UAV %d 생성 실패", mipLevel);
		}

		// SRV 생성
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = 1;

		hr = GetDevice()->CreateShaderResourceView(HZBTextures[mipLevel], &srvDesc, &HZBSRVs[mipLevel]);
		if (FAILED(hr))
		{
			UE_LOG("HZB SRV %d 생성 실패", mipLevel);
		}
	}
}

void UOcclusionCuller::CreateComputeShaderFromFile(const wstring& InFilePath, ID3D11ComputeShader** OutComputeShader) const
{
	UINT CompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

	ID3DBlob* ShaderBlob = nullptr;
	ID3DBlob* ErrorBlob = nullptr;

	HRESULT hr = D3DCompileFromFile(
		InFilePath.c_str(),
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"CSMain",
		"cs_5_0",
		CompileFlags, 0,
		&ShaderBlob,
		&ErrorBlob
	);

	if (FAILED(hr))
	{
		if (ErrorBlob)
		{
			UE_LOG("Compute Shader 컴파일 에러: %s", (char*)ErrorBlob->GetBufferPointer());
			ErrorBlob->Release();
		}
		UE_LOG("Compute Shader 파일 컴파일 실패: %ls", InFilePath.c_str());
		return;
	}

	hr = GetDevice()->CreateComputeShader(
		ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(),
		nullptr,
		OutComputeShader
	);

	ShaderBlob->Release();

	if (FAILED(hr))
	{
		UE_LOG("Compute Shader 생성 실패");
	}
}

ID3D11Device* UOcclusionCuller::GetDevice() const
{
	return DeviceResources ? DeviceResources->GetDevice() : nullptr;
}

ID3D11DeviceContext* UOcclusionCuller::GetDeviceContext() const
{
	return DeviceResources ? DeviceResources->GetDeviceContext() : nullptr;
}

void UOcclusionCuller::DispatchComputeShader(ID3D11ComputeShader* InComputeShader, uint32 ThreadGroupsX, uint32 ThreadGroupsY, uint32 ThreadGroupsZ) const
{
	GetDeviceContext()->CSSetShader(InComputeShader, nullptr, 0);
	GetDeviceContext()->Dispatch(ThreadGroupsX, ThreadGroupsY, ThreadGroupsZ);
	GetDeviceContext()->CSSetShader(nullptr, nullptr, 0);
}

void UOcclusionCuller::GenerateHZB() const
{
	if (!HZBComputeShader || !DepthCopyComputeShader || !DeviceResources)
	{
		UE_LOG("HZB 생성 실패: 리소스가 준비되지 않았습니다.");
		return;
	}

	// 깊이 버퍼 SRV 가져오기
	ID3D11ShaderResourceView* depthSRV = DeviceResources->GetDepthStencilSRV();
	if (!depthSRV)
	{
		UE_LOG("HZB 생성 실패: 깊이 버퍼 SRV를 가져올 수 없습니다.");
		return;
	}

	// 1단계: 깊이 버퍼를 복사본으로 복사 (읽기/쓰기 충돌 방지)
	// 깊이 스텐실 뷰 언바인딩
	ID3D11RenderTargetView* rtv = DeviceResources->GetRenderTargetView();
	GetDeviceContext()->OMSetRenderTargets(1, &rtv, nullptr);

	// 뷰포트 크기 가져오기
	const D3D11_VIEWPORT& ViewportInfo = DeviceResources->GetViewportInfo();
	uint32 fullWidth = static_cast<uint32>(ViewportInfo.Width);
	uint32 fullHeight = static_cast<uint32>(ViewportInfo.Height);

	// 깊이 버퍼 복사 실행
	GetDeviceContext()->CSSetShaderResources(0, 1, &depthSRV);
	GetDeviceContext()->CSSetUnorderedAccessViews(0, 1, &HZBDepthCopyUAV, nullptr);

	uint32 copyThreadGroupsX = (fullWidth + 7) / 8;
	uint32 copyThreadGroupsY = (fullHeight + 7) / 8;
	DispatchComputeShader(DepthCopyComputeShader, copyThreadGroupsX, copyThreadGroupsY, 1);

	// 리소스 언바인딩
	ID3D11ShaderResourceView* nullSRV = nullptr;
	ID3D11UnorderedAccessView* nullUAV = nullptr;
	GetDeviceContext()->CSSetShaderResources(0, 1, &nullSRV);
	GetDeviceContext()->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);

	// 2단계: 복사된 깊이 버퍼에서 HZB 생성
	// 각 mip 레벨 생성
	for (uint32 mipLevel = 0; mipLevel < 4; ++mipLevel)
	{
		uint32 targetWidth = fullWidth >> mipLevel;
		uint32 targetHeight = fullHeight >> mipLevel;
		uint32 sourceWidth = (mipLevel == 0) ? fullWidth : (fullWidth >> (mipLevel - 1));
		uint32 sourceHeight = (mipLevel == 0) ? fullHeight : (fullHeight >> (mipLevel - 1));

		// 최소 크기 1x1 보장
		targetWidth = targetWidth > 0 ? targetWidth : 1;
		targetHeight = targetHeight > 0 ? targetHeight : 1;
		sourceWidth = sourceWidth > 0 ? sourceWidth : 1;
		sourceHeight = sourceHeight > 0 ? sourceHeight : 1;

		// 상수 버퍼 업데이트
		FHZBConstants constants = {};
		constants.SourceDimensions = FVector2(static_cast<float>(sourceWidth), static_cast<float>(sourceHeight));
		constants.TargetDimensions = FVector2(static_cast<float>(targetWidth), static_cast<float>(targetHeight));
		constants.MipLevel = mipLevel;
		constants.MinDepthClamp = 0.5f;  // Prevent near object culling issues

		D3D11_MAPPED_SUBRESOURCE mappedResource = {};
		GetDeviceContext()->Map(HZBConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		memcpy(mappedResource.pData, &constants, sizeof(FHZBConstants));
		GetDeviceContext()->Unmap(HZBConstantBuffer, 0);

		// 리소스 바인딩
		GetDeviceContext()->CSSetConstantBuffers(0, 1, &HZBConstantBuffer);

		if (mipLevel == 0)
		{
			// Level 0: 복사된 깊이 버퍼에서 복사
			GetDeviceContext()->CSSetShaderResources(0, 1, &HZBDepthCopySRV);
		}
		else
		{
			// Level 1+: 이전 mip level에서 다운샘플링
			ID3D11ShaderResourceView* prevSRV = HZBSRVs[mipLevel - 1];
			GetDeviceContext()->CSSetShaderResources(0, 1, &prevSRV);
		}

		GetDeviceContext()->CSSetUnorderedAccessViews(0, 1, &HZBUAVs[mipLevel], nullptr);

		// Dispatch
		uint32 threadGroupsX = (targetWidth + 7) / 8;
		uint32 threadGroupsY = (targetHeight + 7) / 8;
		DispatchComputeShader(HZBComputeShader, threadGroupsX, threadGroupsY, 1);

		// UAV 언바인딩
		ID3D11UnorderedAccessView* nullUAV = nullptr;
		GetDeviceContext()->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);

		// GPU 동기화 - UAV 쓰기 완료 대기
		if (mipLevel < 3)  // 마지막 레벨이 아닌 경우에만
		{
			// UAV 배리어를 통해 이전 레벨의 쓰기가 완료되었음을 보장
			ID3D11UnorderedAccessView* barrierUAV = HZBUAVs[mipLevel];
			GetDeviceContext()->CSSetUnorderedAccessViews(0, 1, &barrierUAV, nullptr);
			GetDeviceContext()->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);
		}
	}

	// SRV 언바인딩
	ID3D11ShaderResourceView* nullSRV2 = nullptr;
	GetDeviceContext()->CSSetShaderResources(0, 1, &nullSRV2);

	// 깊이 스텐실 뷰 다시 바인딩
	ID3D11RenderTargetView* rtv2 = DeviceResources->GetRenderTargetView();
	GetDeviceContext()->OMSetRenderTargets(1, &rtv2, DeviceResources->GetDepthStencilView());
}

// 나머지 함수들 include
#include "OcclusionCullerImpl.inl"
