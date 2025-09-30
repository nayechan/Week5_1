#pragma once
#include "DeviceResources.h"
#include "Core/Public/Object.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Editor/Public/EditorPrimitive.h"

class UPipeline;
class UDeviceResources;
class UPrimitiveComponent;
class UStaticMeshComponent;
class UTextBillboardComponent;
class AActor;
class AGizmo;
class UEditor;
class UFontRenderer;
class FViewport;
class UCamera;

/**
 * @brief Rendering Pipeline 전반을 처리하는 클래스
 *
 * Direct3D 11 장치(Device)와 장치 컨텍스트(Device Context) 및 스왑 체인(Swap Chain)을 관리하기 위한 포인터들
 * @param Device GPU와 통신하기 위한 Direct3D 장치
 * @param DeviceContext GPU 명령 실행을 담당하는 컨텍스트
 * @param SwapChain 프레임 버퍼를 교체하는 데 사용되는 스왑 체인
 *
 * // 렌더링에 필요한 리소스 및 상태를 관리하기 위한 변수들
 * @param FrameBuffer 화면 출력용 텍스처
 * @param FrameBufferRTV 텍스처를 렌더 타겟으로 사용하는 뷰
 * @param RasterizerState 래스터라이저 상태(컬링, 채우기 모드 등 정의)
 * @param ConstantBuffer 쉐이더에 데이터를 전달하기 위한 상수 버퍼
 *
 * @param ClearColor 화면을 초기화(clear)할 때 사용할 색상 (RGBA)
 * @param ViewportInfo 렌더링 영역을 정의하는 뷰포트 정보
 *
 * @param DefaultVertexShader
 * @param DefaultPixelShader
 * @param DefaultInputLayout
 * @param Stride
 *
 * @param vertexBufferSphere
 * @param numVerticesSphere
 */
UCLASS()
class URenderer :
	public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(URenderer, UObject)

