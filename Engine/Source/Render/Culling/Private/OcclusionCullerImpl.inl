// OcclusionCuller 추가 구현 함수들

#include "pch.h"

// HorizontalMax 헬퍼 함수
static inline float HorizontalMax(__m128 v)
{
	__m128 temp = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 3, 0, 1));
	v = _mm_max_ps(v, temp);
	temp = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 0, 3, 2));
	v = _mm_max_ps(v, temp);
	return _mm_cvtss_f32(v);
}

float UOcclusionCuller::SampleHZBRect(float screenX, float screenY, float width, float height) const
{
	// 캐시가 유효하지 않으면 실패
	if (!bHZBCacheValid || CachedHZBLevel3.empty() || CachedHZBWidth == 0 || CachedHZBHeight == 0)
	{
		return -1.0f;
	}

	if (width <= 0.0f || height <= 0.0f)
	{
		return -1.0f;
	}

	// mipLevel 3 (1/8 해상도) 좌표로 변환
	float mipScale = 1.0f / 8.0f;  // mipLevel 3 = 1/8
	uint32 mipX = static_cast<uint32>(screenX * mipScale);
	uint32 mipY = static_cast<uint32>(screenY * mipScale);
	uint32 mipWidth = std::max(1u, static_cast<uint32>(width * mipScale));
	uint32 mipHeight = std::max(1u, static_cast<uint32>(height * mipScale));

	// 경계 체크
	if (mipX >= CachedHZBWidth || mipY >= CachedHZBHeight)
	{
		return -1.0f;
	}

	// 영역 클램핑
	mipWidth = std::min(mipWidth, CachedHZBWidth - mipX);
	mipHeight = std::min(mipHeight, CachedHZBHeight - mipY);

	// 단일 픽셀 빠른 경로
	if (mipWidth == 1 && mipHeight == 1)
	{
		uint32 index = mipY * CachedHZBWidth + mipX;
		return CachedHZBLevel3[index];
	}

	// SIMD로 최적화된 최대값 찾기 (4개씩 동시 처리)
	float maxDepth = 0.0f;
	__m128 maxVec = _mm_setzero_ps();

	// 경계 검사 사전 계산으로 루프 내 분기 제거
	uint32 endY = mipY + mipHeight;
	uint32 endX = mipX + mipWidth;
	uint32 simdEndX = endX & ~3u; // 4의 배수로 정렬

	for (uint32 y = mipY; y < endY; ++y)
	{
		uint32 rowStart = y * CachedHZBWidth;
		uint32 x = mipX;

		// 다음 행 프리페칭 (캐시 미스 감소)
		if (y + 1 < endY)
		{
			uint32 nextRowStart = (y + 1) * CachedHZBWidth + mipX;
			_mm_prefetch(reinterpret_cast<const char*>(&CachedHZBLevel3[nextRowStart]), _MM_HINT_T0);
		}

		// 4개씩 배치로 처리 (경계 검사 제거)
		for (; x < simdEndX; x += 4)
		{
			uint32 index = rowStart + x;
			__m128 depthVec = _mm_loadu_ps(&CachedHZBLevel3[index]);
			maxVec = _mm_max_ps(maxVec, depthVec);
		}

		// 나머지 처리
		for (; x < endX; ++x)
		{
			uint32 index = rowStart + x;
			maxDepth = std::max(maxDepth, CachedHZBLevel3[index]);
		}
	}

	// SIMD 결과에서 수평 최대값 추출
	float simdMax = HorizontalMax(maxVec);
	maxDepth = std::max(maxDepth, simdMax);

	return maxDepth;
}

