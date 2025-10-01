#pragma once
#include "Core/Public/Object.h"
#include <d3d11.h>

class UDeviceResources;
class UCamera;

/**
 * @brief HZB (Hierarchical Z-Buffer) 기반 Occlusion Culling 관리 클래스
 *
 * GPU 컴퓨트 쉐이더를 사용하여 깊이 피라미드를 생성하고,
 * CPU 캐싱을 통해 빠른 occlusion 테스트를 수행합니다.
 */
UCLASS()
class UOcclusionCuller : public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(UOcclusionCuller, UObject)

public:
	/**
	 * @brief HZB 리소스를 초기화합니다.
	 * @param InDeviceResources Direct3D 디바이스 리소스
	 */
	void Initialize(UDeviceResources* InDeviceResources);

	/**
	 * @brief HZB 리소스를 해제합니다 (컴퓨트 쉐이더는 유지).
	 */
	void ReleaseResources();

	/**
	 * @brief 모든 HZB 리소스를 완전히 해제합니다.
	 */
	void Release();

	/**
	 * @brief 화면 크기가 변경될 때 HZB 리소스를 재생성합니다.
	 */
	void OnResize();

	/**
	 * @brief 깊이 버퍼에서 HZB를 생성합니다.
	 */
	void GenerateHZB() const;

	/**
	 * @brief HZB mipLevel 3를 CPU 메모리로 캐시합니다 (빠른 occlusion 테스트용).
	 */
	void CacheHZBForOcclusion() const;

	/**
	 * @brief 캐시된 HZB 데이터를 해제합니다.
	 */
	void ReleaseCachedHZB() const;

	/**
	 * @brief 월드 AABB가 가려져 있는지 테스트합니다.
	 * @param WorldMin AABB 최소 좌표
	 * @param WorldMax AABB 최대 좌표
	 * @param Camera 현재 카메라
	 * @return 완전히 가려져 있으면 true
	 */
	bool IsOccluded(const FVector& WorldMin, const FVector& WorldMax, const UCamera* Camera) const;

	/**
	 * @brief 월드 AABB가 가려져 있는지 테스트합니다 (확장 팩터 적용).
	 * @param WorldMin AABB 최소 좌표
	 * @param WorldMax AABB 최대 좌표
	 * @param Camera 현재 카메라
	 * @param ExpansionFactor AABB 확장 비율
	 * @return 완전히 가려져 있으면 true
	 */
	bool IsOccluded(const FVector& WorldMin, const FVector& WorldMax, const UCamera* Camera, float ExpansionFactor) const;

	/**
	 * @brief 월드 AABB를 스크린 공간으로 투영합니다.
	 */
	bool ProjectAABBToScreen(const FVector& WorldMin, const FVector& WorldMax, const UCamera* Camera,
		float& OutScreenX, float& OutScreenY, float& OutWidth, float& OutHeight, float& OutMinDepth) const;

	// HZB 샘플링 함수들
	float SampleHZBDepth(uint32 MipLevel, uint32 X, uint32 Y) const;
	float SampleHZBRegionMaxDepth(uint32 MipLevel, uint32 StartX, uint32 StartY, uint32 Width, uint32 Height) const;
	float SampleHZBRect(float ScreenX, float ScreenY, float Width, float Height) const;

	// 컴퓨트 쉐이더 헬퍼
	void DispatchComputeShader(ID3D11ComputeShader* InComputeShader, uint32 ThreadGroupsX, uint32 ThreadGroupsY, uint32 ThreadGroupsZ) const;

	// Getters
	bool IsHZBCacheValid() const { return bHZBCacheValid; }
	ID3D11Device* GetDevice() const;
	ID3D11DeviceContext* GetDeviceContext() const;

private:
	/**
	 * @brief 컴퓨트 쉐이더를 초기화합니다.
	 */
	void InitializeComputeShaders();

	/**
	 * @brief 컴퓨트 쉐이더를 해제합니다.
	 */
	void ReleaseComputeShaders();

	/**
	 * @brief 파일에서 컴퓨트 쉐이더를 생성합니다.
	 */
	void CreateComputeShaderFromFile(const wstring& InFilePath, ID3D11ComputeShader** OutComputeShader) const;

	// Direct3D 리소스
	UDeviceResources* DeviceResources = nullptr;

	// HZB Compute Shaders
	ID3D11ComputeShader* HZBComputeShader = nullptr;
	ID3D11ComputeShader* DepthCopyComputeShader = nullptr;
	ID3D11Buffer* HZBConstantBuffer = nullptr;

	// HZB 텍스처 (4개 mip levels)
	ID3D11Texture2D* HZBTextures[4] = { nullptr };
	ID3D11UnorderedAccessView* HZBUAVs[4] = { nullptr };
	ID3D11ShaderResourceView* HZBSRVs[4] = { nullptr };

	// Depth buffer copy (읽기/쓰기 충돌 방지용)
	ID3D11Texture2D* HZBDepthCopy = nullptr;
	ID3D11ShaderResourceView* HZBDepthCopySRV = nullptr;
	ID3D11UnorderedAccessView* HZBDepthCopyUAV = nullptr;

	// HZB mipLevel 3 캐시 (CPU 메모리)
	mutable TArray<float> CachedHZBLevel3;
	mutable uint32 CachedHZBWidth = 0;
	mutable uint32 CachedHZBHeight = 0;
	mutable bool bHZBCacheValid = false;

	// 스테이징 텍스처 캐시 (재사용)
	mutable ID3D11Texture2D* CachedStagingTexture = nullptr;
	mutable uint32 CachedStagingWidth = 0;
	mutable uint32 CachedStagingHeight = 0;
};