public:
	void Init(HWND InWindowHandle);
	void Release();

	// Initialize
	void CreateRasterizerState();
	void CreateDepthStencilState();
	void CreateDefaultShader();
	void CreateTextureShader();
	void CreateComputeShader();
	void CreateConstantBuffer();

	// Release
	void ReleaseConstantBuffer();
	void ReleaseDefaultShader();
	void ReleaseComputeShader();
	void ReleaseDepthStencilState();
	void ReleaseRasterizerState();

	// Render
	void Update();
	void RenderBegin() const;
	void RenderLevel(UCamera* InCurrentCamera);
	void RenderEnd() const;
	void RenderStaticMesh(UStaticMeshComponent* InMeshComp, ID3D11RasterizerState* InRasterizerState);
	void RenderBillboard(UTextBillboardComponent* InBillBoardComp, UCamera* InCurrentCamera);
	void RenderPrimitiveDefault(UPrimitiveComponent* InPrimitiveComp, ID3D11RasterizerState* InRasterizerState);
	void RenderPrimitive(const FEditorPrimitive& InPrimitive, const FRenderState& InRenderState);
	void RenderPrimitiveIndexed(const FEditorPrimitive& InPrimitive, const FRenderState& InRenderState,
	                            bool bInUseBaseConstantBuffer, uint32 InStride, uint32 InIndexBufferStride);

	void OnResize(uint32 Inwidth = 0, uint32 InHeight = 0);

	// Create function
	void CreateVertexShaderAndInputLayout(const wstring& InFilePath,
									  const TArray<D3D11_INPUT_ELEMENT_DESC>& InInputLayoutDescriptions,
									  ID3D11VertexShader** OutVertexShader, ID3D11InputLayout** OutInputLayout);
	ID3D11Buffer* CreateVertexBuffer(FNormalVertex* InVertices, uint32 InByteWidth) const;
	ID3D11Buffer* CreateVertexBuffer(FVector* InVertices, uint32 InByteWidth, bool bCpuAccess) const;
	ID3D11Buffer* CreateIndexBuffer(const void* InIndices, uint32 InByteWidth) const;
	void CreatePixelShader(const wstring& InFilePath, ID3D11PixelShader** InPixelShader) const;
	void CreateComputeShaderFromFile(const wstring& InFilePath, ID3D11ComputeShader** OutComputeShader) const;

	// Compute Shader functions
	void DispatchComputeShader(ID3D11ComputeShader* InComputeShader, uint32 ThreadGroupsX, uint32 ThreadGroupsY, uint32 ThreadGroupsZ) const;
	ID3D11Buffer* CreateStructuredBuffer(uint32 InElementSize, uint32 InElementCount, const void* InInitialData = nullptr) const;
	ID3D11ShaderResourceView* CreateStructuredBufferSRV(ID3D11Buffer* InBuffer, uint32 InElementCount) const;
	ID3D11UnorderedAccessView* CreateStructuredBufferUAV(ID3D11Buffer* InBuffer, uint32 InElementCount) const;

	// HZB functions
	void InitializeHZB();
	void ReleaseHZBResources();
	void GenerateHZB() const;
	// HZB 샘플링 함수들 (일반적인 용도)
	float SampleHZBDepth(uint32 mipLevel, uint32 x, uint32 y) const;
	float SampleHZBRegionMaxDepth(uint32 mipLevel, uint32 startX, uint32 startY, uint32 width, uint32 height) const;

	// Occlusion Culling 전용 HZB 샘플링 함수
	float SampleHZBRect(float screenX, float screenY, float width, float height) const;

	// HZB 캐시 관리
	void CacheHZBForOcclusion() const;
	void ReleaseCachedHZB() const;

	// Occlusion Culling 유틸리티 함수들
	bool ProjectAABBToScreen(const FVector& worldMin, const FVector& worldMax, const UCamera* camera,
		float& outScreenX, float& outScreenY, float& outWidth, float& outHeight, float& outMinDepth) const;
	bool IsOccluded(const FVector& worldMin, const FVector& worldMax, const UCamera* camera) const;
	bool IsOccluded(const FVector& worldMin, const FVector& worldMax, const UCamera* camera, float expansionFactor) const;


	// HZB mipLevel 3 캐시 (CPU 메모리)
	mutable TArray<float> CachedHZBLevel3;
	mutable uint32 CachedHZBWidth = 0;
	mutable uint32 CachedHZBHeight = 0;
	mutable bool bHZBCacheValid = false;

	// 스테이징 텍스처 캐시 (재사용을 위한)
	mutable ID3D11Texture2D* CachedStagingTexture = nullptr;
	mutable uint32 CachedStagingWidth = 0;
	mutable uint32 CachedStagingHeight = 0;

	// 카메라 움직임 추적
	mutable FVector LastCameraPosition = FVector::ZeroVector();
	mutable FVector LastCameraRotation = FVector::ZeroVector();
	mutable bool bCameraMovedSignificantly = false;


	// Test functions
	void TestComputeShaderExecution() const;

	bool UpdateVertexBuffer(ID3D11Buffer* InVertexBuffer, const TArray<FVector>& InVertices) const;
	void UpdateConstant(const UPrimitiveComponent* InPrimitive) const;
	void UpdateConstant(const FVector& InPosition, const FVector& InRotation, const FVector& InScale) const;
	void UpdateConstant(const FViewProjConstants& InViewProjConstants) const;
	void UpdateConstant(const FMatrix& InMatrix) const;
	void UpdateConstant(const FVector4& InColor) const;
	void UpdateConstant(const FMaterialConstants& InMaterial) const;

	static void ReleaseVertexBuffer(ID3D11Buffer* InVertexBuffer);
	static void ReleaseIndexBuffer(ID3D11Buffer* InIndexBuffer);

	// Helper function
	static D3D11_CULL_MODE ToD3D11(ECullMode InCull);
	static D3D11_FILL_MODE ToD3D11(EFillMode InFill);

	// Getter & Setter
	ID3D11Device* GetDevice() const { return DeviceResources->GetDevice(); }
	ID3D11DeviceContext* GetDeviceContext() const { return DeviceResources->GetDeviceContext(); }
	IDXGISwapChain* GetSwapChain() const { return DeviceResources->GetSwapChain(); }
	ID3D11RenderTargetView* GetRenderTargetView() const { return DeviceResources->GetRenderTargetView(); }
	UDeviceResources* GetDeviceResources() const { return DeviceResources; }
	FViewport* GetViewportClient() const { return ViewportClient; }
	bool GetIsResizing() const { return bIsResizing; }

	void SetIsResizing(bool isResizing) { bIsResizing = isResizing; }

	// Config Loading
	void LoadLODSettings();