bool UOcclusionCuller::ProjectAABBToScreen(const FVector& worldMin, const FVector& worldMax, const UCamera* camera,
	float& outScreenX, float& outScreenY, float& outWidth, float& outHeight, float& outMinDepth) const
{
	if (!camera || !DeviceResources)
	{
		return false;
	}

	const FViewProjConstants& viewProj = camera->GetFViewProjConstants();
	const D3D11_VIEWPORT& viewport = DeviceResources->GetViewportInfo();

	// AABB의 8개 꼭짓점 생성
	FVector corners[8] = {
		FVector(worldMin.X, worldMin.Y, worldMin.Z),
		FVector(worldMax.X, worldMin.Y, worldMin.Z),
		FVector(worldMin.X, worldMax.Y, worldMin.Z),
		FVector(worldMax.X, worldMax.Y, worldMin.Z),
		FVector(worldMin.X, worldMin.Y, worldMax.Z),
		FVector(worldMax.X, worldMin.Y, worldMax.Z),
		FVector(worldMin.X, worldMax.Y, worldMax.Z),
		FVector(worldMax.X, worldMax.Y, worldMax.Z)
	};

	float minScreenX = FLT_MAX, maxScreenX = -FLT_MAX;
	float minScreenY = FLT_MAX, maxScreenY = -FLT_MAX;
	float minDepth = FLT_MAX;
	bool hasValidProjection = false;
	int validCorners = 0;
	int behindCamera = 0;

	// 각 꼭짓점을 스크린 공간으로 투영
	for (int i = 0; i < 8; i++)
	{
		FVector4 worldPos(corners[i].X, corners[i].Y, corners[i].Z, 1.0f);

		// ViewProjection 변환
		FVector4 clipPos = worldPos * viewProj.View * viewProj.Projection;

		// 클리핑 공간에서 W 체크
		if (clipPos.W <= 0.001f)  // 카메라 뒤에 있는 점
		{
			behindCamera++;
			continue;
		}

		// 동차 나누기 (Perspective divide)
		float invW = 1.0f / clipPos.W;
		clipPos.X *= invW;
		clipPos.Y *= invW;
		clipPos.Z *= invW;

		// NDC 범위 체크 (-1 ~ 1)
		if (clipPos.X < -1.5f || clipPos.X > 1.5f ||
			clipPos.Y < -1.5f || clipPos.Y > 1.5f ||
			clipPos.Z < -0.1f || clipPos.Z > 1.1f)
		{
			// 약간의 여유를 두고 범위를 벗어나는 점은 클램핑
			clipPos.X = std::max(-1.5f, std::min(1.5f, clipPos.X));
			clipPos.Y = std::max(-1.5f, std::min(1.5f, clipPos.Y));
			clipPos.Z = std::max(0.0f, std::min(1.0f, clipPos.Z));
		}

		// NDC(-1~1)를 스크린 좌표(0~width, 0~height)로 변환
		float screenX = (clipPos.X + 1.0f) * 0.5f * viewport.Width;
		float screenY = (1.0f - clipPos.Y) * 0.5f * viewport.Height;  // Y축 뒤집기
		float depth = clipPos.Z;  // 0~1 범위의 깊이

		minScreenX = std::min(minScreenX, screenX);
		maxScreenX = std::max(maxScreenX, screenX);
		minScreenY = std::min(minScreenY, screenY);
		maxScreenY = std::max(maxScreenY, screenY);
		minDepth = std::min(minDepth, depth);

		hasValidProjection = true;
		validCorners++;
	}

	// 모든 점이 카메라 뒤에 있거나, 유효한 점이 너무 적으면 실패
	if (!hasValidProjection || validCorners < 2)
	{
		return false;
	}

	// 결과 계산 (적당한 보수적 확장)
	float marginX = std::max(2.0f, (maxScreenX - minScreenX) * 0.05f);  // 5% 또는 최소 2픽셀
	float marginY = std::max(2.0f, (maxScreenY - minScreenY) * 0.05f);

	outScreenX = std::max(0.0f, minScreenX - marginX);
	outScreenY = std::max(0.0f, minScreenY - marginY);
	outWidth = std::min(viewport.Width, maxScreenX + marginX) - outScreenX;
	outHeight = std::min(viewport.Height, maxScreenY + marginY) - outScreenY;
	outMinDepth = std::max(0.0f, std::min(1.0f, minDepth));

	// 유효한 영역인지 확인
	return outWidth > 0.0f && outHeight > 0.0f;
}

bool UOcclusionCuller::IsOccluded(const FVector& worldMin, const FVector& worldMax, const UCamera* camera) const
{
	return IsOccluded(worldMin, worldMax, camera, 3.0f);  // 기본 3배 확장
}

bool UOcclusionCuller::IsOccluded(const FVector& worldMin, const FVector& worldMax, const UCamera* camera, float expansionFactor) const
{
	// 카메라와 AABB 사이의 최단 거리 계산
	FVector center = (worldMin + worldMax) * 0.5f;
	FVector cameraPos = camera->GetLocation();

	// Clamp 함수
	auto clamp = [](float value, float min, float max) -> float {
		if (value < min) return min;
		if (value > max) return max;
		return value;
	};

	// AABB와 점(카메라) 사이의 최단 거리 계산
	FVector closestPoint;
	closestPoint.X = clamp(cameraPos.X, worldMin.X, worldMax.X);
	closestPoint.Y = clamp(cameraPos.Y, worldMin.Y, worldMax.Y);
	closestPoint.Z = clamp(cameraPos.Z, worldMin.Z, worldMax.Z);

	float distanceToAABB = (cameraPos - closestPoint).Length();

	// 카메라가 AABB 내부에 있거나 10.0f 거리 이내의 물체는 컬링하지 않음
	if (distanceToAABB < 15.0f)
	{
		return false;
	}

	// 보수적 컬링을 위해 AABB를 확대
	FVector size = (worldMax - worldMin) * expansionFactor;
	FVector conservativeMin = center - size * 0.5f;
	FVector conservativeMax = center + size * 0.5f;

	float screenX, screenY, width, height, minDepth;

	// 확대된 AABB를 스크린 공간으로 투영
	if (!ProjectAABBToScreen(conservativeMin, conservativeMax, camera, screenX, screenY, width, height, minDepth))
	{
		return true;  // 투영 실패 = 화면 밖
	}

	// HZB에서 해당 영역의 최대 깊이 샘플링
	float hzbMaxDepth = SampleHZBRect(screenX, screenY, width, height);

	if (hzbMaxDepth < 0.0f)
	{
		return false;  // HZB 샘플링 실패 = 안전하게 보이는 것으로 처리
	}

	// Occlusion 테스트
	float depthBias = 0.0005f;
	bool isOccluded = (hzbMaxDepth + depthBias) < minDepth;

	return isOccluded;
}

void UOcclusionCuller::CacheHZBForOcclusion() const
{
	if (!DeviceResources || !HZBTextures[3])
	{
		bHZBCacheValid = false;
		return;
	}

	const D3D11_VIEWPORT& viewport = DeviceResources->GetViewportInfo();
	CachedHZBWidth = static_cast<uint32>(viewport.Width) / 8;  // mipLevel 3 = 1/8 해상도
	CachedHZBHeight = static_cast<uint32>(viewport.Height) / 8;

	// 최소 크기 보장
	CachedHZBWidth = std::max(1u, CachedHZBWidth);
	CachedHZBHeight = std::max(1u, CachedHZBHeight);

	// 버퍼 크기 조정
	CachedHZBLevel3.resize(CachedHZBWidth * CachedHZBHeight);

	// 스테이징 텍스처가 있고 크기가 같으면 재사용
	ID3D11Texture2D* stagingTexture = CachedStagingTexture;
	if (stagingTexture &&
		CachedStagingWidth == CachedHZBWidth &&
		CachedStagingHeight == CachedHZBHeight)
	{
		// 재사용 가능
	}
	else
	{
		// 기존 스테이징 텍스처 해제
		if (CachedStagingTexture)
		{
			CachedStagingTexture->Release();
			CachedStagingTexture = nullptr;
		}

		// 새 스테이징 텍스처 생성
		D3D11_TEXTURE2D_DESC stagingDesc = {};
		stagingDesc.Width = CachedHZBWidth;
		stagingDesc.Height = CachedHZBHeight;
		stagingDesc.MipLevels = 1;
		stagingDesc.ArraySize = 1;
		stagingDesc.Format = DXGI_FORMAT_R32_FLOAT;
		stagingDesc.SampleDesc.Count = 1;
		stagingDesc.SampleDesc.Quality = 0;
		stagingDesc.Usage = D3D11_USAGE_STAGING;
		stagingDesc.BindFlags = 0;
		stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		stagingDesc.MiscFlags = 0;

		HRESULT hr = GetDevice()->CreateTexture2D(&stagingDesc, nullptr, &stagingTexture);
		if (FAILED(hr))
		{
			bHZBCacheValid = false;
			return;
		}

		CachedStagingTexture = stagingTexture;
		CachedStagingWidth = CachedHZBWidth;
		CachedStagingHeight = CachedHZBHeight;
	}

	GetDeviceContext()->CopyResource(stagingTexture, HZBTextures[3]);

	D3D11_MAPPED_SUBRESOURCE mappedResource = {};
	HRESULT hr = GetDeviceContext()->Map(stagingTexture, 0, D3D11_MAP_READ, 0, &mappedResource);
	if (SUCCEEDED(hr))
	{
		const float* sourceData = static_cast<const float*>(mappedResource.pData);
		uint32 sourceRowPitch = mappedResource.RowPitch / sizeof(float);

		if (sourceRowPitch == CachedHZBWidth)
		{
			// 연속된 메모리이면 한 번에 복사
			memcpy(CachedHZBLevel3.data(), sourceData, CachedHZBWidth * CachedHZBHeight * sizeof(float));
		}
		else
		{
			// 행 단위로 복사
			for (uint32 y = 0; y < CachedHZBHeight; ++y)
			{
				const float* sourceRow = sourceData + y * sourceRowPitch;
				float* destRow = CachedHZBLevel3.data() + y * CachedHZBWidth;
				memcpy(destRow, sourceRow, CachedHZBWidth * sizeof(float));
			}
		}

		GetDeviceContext()->Unmap(stagingTexture, 0);
		bHZBCacheValid = true;
	}
	else
	{
		bHZBCacheValid = false;
	}
}

void UOcclusionCuller::ReleaseCachedHZB() const
{
	CachedHZBLevel3.clear();
	bHZBCacheValid = false;
	CachedHZBWidth = 0;
	CachedHZBHeight = 0;
}

float UOcclusionCuller::SampleHZBDepth(uint32 mipLevel, uint32 x, uint32 y) const
{
	if (mipLevel >= 4 || !HZBTextures[mipLevel])
	{
		return -1.0f;
	}

	// 스테이징 텍스처를 통한 샘플링 (기본 구현)
	// 실제로는 CacheHZBForOcclusion을 사용하는 것이 더 효율적
	return -1.0f;  // 기본 구현은 캐시 사용 권장
}

float UOcclusionCuller::SampleHZBRegionMaxDepth(uint32 mipLevel, uint32 startX, uint32 startY, uint32 width, uint32 height) const
{
	if (mipLevel >= 4 || !HZBTextures[mipLevel] || width == 0 || height == 0)
	{
		return -1.0f;
	}

	// 스테이징 텍스처를 통한 샘플링 (기본 구현)
	// 실제로는 CacheHZBForOcclusion을 사용하는 것이 더 효율적
	return -1.0f;  // 기본 구현은 캐시 사용 권장
}