private:
	mutable struct FLODStats
	{
		uint32 LODUpdatesPerFrame = 0;
		float LODUpdateTimeMs = 0.0f;
		uint32 LOD0Count = 0;
		uint32 LOD1Count = 0;
		uint32 LOD2Count = 0;
	} LODStats;

	// LOD Config Settings
	bool bLODEnabled = true;
	float LODDistanceSquared0 = 100.0f;   // 10^2 = 100
	float LODDistanceSquared1 = 6400.0f;  // 80^2 = 6400
	// 카메라 위치 캐싱 (성능 최적화)
	mutable FVector LastFrameCameraPos;

	/**
	 * @brief 초고속 LOD 업데이트 (SIMD 최적화)
	 */
	__forceinline void UpdateLODFast(UStaticMeshComponent* MeshComp, const FVector& CameraPos) const;

	/**
	 * @brief SIMD를 이용한 4개 객체 동시 LOD 계산
	 */
	void UpdateLODsBatchSIMD(UStaticMeshComponent** MeshComps, int Count, const FVector& CameraPos) const;

	UPipeline* Pipeline = nullptr;
	UDeviceResources* DeviceResources = nullptr;
	UFontRenderer* FontRenderer = nullptr;
	TArray<UPrimitiveComponent*> PrimitiveComponents;

	ID3D11DepthStencilState* DefaultDepthStencilState = nullptr;
	ID3D11DepthStencilState* DisabledDepthStencilState = nullptr;
	ID3D11DepthStencilState* ReadOnlyDepthStencilState = nullptr;  // Z-Test O, Z-Write X
	ID3D11Buffer* ConstantBufferModels = nullptr;
	ID3D11Buffer* ConstantBufferViewProj = nullptr;
	ID3D11Buffer* ConstantBufferColor = nullptr;
	ID3D11Buffer* ConstantBufferBatchLine = nullptr;
	ID3D11Buffer* ConstantBufferMaterial = nullptr;

	FLOAT ClearColor[4] = {0.025f, 0.025f, 0.025f, 1.0f};

	ID3D11VertexShader* DefaultVertexShader = nullptr;
	ID3D11PixelShader* DefaultPixelShader = nullptr;
	ID3D11InputLayout* DefaultInputLayout = nullptr;
	
	ID3D11VertexShader* TextureVertexShader = nullptr;
	ID3D11PixelShader* TexturePixelShader = nullptr;
	ID3D11InputLayout* TextureInputLayout = nullptr;

	// Compute Shader resources
	ID3D11ComputeShader* TestComputeShader = nullptr;
	ID3D11Buffer* ComputeConstantBuffer = nullptr;

	// HZB resources
	ID3D11ComputeShader* HZBComputeShader = nullptr;
	ID3D11ComputeShader* DepthCopyComputeShader = nullptr;
	ID3D11Buffer* HZBConstantBuffer = nullptr;
	ID3D11Texture2D* HZBTextures[4] = { nullptr }; // Mip levels 0-3
	ID3D11UnorderedAccessView* HZBUAVs[4] = { nullptr }; // UAVs for each mip level
	ID3D11ShaderResourceView* HZBSRVs[4] = { nullptr }; // SRVs for each mip level

	// Depth buffer copy for HZB generation (to avoid read/write conflicts)
	ID3D11Texture2D* HZBDepthCopy = nullptr;
	ID3D11ShaderResourceView* HZBDepthCopySRV = nullptr;
	ID3D11UnorderedAccessView* HZBDepthCopyUAV = nullptr;

	uint32 Stride = 0;

	FViewport* ViewportClient = nullptr;

	struct FRasterKey
	{
		D3D11_FILL_MODE FillMode = {};
		D3D11_CULL_MODE CullMode = {};

		bool operator==(const FRasterKey& InKey) const
		{
			return FillMode == InKey.FillMode && CullMode == InKey.CullMode;
		}
	};

	struct FRasterKeyHasher
	{
		size_t operator()(const FRasterKey& InKey) const noexcept
		{
			auto Mix = [](size_t& H, size_t V)
			{
				H ^= V + 0x9e3779b97f4a7c15ULL + (H << 6) + (H << 2);
			};

			size_t H = 0;
			Mix(H, (size_t)InKey.FillMode);
			Mix(H, (size_t)InKey.CullMode);

			return H;
		}
	};

	TMap<FRasterKey, ID3D11RasterizerState*, FRasterKeyHasher> RasterCache;

	ID3D11RasterizerState* GetRasterizerState(const FRenderState& InRenderState);

	bool bIsResizing = false;
};
