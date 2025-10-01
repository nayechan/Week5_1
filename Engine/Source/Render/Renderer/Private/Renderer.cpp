#include "pch.h"
#include "Render/Renderer/Public/Renderer.h"
#include <chrono>
#include "Render/Renderer/Public/Pipeline.h"
#include "Render/FontRenderer/Public/FontRenderer.h"
#include "Component/Public/BillBoardComponent.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Editor/Public/Editor.h"
#include "Editor/Public/Viewport.h"
#include "Editor/Public/ViewportClient.h"
#include "Editor/Public/Camera.h"
#include "Level/Public/Level.h"
#include "Manager/Level/Public/LevelManager.h"
#include "Manager/UI/Public/UIManager.h"
#include "Manager/Config/Public/ConfigManager.h"
#include "Render/UI/Overlay/Public/StatOverlay.h"
#include "Texture/Public/Material.h"
#include "Texture/Public/Texture.h"
#include "Texture/Public/TextureRenderProxy.h"
#include "Source/Component/Mesh/Public/StaticMesh.h"
#include "Render/Spatial/Public/Frustum.h"
#include "Utility/Public/ScopeCycleCounter.h"
#include <immintrin.h>
#include "Component/Public/TextRenderComponent.h"

IMPLEMENT_SINGLETON_CLASS_BASE(URenderer)

URenderer::URenderer() = default;

URenderer::~URenderer() = default;

void URenderer::Init(HWND InWindowHandle)
{
	DeviceResources = new UDeviceResources(InWindowHandle);
	Pipeline = new UPipeline(GetDeviceContext());
	ViewportClient = new FViewport();
	

	// 래스터라이저 상태 생성
	CreateRasterizerState();
	CreateDepthStencilState();
	CreateDefaultShader();
	CreateTextureShader();
	CreateComputeShader();
	CreateConstantBuffer();
	InitializeHZB();

	// FontRenderer 초기화
	FontRenderer = new UFontRenderer();
	if (!FontRenderer->Initialize())
	{
		UE_LOG("FontRenderer 초기화 실패");
		SafeDelete(FontRenderer);
	}

	ViewportClient->InitializeLayout(DeviceResources->GetViewportInfo());

	// Compute Shader 테스트 실행 (한번만)
	TestComputeShaderExecution();

	// LOD 설정 로드
	LoadLODSettings();
}

void URenderer::Release()
{
	ReleaseConstantBuffer();
	ReleaseDefaultShader();
	ReleaseComputeShader();
	ReleaseDepthStencilState();
	ReleaseRasterizerState();

	SafeDelete(ViewportClient);

	// FontRenderer 해제
	SafeDelete(FontRenderer);

	SafeDelete(Pipeline);
	SafeDelete(DeviceResources);
}

/**
 * @brief 래스터라이저 상태를 생성하는 함수
 */
void URenderer::CreateRasterizerState()
{
	// 현재 따로 생성하지 않음
}

/**
 * @brief Renderer에서 주로 사용할 Depth-Stencil State를 생성하는 함수
 */
void URenderer::CreateDepthStencilState()
{
	// 3D Default Depth Stencil 설정 (Depth 판정 O, Stencil 판정 X)
	D3D11_DEPTH_STENCIL_DESC DefaultDescription = {};

	DefaultDescription.DepthEnable = TRUE;
	DefaultDescription.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	DefaultDescription.DepthFunc = D3D11_COMPARISON_LESS;

	DefaultDescription.StencilEnable = FALSE;
	DefaultDescription.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	DefaultDescription.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

	GetDevice()->CreateDepthStencilState(&DefaultDescription, &DefaultDepthStencilState);

	// Disabled Depth Stencil 설정 (Depth 판정 X, Stencil 판정 X)
	D3D11_DEPTH_STENCIL_DESC DisabledDescription = {};

	DisabledDescription.DepthEnable = FALSE;
	DisabledDescription.StencilEnable = FALSE;

	GetDevice()->CreateDepthStencilState(&DisabledDescription, &DisabledDepthStencilState);

	// Read-Only Depth Stencil 설정 (Z-Test O, Z-Write X) - 디버깅 요소용
	D3D11_DEPTH_STENCIL_DESC ReadOnlyDescription = {};

	ReadOnlyDescription.DepthEnable = TRUE;          // Depth 테스트는 수행
	ReadOnlyDescription.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;  // Depth 쓰기는 비활성화
	ReadOnlyDescription.DepthFunc = D3D11_COMPARISON_LESS;

	ReadOnlyDescription.StencilEnable = FALSE;
	ReadOnlyDescription.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	ReadOnlyDescription.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

	GetDevice()->CreateDepthStencilState(&ReadOnlyDescription, &ReadOnlyDepthStencilState);
}

/**
 * @brief Shader 기반의 CSO 생성 함수
 */
void URenderer::CreateDefaultShader()
{
	ID3DBlob* VertexShaderCSO;
	ID3DBlob* PixelShaderCSO;

	D3DCompileFromFile(L"Asset/Shader/SampleShader.hlsl", nullptr, nullptr, "mainVS", "vs_5_0", 0, 0,
		&VertexShaderCSO, nullptr);

	GetDevice()->CreateVertexShader(VertexShaderCSO->GetBufferPointer(),
		VertexShaderCSO->GetBufferSize(), nullptr, &DefaultVertexShader);

	D3DCompileFromFile(L"Asset/Shader/SampleShader.hlsl", nullptr, nullptr, "mainPS", "ps_5_0", 0, 0,
		&PixelShaderCSO, nullptr);

	GetDevice()->CreatePixelShader(PixelShaderCSO->GetBufferPointer(),
		PixelShaderCSO->GetBufferSize(), nullptr, &DefaultPixelShader);

	D3D11_INPUT_ELEMENT_DESC DefaultLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FNormalVertex, Position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FNormalVertex, Normal), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(FNormalVertex, Color), D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(FNormalVertex, TexCoord), D3D11_INPUT_PER_VERTEX_DATA, 0	}
	};

	GetDevice()->CreateInputLayout(DefaultLayout, ARRAYSIZE(DefaultLayout), VertexShaderCSO->GetBufferPointer(),
		VertexShaderCSO->GetBufferSize(), &DefaultInputLayout);

	Stride = sizeof(FNormalVertex);

	VertexShaderCSO->Release();
	PixelShaderCSO->Release();
}

void URenderer::CreateTextureShader()
{
	ID3DBlob* TextureVSBlob;
	ID3DBlob* TexturePSBlob;

	D3DCompileFromFile(L"Asset/Shader/TextureShader.hlsl", nullptr, nullptr, "mainVS", "vs_5_0", 0, 0,
		&TextureVSBlob, nullptr);

	GetDevice()->CreateVertexShader(TextureVSBlob->GetBufferPointer(),
		TextureVSBlob->GetBufferSize(), nullptr, &TextureVertexShader);

	D3DCompileFromFile(L"Asset/Shader/TextureShader.hlsl", nullptr, nullptr, "mainPS", "ps_5_0", 0, 0,
		&TexturePSBlob, nullptr);

	GetDevice()->CreatePixelShader(TexturePSBlob->GetBufferPointer(),
		TexturePSBlob->GetBufferSize(), nullptr, &TexturePixelShader);

	D3D11_INPUT_ELEMENT_DESC TextureLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FNormalVertex, Position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FNormalVertex, Normal), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(FNormalVertex, Color), D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(FNormalVertex, TexCoord), D3D11_INPUT_PER_VERTEX_DATA, 0	}
	};
	GetDevice()->CreateInputLayout(TextureLayout, ARRAYSIZE(TextureLayout), TextureVSBlob->GetBufferPointer(),
		TextureVSBlob->GetBufferSize(), &TextureInputLayout);

	// TODO(KHJ): ShaderBlob 파일로 저장하고, 이후 이미 존재하는 경우 컴파일 없이 Blob을 로드할 수 있도록 할 것
	// TODO(KHJ): 실제 텍스처용 셰이더를 별도로 생성해야 함 (UV 좌표 포함)

	TextureVSBlob->Release();
	TexturePSBlob->Release();
}

/**
 * @brief 래스터라이저 상태를 해제하는 함수
 */
void URenderer::ReleaseRasterizerState()
{
	for (auto& Cache : RasterCache)
	{
		if (Cache.second != nullptr)
		{
			Cache.second->Release();
		}
	}
	RasterCache.clear();
}

/**
 * @brief Shader Release
 */
void URenderer::ReleaseDefaultShader()
{
	if (DefaultInputLayout)
	{
		DefaultInputLayout->Release();
		DefaultInputLayout = nullptr;
	}

	if (DefaultPixelShader)
	{
		DefaultPixelShader->Release();
		DefaultPixelShader = nullptr;
	}

	if (DefaultVertexShader)
	{
		DefaultVertexShader->Release();
		DefaultVertexShader = nullptr;
	}
}

/**
 * @brief 렌더러에 사용된 모든 리소스를 해제하는 함수
 */
void URenderer::ReleaseDepthStencilState()
{
	if (DefaultDepthStencilState)
	{
		DefaultDepthStencilState->Release();
		DefaultDepthStencilState = nullptr;
	}

	if (DisabledDepthStencilState)
	{
		DisabledDepthStencilState->Release();
		DisabledDepthStencilState = nullptr;
	}

	if (ReadOnlyDepthStencilState)
	{
		ReadOnlyDepthStencilState->Release();
		ReadOnlyDepthStencilState = nullptr;
	}

	// 렌더 타겟을 초기화
	if (GetDeviceContext())
	{
		GetDeviceContext()->OMSetRenderTargets(0, nullptr, nullptr);
	}
}

// Renderer.cpp
void URenderer::Update()
{

	RenderBegin();

	// FViewportClient로부터 모든 뷰포트를 가져옵니다.
	for (FViewportClient& ViewportClient : ViewportClient->GetViewports())
	{
		// 0. 뷰포트가 숨겨져 있다면 렌더링을 하지 않습니다.
		if (!ViewportClient.bIsVisible) { continue; }

		// 1. 현재 뷰포트가 닫혀있다면 렌더링을 하지 않습니다.
		if (ViewportClient.GetViewportInfo().Width < 1.0f || ViewportClient.GetViewportInfo().Height < 1.0f) { continue; }

		// 1. 현재 뷰포트의 영역을 설정합니다.
		ViewportClient.Apply(GetDeviceContext());

		// 2. 현재 뷰포트의 카메라 정보를 가져옵니다.
		UCamera* CurrentCamera = &ViewportClient.Camera;

		// 3. 해당 카메라의 View/Projection 행렬로 상수 버퍼를 업데이트합니다.
		CurrentCamera->Update(ViewportClient.GetViewportInfo());
		UpdateConstant(CurrentCamera->GetFViewProjConstants());

		// 4. 씬(레벨, 에디터 요소 등)을 이 뷰포트와 카메라 기준으로 렌더링합니다.
		RenderLevel(CurrentCamera);

		// 5. 에디터를 렌더링합니다.
		ULevelManager::GetInstance().GetEditor()->RenderEditor(CurrentCamera);
	}

	// HZB 생성 (매 프레임 깊이 버퍼 완료 후)
	GenerateHZB();

	// HZB 생성 직후 Occlusion Culling용 캐시 생성
	CacheHZBForOcclusion();


	// 최상위 에디터/GUI는 프레임에 1회만
	UUIManager::GetInstance().Render();
	UStatOverlay::GetInstance().Render();

	RenderEnd(); // Present 1회
}


/**
 * @brief Render Prepare Step
 */
void URenderer::RenderBegin() const
{
	auto* RenderTargetView = DeviceResources->GetRenderTargetView();
	GetDeviceContext()->ClearRenderTargetView(RenderTargetView, ClearColor);
	auto* DepthStencilView = DeviceResources->GetDepthStencilView();
	GetDeviceContext()->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	ID3D11RenderTargetView* rtvs[] = { RenderTargetView }; // 배열 생성

	GetDeviceContext()->OMSetRenderTargets(1, rtvs, DeviceResources->GetDepthStencilView());
	DeviceResources->UpdateViewport();
}

/**
 * @brief Buffer에 데이터 입력 및 Draw
 */
void URenderer::RenderLevel(UCamera* InCurrentCamera)
{

    // Level 없으면 Early Return
    ULevel* CurrentLevel = ULevelManager::GetInstance().GetCurrentLevel();
    if (!CurrentLevel)
        return;

	// 통계 초기화
	uint32 totalStaticPrimitives = CurrentLevel->GetStaticOctree().GetObjectCount();
	uint32 totalDynamicPrimitives = CurrentLevel->GetDynamicPrimitives().size();
    uint32 lodCounts[3] = { 0 };


	// Frustum Culling Test
	FFrustum ViewFrustum = InCurrentCamera->GetViewFrustum();

	// 옥클루전 콜백 구조체 준비
	struct OcclusionContext
	{
		const URenderer* Renderer;
		const UCamera* Camera;
		bool bHZBValid;
	};

	OcclusionContext Context = { this, InCurrentCamera, bHZBCacheValid && !CachedHZBLevel3.empty() };

	// 옥클루전 콜백 함수 (노드 AABB용 - 보수적 처리)
	auto IsOccludedCallback = [](const FAABB& bounds, const void* context) -> bool
	{
		const OcclusionContext* ctx = static_cast<const OcclusionContext*>(context);
		if (!ctx->bHZBValid) return false;


		return ctx->Renderer->IsOccluded(bounds.Min, bounds.Max, ctx->Camera, 2.5f);  // 노드도 3배 확장 (보수적)
	};

	// 렌더링 통계를 위한 카운터
	uint32 renderedPrimitiveCount = 0;

	// Get view mode from editor
    const EViewModeIndex ViewMode = ULevelManager::GetInstance().GetEditor()->GetViewMode();

	// 렌더링 콜백 함수
	auto RenderCallback = [&renderedPrimitiveCount, &lodCounts, &InCurrentCamera, ViewMode, this](UPrimitiveComponent* primitive, const void* context) -> void
	{
		if (!primitive) return;
		if (!primitive->IsVisible())
		{
			UE_LOG("Renderer: Primitive %p not visible, skipping render", primitive);
			return;
		}

		// LOD 업데이트 추가 (StaticMesh인 경우에만, 3프레임마다)
		static int lodFrameCounter = 0;
		if (primitive->GetPrimitiveType() == EPrimitiveType::StaticMesh && (++lodFrameCounter % 6) == 0)
		{
			UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(primitive);
			if (MeshComp)
			{
				// 초고속 LOD 업데이트 (제곱근 계산 없음!)
				UpdateLODFast(MeshComp, InCurrentCamera->GetLocation());
			}
		}

		// 카운트 증가
		renderedPrimitiveCount++;

		// 렌더 상태 설정
		FRenderState RenderState = primitive->GetRenderState();
		if (ViewMode == EViewModeIndex::VMI_Wireframe)
		{
			RenderState.CullMode = ECullMode::None;
			RenderState.FillMode = EFillMode::WireFrame;
		}

		ID3D11RasterizerState* LoadedRasterizerState = GetRasterizerState(RenderState);

		switch (primitive->GetPrimitiveType())
		{
		case EPrimitiveType::StaticMesh:
        {
            UStaticMeshComponent* MeshComponent = Cast<UStaticMeshComponent>(primitive);
            if (MeshComponent)
            {
                UStaticMeshComponent* MeshComponent = Cast<UStaticMeshComponent>(primitive);
                if (MeshComponent)
                {
					// LOD는 이미 UpdateLODFast()에서 계산되었으므로 중복 계산 제거
					int32 LodIndex = MeshComponent->GetCurrentLODIndex();

					/*FVector Min, Max;
					MeshComponent->GetWorldAABB(Min, Max);
					FVector CompLocation = (Min + Max) * 0.5f;
					FVector CamerLocation = InCurrentCamera->GetLocation();
					float DistSq = (CamerLocation - CompLocation).LengthSquared();

				const TArray<float>& LodDistanceSq = MeshComponent->GetLODDistancesSquared();
				int32 LodIndex = 0;
				for (int32 i = LodDistanceSq.size() - 1; i >= 0; i--)
				{
					if (DistSq >= LodDistanceSq[i])
					{
						LodIndex = i;
						break;
					}
					MeshComponent->SetCurrentLODIndex(LodIndex);*/
                    if (LodIndex >= 0 && LodIndex < 3)
                    {
                        lodCounts[LodIndex]++;
                    }
                }
            }
			RenderStaticMesh(MeshComponent, LoadedRasterizerState);
			break;
        }
		default:
			RenderPrimitiveDefault(primitive, LoadedRasterizerState);
			break;
		}
	};


	{
		FScopeCycleCounter Counter(GetCullingStatId());
		// 옥트리에서 람다 기반 렌더링 수행 (Static Primitives)
		CurrentLevel->GetStaticOctree().QueryFrustumWithRenderCallback(ViewFrustum, IsOccludedCallback, &Context, RenderCallback, nullptr);
	}

	// Dynamic Primitives 처리
	const auto& DynamicPrimitives = CurrentLevel->GetDynamicPrimitives();
	for (UPrimitiveComponent* DynPrim : DynamicPrimitives)
	{
		if (!DynPrim)
		{
			continue;
		}
		if (!DynPrim->IsVisible())
		{
			continue;
		}
		FVector WorldMin, WorldMax;
		DynPrim->GetWorldAABB(WorldMin, WorldMax);
		bool bInFrustum = ViewFrustum.IsBoxInFrustum(FAABB(WorldMin, WorldMax));
		if (!bInFrustum)
		{
			continue;
		}
		RenderCallback(DynPrim, nullptr);
	}
	
}

/**
 * @brief Editor용 Primitive를 렌더링하는 함수 (Gizmo, Axis 등)
 * @param InPrimitive 렌더링할 에디터 프리미티브
 * @param InRenderState 렌더링 상태
 */
void URenderer::RenderPrimitive(const FEditorPrimitive& InPrimitive, const FRenderState& InRenderState)
{
	// 에디터 프리미티브는 ReadOnly Depth 사용 (HZB 오염 방지)
	// Always visible인 경우에만 완전히 비활성화
	ID3D11DepthStencilState* DepthStencilState =
		InPrimitive.bShouldAlwaysVisible ? DisabledDepthStencilState : ReadOnlyDepthStencilState;

	ID3D11RasterizerState* RasterizerState = GetRasterizerState(InRenderState);

	// Pipeline 정보 구성
	FPipelineInfo PipelineInfo = {
		DefaultInputLayout,
		DefaultVertexShader,
		RasterizerState,
		DepthStencilState,
		DefaultPixelShader,
		nullptr,
		InPrimitive.Topology
	};

	Pipeline->UpdatePipeline(PipelineInfo);

	// Update constant buffers
	Pipeline->SetConstantBuffer(0, true, ConstantBufferModels);
	UpdateConstant(InPrimitive.Location, InPrimitive.Rotation, InPrimitive.Scale);

	Pipeline->SetConstantBuffer(2, true, ConstantBufferColor);
	UpdateConstant(InPrimitive.Color);

	// Set vertex buffer and draw
	Pipeline->SetVertexBuffer(InPrimitive.Vertexbuffer, Stride);
	Pipeline->Draw(InPrimitive.NumVertices, 0);
}

/**
 * @brief Index Buffer를 사용하는 Editor Primitive 렌더링 함수
 * @param InPrimitive 렌더링할 에디터 프리미티브
 * @param InRenderState 렌더링 상태
 * @param bInUseBaseConstantBuffer 기본 상수 버퍼 사용 여부
 * @param InStride 정점 스트라이드
 * @param InIndexBufferStride 인덱스 버퍼 스트라이드
 */
void URenderer::RenderPrimitiveIndexed(const FEditorPrimitive& InPrimitive, const FRenderState& InRenderState,
	bool bInUseBaseConstantBuffer, uint32 InStride, uint32 InIndexBufferStride)
{
	// 에디터 프리미티브는 ReadOnly Depth 사용 (HZB 오염 방지)
	// Always visible인 경우에만 완전히 비활성화
	ID3D11DepthStencilState* DepthStencilState =
		InPrimitive.bShouldAlwaysVisible ? DisabledDepthStencilState : ReadOnlyDepthStencilState;

	ID3D11RasterizerState* RasterizerState = GetRasterizerState(InRenderState);

	// 커스텀 셰이더가 있으면 사용, 없으면 기본 셰이더 사용
	ID3D11InputLayout* InputLayout = InPrimitive.InputLayout ? InPrimitive.InputLayout : DefaultInputLayout;
	ID3D11VertexShader* VertexShader = InPrimitive.VertexShader ? InPrimitive.VertexShader : DefaultVertexShader;
	ID3D11PixelShader* PixelShader = InPrimitive.PixelShader ? InPrimitive.PixelShader : DefaultPixelShader;

	// Pipeline 정보 구성
	FPipelineInfo PipelineInfo = {
		InputLayout,
		VertexShader,
		RasterizerState,
		DepthStencilState,
		PixelShader,
		nullptr,
		InPrimitive.Topology
	};

	Pipeline->UpdatePipeline(PipelineInfo);

	// 기본 상수 버퍼 사용하는 경우에만 업데이트
	if (bInUseBaseConstantBuffer)
	{
		Pipeline->SetConstantBuffer(0, true, ConstantBufferModels);
		UpdateConstant(InPrimitive.Location, InPrimitive.Rotation, InPrimitive.Scale);

		Pipeline->SetConstantBuffer(2, true, ConstantBufferColor);
		UpdateConstant(InPrimitive.Color);
	}

	// Set buffers and draw indexed
	Pipeline->SetIndexBuffer(InPrimitive.IndexBuffer, InIndexBufferStride);
	Pipeline->SetVertexBuffer(InPrimitive.Vertexbuffer, InStride);
	Pipeline->DrawIndexed(InPrimitive.NumIndices, 0, 0);
}

/**
 * @brief 스왑 체인의 백 버퍼와 프론트 버퍼를 교체하여 화면에 출력
 */
void URenderer::RenderEnd() const
{
	GetSwapChain()->Present(0, 0); // 1: VSync 활성화
}

void URenderer::RenderStaticMesh(UStaticMeshComponent* InMeshComp, ID3D11RasterizerState* InRasterizerState)
{
    UStaticMesh* MeshAsset = InMeshComp->GetStaticMesh();
	if (!MeshAsset || !MeshAsset->IsValid()) return;

    // Get the LOD index that was pre-calculated in RenderLevel
    int32 lodIndex = InMeshComp->GetCurrentLODIndex();

    // Get the mesh data and buffers for the selected LOD
	FStaticMesh* MeshData = MeshAsset->GetLOD(lodIndex);
    ID3D11Buffer* vb = InMeshComp->GetVertexBuffer(lodIndex);
    ID3D11Buffer* ib = InMeshComp->GetIndexBuffer(lodIndex);

	if (!MeshData || !vb || !ib) return;

	// Set up pipeline and render
	FPipelineInfo PipelineInfo = {
		TextureInputLayout,
		TextureVertexShader,
		InRasterizerState,
		DefaultDepthStencilState,
		TexturePixelShader,
		nullptr,
	};
	Pipeline->UpdatePipeline(PipelineInfo);

	// Constant buffer & transform
	Pipeline->SetConstantBuffer(0, true, ConstantBufferModels);
	UpdateConstant(
		InMeshComp->GetRelativeLocation(),
		InMeshComp->GetRelativeRotation(),
		InMeshComp->GetRelativeScale3D()
	);

	Pipeline->SetVertexBuffer(vb, sizeof(FNormalVertex));
	Pipeline->SetIndexBuffer(ib, 0);

	// If no material is assigned, render the entire mesh using the default shader
	if (MeshData->MaterialInfo.empty() || InMeshComp->GetStaticMesh()->GetNumMaterials() == 0)
	{
		Pipeline->DrawIndexed(MeshData->Indices.size(), 0, 0);
		return;
	}

	if (InMeshComp->IsScrollEnabled())
	{
		UTimeManager& TimeManager = UTimeManager::GetInstance();
		InMeshComp->SetElapsedTime(InMeshComp->GetElapsedTime() + TimeManager.GetDeltaTime());
	}

	for (const FMeshSection& Section : MeshData->Sections)
	{
		UMaterial* Material = InMeshComp->GetMaterial(Section.MaterialSlot);
		if (Material)
		{
			FMaterialConstants MaterialConstants = {};
			FVector AmbientColor = Material->GetAmbientColor();
			MaterialConstants.Ka = FVector4(AmbientColor.X, AmbientColor.Y, AmbientColor.Z, 1.0f);
			FVector DiffuseColor = Material->GetDiffuseColor();
			MaterialConstants.Kd = FVector4(DiffuseColor.X, DiffuseColor.Y, DiffuseColor.Z, 1.0f);
			FVector SpecularColor = Material->GetSpecularColor();
			MaterialConstants.Ks = FVector4(SpecularColor.X, SpecularColor.Y, SpecularColor.Z, 1.0f);
			MaterialConstants.Ns = Material->GetSpecularExponent();
			MaterialConstants.Ni = Material->GetRefractionIndex();
			MaterialConstants.D = Material->GetDissolveFactor();
			MaterialConstants.MaterialFlags = 0; // Placeholder
			MaterialConstants.Time = InMeshComp->GetElapsedTime();

			// Update Constant Buffer
			UpdateConstant(MaterialConstants);

			if (Material->GetDiffuseTexture())
			{
				auto* Proxy = Material->GetDiffuseTexture()->GetRenderProxy();
				Pipeline->SetTexture(0, false, Proxy->GetSRV());
				Pipeline->SetSamplerState(0, false, Proxy->GetSampler());
			}
			if (Material->GetAmbientTexture())
			{
				auto* Proxy = Material->GetAmbientTexture()->GetRenderProxy();
				Pipeline->SetTexture(1, false, Proxy->GetSRV());
			}
			if (Material->GetSpecularTexture())
			{
				auto* Proxy = Material->GetSpecularTexture()->GetRenderProxy();
				Pipeline->SetTexture(2, false, Proxy->GetSRV());
			}
			if (Material->GetAlphaTexture())
			{
				auto* Proxy = Material->GetAlphaTexture()->GetRenderProxy();
				Pipeline->SetTexture(4, false, Proxy->GetSRV());
			}
		}
		Pipeline->DrawIndexed(Section.IndexCount, Section.StartIndex, 0);
	}
}

void URenderer::RenderBillboard(UBillBoardComponent* InBillBoardComp, UCamera* InCurrentCamera)
{
	if (!InCurrentCamera)	return;

	// 이제 올바른 카메라 위치를 전달하여 빌보드 회전 업데이트
	InBillBoardComp->UpdateRotationMatrix(InCurrentCamera->GetLocation());

	FString UUIDString = "UID: " + std::to_string(InBillBoardComp->GetUUID());
	FMatrix RT = InBillBoardComp->GetRTMatrix();

	// UEditor에서 가져오는 대신, 인자로 받은 카메라의 ViewProj 행렬을 사용
	const FViewProjConstants& viewProjConstData = InCurrentCamera->GetFViewProjConstants();
	FontRenderer->RenderText(UUIDString.c_str(), RT, viewProjConstData);
}

void URenderer::RenderText(UTextRenderComponent* InTextRenderComp, UCamera* InCurrentCamera)
{
	if (!InCurrentCamera)
	{
		return;
	}

	FMatrix RT;
	// TODO: FMatrix RT = InTextRenderComp->GetRTMatrix();

	const FViewProjConstants& viewProjConstData = InCurrentCamera->GetFViewProjConstants();
	FontRenderer->RenderText(InTextRenderComp->GetText().c_str(), RT, viewProjConstData);
}

void URenderer::RenderPrimitiveDefault(UPrimitiveComponent* InPrimitiveComp, ID3D11RasterizerState* InRasterizerState)
{

	// Update pipeline info
	FPipelineInfo PipelineInfo = {
		DefaultInputLayout,
		DefaultVertexShader,
		InRasterizerState,
		DefaultDepthStencilState,
		DefaultPixelShader,
		nullptr,
	};
	Pipeline->UpdatePipeline(PipelineInfo);

	// Update pipeline buffers
	Pipeline->SetConstantBuffer(0, true, ConstantBufferModels);
	UpdateConstant(
		InPrimitiveComp->GetRelativeLocation(),
		InPrimitiveComp->GetRelativeRotation(),
		InPrimitiveComp->GetRelativeScale3D()
	);
	Pipeline->SetConstantBuffer(2, true, ConstantBufferColor);
	UpdateConstant(InPrimitiveComp->GetColor());

	// Bind vertex buffer
	Pipeline->SetVertexBuffer(InPrimitiveComp->GetVertexBuffer(), Stride);

	// Draw vertex + index
	if (InPrimitiveComp->GetIndexBuffer() && InPrimitiveComp->GetIndicesData())
	{
		Pipeline->SetIndexBuffer(InPrimitiveComp->GetIndexBuffer(), 0);
		Pipeline->DrawIndexed(InPrimitiveComp->GetNumIndices(), 0, 0);
	}
	else
	{
		Pipeline->Draw(static_cast<uint32>(InPrimitiveComp->GetNumVertices()), 0);
	}
}

/**
 * @brief FVertex 타입용 정점 Buffer 생성 함수
 * @param InVertices 정점 데이터 포인터
 * @param InByteWidth 버퍼 크기 (바이트 단위)
 * @return 생성된 D3D11 정점 버퍼
 */
ID3D11Buffer* URenderer::CreateVertexBuffer(FNormalVertex* InVertices, uint32 InByteWidth) const
{
	D3D11_BUFFER_DESC VertexBufferDescription = {};
	VertexBufferDescription.ByteWidth = InByteWidth;
	VertexBufferDescription.Usage = D3D11_USAGE_IMMUTABLE; // 변경되지 않는 정적 데이터
	VertexBufferDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA VertexBufferInitData = { InVertices };

	ID3D11Buffer* VertexBuffer = nullptr;
	GetDevice()->CreateBuffer(&VertexBufferDescription, &VertexBufferInitData, &VertexBuffer);

	return VertexBuffer;
}

/**
 * @brief FVector 타입용 정점 Buffer 생성 함수
 * @param InVertices 정점 데이터 포인터
 * @param InByteWidth 버퍼 크기 (바이트 단위)
 * @param bCpuAccess CPU에서 접근 가능한 동적 버퍼 여부
 * @return 생성된 D3D11 정점 버퍼
 */
ID3D11Buffer* URenderer::CreateVertexBuffer(FVector* InVertices, uint32 InByteWidth, bool bCpuAccess) const
{
	D3D11_BUFFER_DESC VertexBufferDescription = {};
	VertexBufferDescription.ByteWidth = InByteWidth;
	VertexBufferDescription.Usage = D3D11_USAGE_IMMUTABLE; // 기본값: 정적 데이터
	VertexBufferDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	// CPU 접근이 필요한 경우 동적 버퍼로 변경
	if (bCpuAccess)
	{
		VertexBufferDescription.Usage = D3D11_USAGE_DYNAMIC; // CPU에서 자주 수정할 경우
		VertexBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // CPU 쓰기 가능
		VertexBufferDescription.MiscFlags = 0;
	}

	D3D11_SUBRESOURCE_DATA VertexBufferInitData = { InVertices };

	ID3D11Buffer* VertexBuffer = nullptr;
	GetDevice()->CreateBuffer(&VertexBufferDescription, &VertexBufferInitData, &VertexBuffer);

	return VertexBuffer;
}

/**
 * @brief Index Buffer 생성 함수
 * @param InIndices 인덱스 데이터 포인터
 * @param InByteWidth 버퍼 크기 (바이트 단위)
 * @return 생성된 D3D11 인덱스 버퍼
 */
ID3D11Buffer* URenderer::CreateIndexBuffer(const void* InIndices, uint32 InByteWidth) const
{
	D3D11_BUFFER_DESC IndexBufferDescription = {};
	IndexBufferDescription.ByteWidth = InByteWidth;
	IndexBufferDescription.Usage = D3D11_USAGE_IMMUTABLE;
	IndexBufferDescription.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA IndexBufferInitData = {};
	IndexBufferInitData.pSysMem = InIndices;

	ID3D11Buffer* IndexBuffer = nullptr;
	GetDevice()->CreateBuffer(&IndexBufferDescription, &IndexBufferInitData, &IndexBuffer);
	return IndexBuffer;
}

/**
 * @brief 창 크기 변경 시 렌더 타곟 및 버퍼를 재설정하는 함수
 * @param InWidth 새로운 창 너비
 * @param InHeight 새로운 창 높이
 */
void URenderer::OnResize(uint32 InWidth, uint32 InHeight)
{
	// 필수 리소스가 유효하지 않으면 Early Return
	if (!DeviceResources || !GetDevice() || !GetDeviceContext() || !GetSwapChain())
	{
		return;
	}

	// 기존 버퍼들 해제
	DeviceResources->ReleaseFrameBuffer();
	DeviceResources->ReleaseDepthBuffer();
	GetDeviceContext()->OMSetRenderTargets(0, nullptr, nullptr);

	// SwapChain 버퍼 크기 재설정
	UStatOverlay::GetInstance().PreResize();
	HRESULT Result = GetSwapChain()->ResizeBuffers(2, InWidth, InHeight, DXGI_FORMAT_UNKNOWN, 0);
	if (FAILED(Result))
	{
		UE_LOG("OnResize Failed");
		return;
	}

	// 버퍼 재생성 및 렌더 타겟 설정
	DeviceResources->UpdateViewport();
	DeviceResources->CreateFrameBuffer();
	DeviceResources->CreateDepthBuffer();

	// 새로운 렌더 타겟 바인딩
	auto* RenderTargetView = DeviceResources->GetRenderTargetView();
	ID3D11RenderTargetView* RenderTargetViews[] = { RenderTargetView };
	GetDeviceContext()->OMSetRenderTargets(1, RenderTargetViews, DeviceResources->GetDepthStencilView());
	UStatOverlay::GetInstance().OnResize();

	// HZB 리소스 재초기화 (새로운 화면 크기에 맞춤)
	ReleaseHZBResources();
	InitializeHZB();
}

/**
 * @brief Vertex Buffer 해제 함수
 * @param InVertexBuffer 해제할 정점 버퍼
 */
void URenderer::ReleaseVertexBuffer(ID3D11Buffer* InVertexBuffer)
{
	if (InVertexBuffer)
	{
		InVertexBuffer->Release();
	}
}

/**
 * @brief Index Buffer 해제 함수
 * @param InIndexBuffer 해제할 인덱스 버퍼
 */
void URenderer::ReleaseIndexBuffer(ID3D11Buffer* InIndexBuffer)
{
	if (InIndexBuffer)
	{
		InIndexBuffer->Release();
	}
}

/**
 * @brief 커스텀 Vertex Shader와 Input Layout을 생성하는 함수
 * @param InFilePath 셰이더 파일 경로
 * @param InInputLayoutDescs Input Layout 스팩 배열
 * @param OutVertexShader 출력될 Vertex Shader 포인터
 * @param OutInputLayout 출력될 Input Layout 포인터
 */
void URenderer::CreateVertexShaderAndInputLayout(const wstring& InFilePath,
	const TArray<D3D11_INPUT_ELEMENT_DESC>& InInputLayoutDescs,
	ID3D11VertexShader** OutVertexShader,
	ID3D11InputLayout** OutInputLayout)
{
	ID3DBlob* VertexShaderBlob = nullptr;
	ID3DBlob* ErrorBlob = nullptr;

	// Vertex Shader 컴파일
	HRESULT Result = D3DCompileFromFile(InFilePath.data(), nullptr, nullptr, "mainVS", "vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0,
		&VertexShaderBlob, &ErrorBlob);

	// 컴파일 실패 시 에러 처리
	if (FAILED(Result))
	{
		if (ErrorBlob)
		{
			OutputDebugStringA(static_cast<char*>(ErrorBlob->GetBufferPointer()));
			ErrorBlob->Release();
		}
		return;
	}

	// Vertex Shader 객체 생성
	GetDevice()->CreateVertexShader(VertexShaderBlob->GetBufferPointer(),
		VertexShaderBlob->GetBufferSize(), nullptr, OutVertexShader);

	// Input Layout 생성
	GetDevice()->CreateInputLayout(InInputLayoutDescs.data(), static_cast<uint32>(InInputLayoutDescs.size()),
		VertexShaderBlob->GetBufferPointer(),
		VertexShaderBlob->GetBufferSize(), OutInputLayout);

	// TODO(KHJ): 이 값이 여기에 있는 게 맞나? 검토 필요
	Stride = sizeof(FNormalVertex);

	VertexShaderBlob->Release();
}

/**
 * @brief 커스텀 Pixel Shader를 생성하는 함수
 * @param InFilePath 셰이더 파일 경로
 * @param OutPixelShader 출력될 Pixel Shader 포인터
 */
void URenderer::CreatePixelShader(const wstring& InFilePath, ID3D11PixelShader** OutPixelShader) const
{
	ID3DBlob* PixelShaderBlob = nullptr;
	ID3DBlob* ErrorBlob = nullptr;

	// Pixel Shader 컴파일
	HRESULT Result = D3DCompileFromFile(InFilePath.data(), nullptr, nullptr, "mainPS", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0,
		&PixelShaderBlob, &ErrorBlob);

	// 컴파일 실패 시 에러 처리
	if (FAILED(Result))
	{
		if (ErrorBlob)
		{
			OutputDebugStringA(static_cast<char*>(ErrorBlob->GetBufferPointer()));
			ErrorBlob->Release();
		}
		return;
	}

	// Pixel Shader 객체 생성
	GetDevice()->CreatePixelShader(PixelShaderBlob->GetBufferPointer(),
		PixelShaderBlob->GetBufferSize(), nullptr, OutPixelShader);

	PixelShaderBlob->Release();
}

/**
 * @brief 렌더링에 사용될 상수 버퍼들을 생성하는 함수
 */
void URenderer::CreateConstantBuffer()
{
	// 모델 변환 행렬용 상수 버퍼 생성 (Slot 0)
	{
		D3D11_BUFFER_DESC ModelConstantBufferDescription = {};
		ModelConstantBufferDescription.ByteWidth = sizeof(FMatrix) + 0xf & 0xfffffff0; // 16바이트 단위 정렬
		ModelConstantBufferDescription.Usage = D3D11_USAGE_DYNAMIC; // 매 프레임 CPU에서 업데이트
		ModelConstantBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		ModelConstantBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		GetDevice()->CreateBuffer(&ModelConstantBufferDescription, nullptr, &ConstantBufferModels);
	}

	// 색상 정보용 상수 버퍼 생성 (Slot 2)
	{
		D3D11_BUFFER_DESC ColorConstantBufferDescription = {};
		ColorConstantBufferDescription.ByteWidth = sizeof(FVector4) + 0xf & 0xfffffff0; // 16바이트 단위 정렬
		ColorConstantBufferDescription.Usage = D3D11_USAGE_DYNAMIC; // 매 프레임 CPU에서 업데이트
		ColorConstantBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		ColorConstantBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		GetDevice()->CreateBuffer(&ColorConstantBufferDescription, nullptr, &ConstantBufferColor);
	}

	// 카메라 View/Projection 행렬용 상수 버퍼 생성 (Slot 1)
	{
		D3D11_BUFFER_DESC ViewProjConstantBufferDescription = {};
		ViewProjConstantBufferDescription.ByteWidth = sizeof(FViewProjConstants) + 0xf & 0xfffffff0; // 16바이트 단위 정렬
		ViewProjConstantBufferDescription.Usage = D3D11_USAGE_DYNAMIC; // 매 프레임 CPU에서 업데이트
		ViewProjConstantBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		ViewProjConstantBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		GetDevice()->CreateBuffer(&ViewProjConstantBufferDescription, nullptr, &ConstantBufferViewProj);
	}

	{
		D3D11_BUFFER_DESC MaterialConstantBufferDescription = {};
		MaterialConstantBufferDescription.ByteWidth = sizeof(FMaterial) + 0xf & 0xfffffff0;
		MaterialConstantBufferDescription.Usage = D3D11_USAGE_DYNAMIC; // 매 프레임 CPU에서 업데이트
		MaterialConstantBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		MaterialConstantBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		GetDevice()->CreateBuffer(&MaterialConstantBufferDescription, nullptr, &ConstantBufferMaterial);
	}
}

/**
 * @brief 상수 버퍼 소멸 함수
 */
void URenderer::ReleaseConstantBuffer()
{
	if (ConstantBufferModels)
	{
		ConstantBufferModels->Release();
		ConstantBufferModels = nullptr;
	}

	if (ConstantBufferColor)
	{
		ConstantBufferColor->Release();
		ConstantBufferColor = nullptr;
	}

	if (ConstantBufferViewProj)
	{
		ConstantBufferViewProj->Release();
		ConstantBufferViewProj = nullptr;
	}

	if (ConstantBufferMaterial)
	{
		ConstantBufferMaterial->Release();
		ConstantBufferMaterial = nullptr;
	}
}

void URenderer::UpdateConstant(const UPrimitiveComponent* InPrimitive) const
{
	if (ConstantBufferModels)
	{
		D3D11_MAPPED_SUBRESOURCE constantbufferMSR;

		GetDeviceContext()->Map(ConstantBufferModels, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR);
		// update constant buffer every frame
		FMatrix* Constants = static_cast<FMatrix*>(constantbufferMSR.pData);
		{
			*Constants = FMatrix::GetModelMatrix(InPrimitive->GetRelativeLocation(),
				FVector::GetDegreeToRadian(InPrimitive->GetRelativeRotation()),
				InPrimitive->GetRelativeScale3D());
		}
		GetDeviceContext()->Unmap(ConstantBufferModels, 0);
	}
}

/**
 * @brief 상수 버퍼 업데이트 함수
 * @param InPosition
 * @param InRotation
 * @param InScale Ball Size
 */
void URenderer::UpdateConstant(const FVector& InPosition, const FVector& InRotation, const FVector& InScale) const
{
	if (ConstantBufferModels)
	{
		D3D11_MAPPED_SUBRESOURCE constantbufferMSR;

		GetDeviceContext()->Map(ConstantBufferModels, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR);

		// update constant buffer every frame
		FMatrix* Constants = static_cast<FMatrix*>(constantbufferMSR.pData);
		{
			*Constants = FMatrix::GetModelMatrix(InPosition, FVector::GetDegreeToRadian(InRotation), InScale);
		}
		GetDeviceContext()->Unmap(ConstantBufferModels, 0);
	}
}

void URenderer::UpdateConstant(const FViewProjConstants& InViewProjConstants) const
{
	Pipeline->SetConstantBuffer(1, true, ConstantBufferViewProj);

	if (ConstantBufferViewProj)
	{
		D3D11_MAPPED_SUBRESOURCE ConstantBufferMSR = {};

		GetDeviceContext()->Map(ConstantBufferViewProj, 0, D3D11_MAP_WRITE_DISCARD, 0, &ConstantBufferMSR);
		// update constant buffer every frame
		FViewProjConstants* ViewProjectionConstants = static_cast<FViewProjConstants*>(ConstantBufferMSR.pData);
		{
			ViewProjectionConstants->View = InViewProjConstants.View;
			ViewProjectionConstants->Projection = InViewProjConstants.Projection;
		}
		GetDeviceContext()->Unmap(ConstantBufferViewProj, 0);
	}
}

void URenderer::UpdateConstant(const FMatrix& InMatrix) const
{
	if (ConstantBufferModels)
	{
		D3D11_MAPPED_SUBRESOURCE MappedSubResource;
		GetDeviceContext()->Map(ConstantBufferModels, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubResource);
		memcpy(MappedSubResource.pData, &InMatrix, sizeof(FMatrix));
		GetDeviceContext()->Unmap(ConstantBufferModels, 0);
	}
}

void URenderer::UpdateConstant(const FVector4& InColor) const
{
	Pipeline->SetConstantBuffer(2, false, ConstantBufferColor);

	if (ConstantBufferColor)
	{
		D3D11_MAPPED_SUBRESOURCE ConstantBufferMSR = {};

		GetDeviceContext()->Map(ConstantBufferColor, 0, D3D11_MAP_WRITE_DISCARD, 0, &ConstantBufferMSR);
		// update constant buffer every frame
		FVector4* ColorConstants = static_cast<FVector4*>(ConstantBufferMSR.pData);
		{
			ColorConstants->X = InColor.X;
			ColorConstants->Y = InColor.Y;
			ColorConstants->Z = InColor.Z;
			ColorConstants->W = InColor.W;
		}
		GetDeviceContext()->Unmap(ConstantBufferColor, 0);
	}
}

void URenderer::UpdateConstant(const FMaterialConstants& InMaterial) const
{
	Pipeline->SetConstantBuffer(2, false, ConstantBufferMaterial);

	if (ConstantBufferMaterial)
	{
		D3D11_MAPPED_SUBRESOURCE ConstantBufferMSR = {};

		GetDeviceContext()->Map(ConstantBufferMaterial, 0, D3D11_MAP_WRITE_DISCARD, 0, &ConstantBufferMSR);

		FMaterialConstants* MaterialConstants = static_cast<FMaterialConstants*>(ConstantBufferMSR.pData);
		{
			MaterialConstants->Ka = InMaterial.Ka;
			MaterialConstants->Kd = InMaterial.Kd;
			MaterialConstants->Ks = InMaterial.Ks;
			MaterialConstants->Ns = InMaterial.Ns;
			MaterialConstants->Ni = InMaterial.Ni;
			MaterialConstants->D = InMaterial.D;
			MaterialConstants->MaterialFlags = InMaterial.MaterialFlags;
			MaterialConstants->Time = InMaterial.Time;
		}
		GetDeviceContext()->Unmap(ConstantBufferMaterial, 0);
	}
}

bool URenderer::UpdateVertexBuffer(ID3D11Buffer* InVertexBuffer, const TArray<FVector>& InVertices) const
{
	if (!GetDeviceContext() || !InVertexBuffer || InVertices.empty())
	{
		return false;
	}

	D3D11_MAPPED_SUBRESOURCE MappedResource = {};
	HRESULT ResultHandle = GetDeviceContext()->Map(
		InVertexBuffer,
		0, // 서브리소스 인덱스 (버퍼는 0)
		D3D11_MAP_WRITE_DISCARD, // 전체 갱신
		0, // 플래그 없음
		&MappedResource
	);

	if (FAILED(ResultHandle))
	{
		return false;
	}

	// GPU 메모리에 새 데이터 복사
	// TODO: 어쩔 때 한번 read access violation 걸림
	memcpy(MappedResource.pData, InVertices.data(), sizeof(FVector) * InVertices.size());

	// GPU 접근 재허용
	GetDeviceContext()->Unmap(InVertexBuffer, 0);

	return true;
}

ID3D11RasterizerState* URenderer::GetRasterizerState(const FRenderState& InRenderState)
{
	D3D11_FILL_MODE FillMode = ToD3D11(InRenderState.FillMode);
	D3D11_CULL_MODE CullMode = ToD3D11(InRenderState.CullMode);

	const FRasterKey Key{ FillMode, CullMode };
	if (auto Iter = RasterCache.find(Key); Iter != RasterCache.end())
	{
		return Iter->second;
	}

	ID3D11RasterizerState* RasterizerState = nullptr;
	D3D11_RASTERIZER_DESC RasterizerDesc = {};
	RasterizerDesc.FillMode = FillMode;
	RasterizerDesc.CullMode = CullMode;
	RasterizerDesc.FrontCounterClockwise = TRUE;
	RasterizerDesc.DepthClipEnable = TRUE; // ✅ 근/원거리 평면 클리핑 활성화 (핵심)

	HRESULT ResultHandle = GetDevice()->CreateRasterizerState(&RasterizerDesc, &RasterizerState);

	if (FAILED(ResultHandle))
	{
		return nullptr;
	}

	RasterCache.emplace(Key, RasterizerState);
	return RasterizerState;
}

D3D11_CULL_MODE URenderer::ToD3D11(ECullMode InCull)
{
	switch (InCull)
	{
	case ECullMode::Back:
		return D3D11_CULL_BACK;
	case ECullMode::Front:
		return D3D11_CULL_FRONT;
	case ECullMode::None:
		return D3D11_CULL_NONE;
	default:
		return D3D11_CULL_BACK;
	}
}

D3D11_FILL_MODE URenderer::ToD3D11(EFillMode InFill)
{
	switch (InFill)
	{
	case EFillMode::Solid:
		return D3D11_FILL_SOLID;
	case EFillMode::WireFrame:
		return D3D11_FILL_WIREFRAME;
	default:
		return D3D11_FILL_SOLID;
	}
}

/**
 * @brief Compute Shader 생성 함수
 */
void URenderer::CreateComputeShader()
{
	// Test compute shader 컴파일
	CreateComputeShaderFromFile(L"Asset/Shader/TestComputeShader.hlsl", &TestComputeShader);

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
		UE_LOG("Depth Copy Compute Shader 컴파일 성공");
	}
	else
	{
		UE_LOG("Depth Copy Compute Shader 컴파일 실패!");
	}

	// Compute constant buffer 생성
	D3D11_BUFFER_DESC BufferDescription = {};
	BufferDescription.Usage = D3D11_USAGE_DYNAMIC;
	BufferDescription.ByteWidth = sizeof(FComputeConstants);
	BufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	BufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	GetDevice()->CreateBuffer(&BufferDescription, nullptr, &ComputeConstantBuffer);

	// HZB constant buffer 생성
	D3D11_BUFFER_DESC HZBBufferDescription = {};
	HZBBufferDescription.Usage = D3D11_USAGE_DYNAMIC;
	HZBBufferDescription.ByteWidth = sizeof(FHZBConstants);
	HZBBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	HZBBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	GetDevice()->CreateBuffer(&HZBBufferDescription, nullptr, &HZBConstantBuffer);
}

/**
 * @brief Compute Shader 해제 함수
 */
void URenderer::ReleaseComputeShader()
{
	if (TestComputeShader)
	{
		TestComputeShader->Release();
		TestComputeShader = nullptr;
	}

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

	if (ComputeConstantBuffer)
	{
		ComputeConstantBuffer->Release();
		ComputeConstantBuffer = nullptr;
	}

	if (HZBConstantBuffer)
	{
		HZBConstantBuffer->Release();
		HZBConstantBuffer = nullptr;
	}

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
}

/**
 * @brief 파일에서 Compute Shader를 로드하고 컴파일하는 함수
 */
void URenderer::CreateComputeShaderFromFile(const wstring& InFilePath, ID3D11ComputeShader** OutComputeShader) const
{
	ID3DBlob* ComputeShaderBlob = nullptr;
	ID3DBlob* ErrorBlob = nullptr;

	HRESULT hr = D3DCompileFromFile(InFilePath.c_str(), nullptr, nullptr, "CSMain", "cs_5_0", 0, 0, &ComputeShaderBlob, &ErrorBlob);

	if (FAILED(hr))
	{
		if (ErrorBlob)
		{
			UE_LOG("Compute Shader 컴파일 에러: %s", (char*)ErrorBlob->GetBufferPointer());
			ErrorBlob->Release();
		}
		return;
	}

	hr = GetDevice()->CreateComputeShader(ComputeShaderBlob->GetBufferPointer(), ComputeShaderBlob->GetBufferSize(), nullptr, OutComputeShader);

	if (FAILED(hr))
	{
		UE_LOG("Compute Shader 생성 실패");
	}

	ComputeShaderBlob->Release();
	if (ErrorBlob) ErrorBlob->Release();
}

/**
 * @brief Compute Shader 디스패치 함수
 */
void URenderer::DispatchComputeShader(ID3D11ComputeShader* InComputeShader, uint32 ThreadGroupsX, uint32 ThreadGroupsY, uint32 ThreadGroupsZ) const
{
	if (!InComputeShader) return;

	GetDeviceContext()->CSSetShader(InComputeShader, nullptr, 0);
	GetDeviceContext()->Dispatch(ThreadGroupsX, ThreadGroupsY, ThreadGroupsZ);

	// Clear compute shader after dispatch
	GetDeviceContext()->CSSetShader(nullptr, nullptr, 0);
}

/**
 * @brief Structured Buffer 생성 함수
 */
ID3D11Buffer* URenderer::CreateStructuredBuffer(uint32 InElementSize, uint32 InElementCount, const void* InInitialData) const
{
	D3D11_BUFFER_DESC BufferDesc = {};
	BufferDesc.ByteWidth = InElementSize * InElementCount;
	BufferDesc.Usage = D3D11_USAGE_DEFAULT;
	BufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	BufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	BufferDesc.StructureByteStride = InElementSize;

	D3D11_SUBRESOURCE_DATA InitData = {};
	InitData.pSysMem = InInitialData;

	ID3D11Buffer* Buffer = nullptr;
	HRESULT hr = GetDevice()->CreateBuffer(&BufferDesc, InInitialData ? &InitData : nullptr, &Buffer);

	if (FAILED(hr))
	{
		UE_LOG("Structured Buffer 생성 실패");
		return nullptr;
	}

	return Buffer;
}

/**
 * @brief Structured Buffer SRV 생성 함수
 */
ID3D11ShaderResourceView* URenderer::CreateStructuredBufferSRV(ID3D11Buffer* InBuffer, uint32 InElementCount) const
{
	if (!InBuffer) return nullptr;

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	SRVDesc.Buffer.ElementWidth = InElementCount;

	ID3D11ShaderResourceView* SRV = nullptr;
	HRESULT hr = GetDevice()->CreateShaderResourceView(InBuffer, &SRVDesc, &SRV);

	if (FAILED(hr))
	{
		UE_LOG("Structured Buffer SRV 생성 실패");
		return nullptr;
	}

	return SRV;
}

/**
 * @brief Structured Buffer UAV 생성 함수
 */
ID3D11UnorderedAccessView* URenderer::CreateStructuredBufferUAV(ID3D11Buffer* InBuffer, uint32 InElementCount) const
{
	if (!InBuffer) return nullptr;

	D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
	UAVDesc.Format = DXGI_FORMAT_UNKNOWN;
	UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	UAVDesc.Buffer.NumElements = InElementCount;

	ID3D11UnorderedAccessView* UAV = nullptr;
	HRESULT hr = GetDevice()->CreateUnorderedAccessView(InBuffer, &UAVDesc, &UAV);

	if (FAILED(hr))
	{
		UE_LOG("Structured Buffer UAV 생성 실패");
		return nullptr;
	}

	return UAV;
}

/**
 * @brief Compute Shader 테스트 실행 함수
 */
void URenderer::TestComputeShaderExecution() const
{
	if (!TestComputeShader)
	{
		UE_LOG("TestComputeShader가 초기화되지 않았습니다."); //
		return;
	}

	// 1. 테스트 데이터 생성
	const uint32 ElementCount = 1024;
	TArray<FVector4> TestData(ElementCount);

	// 초기 데이터 설정
	for (uint32 i = 0; i < ElementCount; ++i)
	{
		TestData[i] = FVector4(
			static_cast<float>(i),
			static_cast<float>(i) * 2.0f,
			static_cast<float>(i) * 3.0f,
			1.0f
		);
	}

	// 2. Structured Buffer 생성
	ID3D11Buffer* TestBuffer = CreateStructuredBuffer(sizeof(FVector4), ElementCount, TestData.data());
	if (!TestBuffer)
	{
		UE_LOG("TestBuffer 생성 실패");
		return;
	}

	// 3. UAV 생성
	ID3D11UnorderedAccessView* TestUAV = CreateStructuredBufferUAV(TestBuffer, ElementCount);
	if (!TestUAV)
	{
		UE_LOG("TestUAV 생성 실패");
		TestBuffer->Release();
		return;
	}

	// 4. Constant Buffer 업데이트
	FComputeConstants Constants = {};
	Constants.NumElements = ElementCount;
	Constants.Multiplier = 2.5f;

	D3D11_MAPPED_SUBRESOURCE MappedResource = {};
	GetDeviceContext()->Map(ComputeConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	memcpy(MappedResource.pData, &Constants, sizeof(FComputeConstants));
	GetDeviceContext()->Unmap(ComputeConstantBuffer, 0);

	// 5. Compute Shader 바인딩
	GetDeviceContext()->CSSetConstantBuffers(0, 1, &ComputeConstantBuffer);
	GetDeviceContext()->CSSetUnorderedAccessViews(0, 1, &TestUAV, nullptr);

	// 6. Dispatch - 64 threads per group이므로 (ElementCount + 63) / 64 그룹 필요
	uint32 ThreadGroups = (ElementCount + 63) / 64;
	DispatchComputeShader(TestComputeShader, ThreadGroups, 1, 1);

	// 7. UAV 언바인딩
	ID3D11UnorderedAccessView* NullUAV = nullptr;
	GetDeviceContext()->CSSetUnorderedAccessViews(0, 1, &NullUAV, nullptr);

	// 8. 결과 읽기를 위한 스테이징 버퍼 생성
	D3D11_BUFFER_DESC StagingDesc = {};
	StagingDesc.ByteWidth = sizeof(FVector4) * ElementCount;
	StagingDesc.Usage = D3D11_USAGE_STAGING;
	StagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

	ID3D11Buffer* StagingBuffer = nullptr;
	GetDevice()->CreateBuffer(&StagingDesc, nullptr, &StagingBuffer);

	if (StagingBuffer)
	{
		// 9. GPU에서 CPU로 데이터 복사
		GetDeviceContext()->CopyResource(StagingBuffer, TestBuffer);

		// 10. 결과 확인 (처음 10개 원소만)
		D3D11_MAPPED_SUBRESOURCE MappedResult = {};
		if (SUCCEEDED(GetDeviceContext()->Map(StagingBuffer, 0, D3D11_MAP_READ, 0, &MappedResult)))
		{
			FVector4* ResultData = static_cast<FVector4*>(MappedResult.pData);

			UE_LOG("=== Compute Shader 테스트 결과 ===");
			for (uint32 i = 0; i < 10 && i < ElementCount; ++i)
			{
				UE_LOG("Element[%u]: (%.2f, %.2f, %.2f, %.2f)",
					i, ResultData[i].X, ResultData[i].Y, ResultData[i].Z, ResultData[i].W);
			}
			UE_LOG("=== 테스트 완료 ===");

			GetDeviceContext()->Unmap(StagingBuffer, 0);
		}

		StagingBuffer->Release();
	}

	// 11. 리소스 정리
	TestUAV->Release();
	TestBuffer->Release();
}

/**
 * @brief HZB 텍스처와 리소스 초기화 함수
 */
void URenderer::InitializeHZB()
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
	depthCopyDesc.Format = DXGI_FORMAT_R32_FLOAT; // 단순한 float 포맷 사용
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
		textureDesc.Format = DXGI_FORMAT_R32_FLOAT; // Single channel float for depth
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Usage = D3D11_USAGE_DEFAULT;
		textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		textureDesc.CPUAccessFlags = 0;
		textureDesc.MiscFlags = 0;

		HRESULT hr = GetDevice()->CreateTexture2D(&textureDesc, nullptr, &HZBTextures[mipLevel]);
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

/**
 * @brief HZB 리소스만 해제하는 함수 (컴퓨트 쉐이더는 유지)
 */
void URenderer::ReleaseHZBResources()
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

/**
 * @brief 깊이 버퍼에서 HZB 생성하는 함수
 */
void URenderer::GenerateHZB() const
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



/**
 * @brief HZB에서 특정 좌표의 깊이값을 샘플링하는 함수
 * @param mipLevel 샘플링할 mip level (0~3)
 * @param x X 좌표 (mip level에 맞는 좌표계)
 * @param y Y 좌표 (mip level에 맞는 좌표계)
 * @return 해당 위치의 깊이값 (0.0~1.0), 실패시 -1.0
 */
float URenderer::SampleHZBDepth(uint32 mipLevel, uint32 x, uint32 y) const
{
	if (mipLevel >= 4 || !HZBTextures[mipLevel])
	{
		return -1.0f;
	}

	// 스테이징 텍스처 생성 (1x1 픽셀)
	D3D11_TEXTURE2D_DESC stagingDesc = {};
	stagingDesc.Width = 1;
	stagingDesc.Height = 1;
	stagingDesc.MipLevels = 1;
	stagingDesc.ArraySize = 1;
	stagingDesc.Format = DXGI_FORMAT_R32_FLOAT;
	stagingDesc.SampleDesc.Count = 1;
	stagingDesc.SampleDesc.Quality = 0;
	stagingDesc.Usage = D3D11_USAGE_STAGING;
	stagingDesc.BindFlags = 0;
	stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	stagingDesc.MiscFlags = 0;

	ID3D11Texture2D* stagingTexture = nullptr;
	HRESULT hr = GetDevice()->CreateTexture2D(&stagingDesc, nullptr, &stagingTexture);
	if (FAILED(hr))
	{
		return -1.0f;
	}

	// 특정 픽셀만 복사
	D3D11_BOX sourceBox = {};
	sourceBox.left = x;
	sourceBox.top = y;
	sourceBox.front = 0;
	sourceBox.right = x + 1;
	sourceBox.bottom = y + 1;
	sourceBox.back = 1;

	GetDeviceContext()->CopySubresourceRegion(
		stagingTexture, 0, 0, 0, 0,
		HZBTextures[mipLevel], 0, &sourceBox
	);

	// CPU에서 데이터 읽기
	D3D11_MAPPED_SUBRESOURCE mappedResource = {};
	hr = GetDeviceContext()->Map(stagingTexture, 0, D3D11_MAP_READ, 0, &mappedResource);

	float result = -1.0f;
	if (SUCCEEDED(hr))
	{
		float* depthData = static_cast<float*>(mappedResource.pData);
		result = depthData[0];
		GetDeviceContext()->Unmap(stagingTexture, 0);
	}

	stagingTexture->Release();
	return result;
}

/**
 * @brief HZB의 특정 mip level에서 영역의 최대 깊이값을 찾는 함수
 * @param mipLevel 검사할 mip level (0~3)
 * @param startX 시작 X 좌표
 * @param startY 시작 Y 좌표
 * @param width 영역 너비
 * @param height 영역 높이
 * @return 영역 내 최대 깊이값 (0.0~1.0), 실패시 -1.0
 */
float URenderer::SampleHZBRegionMaxDepth(uint32 mipLevel, uint32 startX, uint32 startY, uint32 width, uint32 height) const
{
	if (mipLevel >= 4 || !HZBTextures[mipLevel] || width == 0 || height == 0)
	{
		return -1.0f;
	}

	// 스테이징 텍스처 생성
	D3D11_TEXTURE2D_DESC stagingDesc = {};
	stagingDesc.Width = width;
	stagingDesc.Height = height;
	stagingDesc.MipLevels = 1;
	stagingDesc.ArraySize = 1;
	stagingDesc.Format = DXGI_FORMAT_R32_FLOAT;
	stagingDesc.SampleDesc.Count = 1;
	stagingDesc.SampleDesc.Quality = 0;
	stagingDesc.Usage = D3D11_USAGE_STAGING;
	stagingDesc.BindFlags = 0;
	stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	stagingDesc.MiscFlags = 0;

	ID3D11Texture2D* stagingTexture = nullptr;
	HRESULT hr = GetDevice()->CreateTexture2D(&stagingDesc, nullptr, &stagingTexture);
	if (FAILED(hr))
	{
		return -1.0f;
	}

	// 특정 영역 복사
	D3D11_BOX sourceBox = {};
	sourceBox.left = startX;
	sourceBox.top = startY;
	sourceBox.front = 0;
	sourceBox.right = startX + width;
	sourceBox.bottom = startY + height;
	sourceBox.back = 1;

	GetDeviceContext()->CopySubresourceRegion(
		stagingTexture, 0, 0, 0, 0,
		HZBTextures[mipLevel], 0, &sourceBox
	);

	// CPU에서 데이터 읽기
	D3D11_MAPPED_SUBRESOURCE mappedResource = {};
	hr = GetDeviceContext()->Map(stagingTexture, 0, D3D11_MAP_READ, 0, &mappedResource);

	float maxDepth = -1.0f;
	if (SUCCEEDED(hr))
	{
		float* depthData = static_cast<float*>(mappedResource.pData);
		maxDepth = 0.0f;

		for (uint32 y = 0; y < height; ++y)
		{
			for (uint32 x = 0; x < width; ++x)
			{
				uint32 index = y * (mappedResource.RowPitch / sizeof(float)) + x;
				maxDepth = std::max(maxDepth, depthData[index]);
			}
		}

		GetDeviceContext()->Unmap(stagingTexture, 0);
	}

	stagingTexture->Release();
	return maxDepth;
}

/**
 * @brief SIMD 벡터에서 수평 최대값을 계산하는 헬퍼 함수
 * @param v 4개 float 값을 담은 __m128 벡터
 * @return 벡터 내 4개 값 중 최대값
 */
__forceinline float HorizontalMax(__m128 v)
{
	// [x, y, z, w] -> [z, w, x, y]
	__m128 temp = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 0, 3, 2));
	// 각 쌍의 최대값: [max(x,z), max(y,w), max(z,x), max(w,y)]
	__m128 max1 = _mm_max_ps(v, temp);
	// [max(x,z), max(y,w), max(z,x), max(w,y)] -> [max(y,w), max(x,z), max(w,y), max(z,x)]
	temp = _mm_shuffle_ps(max1, max1, _MM_SHUFFLE(2, 3, 0, 1));
	// 최종 최대값: [max(max(x,z),max(y,w)), ...]
	__m128 max2 = _mm_max_ps(max1, temp);
	// 첫 번째 요소 추출
	return _mm_cvtss_f32(max2);
}

/**
 * @brief 캐시된 HZB 데이터에서 빠른 샘플링 함수 (CPU 기반)
 * @param screenX 스크린 공간 X 좌표 (0~화면너비)
 * @param screenY 스크린 공간 Y 좌표 (0~화면높이)
 * @param width 샘플링할 영역의 너비
 * @param height 샘플링할 영역의 높이
 * @return 해당 영역의 최대 깊이값 (0.0~1.0), 실패시 -1.0
 */
float URenderer::SampleHZBRect(float screenX, float screenY, float width, float height) const
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
			// 사전 계산된 범위 내에서는 경계 검사 불필요
			__m128 depthVec = _mm_loadu_ps(&CachedHZBLevel3[index]);
			maxVec = _mm_max_ps(maxVec, depthVec);
		}
		
		// 나머지 처리 (rowStart 재사용)
		for (; x < endX; ++x)
		{
			uint32 index = rowStart + x;
			maxDepth = std::max(maxDepth, CachedHZBLevel3[index]);
		}
	}
	
	// SIMD 결과에서 수평 최대값 추출 (최적화된 방법)
	float simdMax = HorizontalMax(maxVec);
	maxDepth = std::max(maxDepth, simdMax);

	return maxDepth;
}

/**
 * @brief 월드 공간 AABB를 스크린 공간으로 투영하는 함수
 * @param worldMin AABB 최소 좌표 (월드 공간)
 * @param worldMax AABB 최대 좌표 (월드 공간)
 * @param camera 현재 카메라
 * @param outScreenX 출력: 스크린 공간 X 좌표
 * @param outScreenY 출력: 스크린 공간 Y 좌표
 * @param outWidth 출력: 스크린 공간 너비
 * @param outHeight 출력: 스크린 공간 높이
 * @param outMinDepth 출력: 최소 깊이값 (0.0~1.0)
 * @return 투영 성공 여부
 */
bool URenderer::ProjectAABBToScreen(const FVector& worldMin, const FVector& worldMax, const UCamera* camera,
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

/**
 * @brief 객체가 가려져 있는지 확인하는 함수
 * @param worldMin AABB 최소 좌표 (월드 공간)
 * @param worldMax AABB 최대 좌표 (월드 공간)
 * @param camera 현재 카메라
 * @return true면 가려져 있음 (렌더링 불필요), false면 보임 (렌더링 필요)
 */
bool URenderer::IsOccluded(const FVector& worldMin, const FVector& worldMax, const UCamera* camera) const
{
	return IsOccluded(worldMin, worldMax, camera, 3.0f);  // 기본 3배 확장
}

bool URenderer::IsOccluded(const FVector& worldMin, const FVector& worldMax, const UCamera* camera, float expansionFactor) const
{
	// 카메라와 AABB 사이의 최단 거리 계산 - 물체 크기를 고려한 거리 체크
	FVector center = (worldMin + worldMax) * 0.5f;
	FVector cameraPos = camera->GetLocation();

	// Clamp 함수 정의
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
	if (distanceToAABB < 10.0f)
	{
		return false;
	}

	// 보수적 컬링을 위해 AABB를 확대 (더 큰 bounding box로 테스트)
	// 이렇게 하면 더 큰 영역에서 테스트하므로 false negative 감소 (덜 적극적인 컬링)
	FVector size = (worldMax - worldMin) * expansionFactor;  // 지정된 배수로 확대
	FVector conservativeMin = center - size * 0.5f;
	FVector conservativeMax = center + size * 0.5f;

	float screenX, screenY, width, height, minDepth;


	// 확대된 AABB를 스크린 공간으로 투영
	if (!ProjectAABBToScreen(conservativeMin, conservativeMax, camera, screenX, screenY, width, height, minDepth))
	{
		return true;  // 투영 실패 = 화면 밖에 있음 = 컬링
	}

	// 화면 영역이 너무 작으면 컬링하지 않음 (적당히 보수적으로)
	//if (width < 10.0f || height < 10.0f)
	//{
	//	return false;  // 매우 작은 객체는 가려짐 테스트 생략
	//}

	// HZB에서 해당 영역의 최대 깊이 샘플링
	float hzbMaxDepth = SampleHZBRect(screenX, screenY, width, height);

	if (hzbMaxDepth < 0.0f)
	{
		return false;  // HZB 샘플링 실패 = 안전하게 보이는 것으로 처리
	}

	// Occlusion 테스트: HZB 최대 깊이 < 객체 최소 깊이 → 완전히 가려짐
	// 적당한 bias로 false negative 방지하면서 컬링 효율성 유지
	// 정규화된 깊이 비교 (부동소수점 정밀도 문제 해결)
	// 깊이 범위에 비례한 상대적 bias 사용

	// Multi-scale bias: 깊이 구간별로 최적화된 bias 적용
	// 일반적인 상황에서는 0.0003이 잘 작동하지만 깊이별로 세분화

	// 3프레임 연속 시스템과 함께 사용할 적당한 bias (안정적)
	float depthBias = 0.0005f;

	//// 깊이 구간별 최적화된 bias 적용
	//if (minDepth < 0.93f) {
	//	depthBias = 0.0005f;    // 근거리: 매우 정밀한 bias (높은 정밀도 구간)
	//}
	//else if (minDepth < 0.95f) {
	//	depthBias = 0.0007f;     // 근-중거리: 작은 bias
	//}
	//else if (minDepth < 0.965f) {
	//	depthBias = 0.001f;    // 근거리: 매우 정밀한 bias (높은 정밀도 구간)
	//}
	//else if (minDepth < 0.98f) {
	//	depthBias = 0.0015f;     // 근-중거리: 작은 bias
	//}
	//else if (minDepth < 0.99f) {
	//	depthBias = 0.002f;    // 근거리: 매우 정밀한 bias (높은 정밀도 구간)
	//}
	//else if (minDepth < 0.995f) {
	//	depthBias = 0.003f;     // 근-중거리: 작은 bias
	//}
	//else
	//{
	//	depthBias = 0.005f;     // 근-중거리: 작은 bias
	//}

	bool isOccluded = (hzbMaxDepth + depthBias) < minDepth;

	// 시점별 깊이 분포 및 카메라 속도 디버깅
	static int totalTests = 0;
	static int occludedCount = 0;
	static float minHZBDepth = FLT_MAX;
	static float maxHZBDepth = -FLT_MAX;
	static float minObjDepth = FLT_MAX;
	static float maxObjDepth = -FLT_MAX;
	totalTests++;
	if (isOccluded) occludedCount++;

	// 깊이 범위 추적
	if (hzbMaxDepth >= 0.0f)
	{
		minHZBDepth = std::min(minHZBDepth, hzbMaxDepth);
		maxHZBDepth = std::max(maxHZBDepth, hzbMaxDepth);
	}
	minObjDepth = std::min(minObjDepth, minDepth);
	maxObjDepth = std::max(maxObjDepth, minDepth);

	return isOccluded;
}

/**
 * @brief HZB mipLevel 3를 CPU 메모리로 캐시하는 함수
 */
void URenderer::CacheHZBForOcclusion() const
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

	// 캐시 배열 크기 조정
	CachedHZBLevel3.resize(CachedHZBWidth * CachedHZBHeight);

	// 스테이징 텍스처 재사용 최적화
	ID3D11Texture2D* stagingTexture = nullptr;
	HRESULT hr = S_OK;

	// 기존 스테이징 텍스처가 크기가 맞으면 재사용
	if (CachedStagingTexture &&
		CachedStagingWidth == CachedHZBWidth &&
		CachedStagingHeight == CachedHZBHeight)
	{
		stagingTexture = CachedStagingTexture;
	}
	else
	{
		// 기존 텍스처 해제
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

		hr = GetDevice()->CreateTexture2D(&stagingDesc, nullptr, &stagingTexture);
		if (FAILED(hr))
		{
			bHZBCacheValid = false;
			return;
		}

		// 캐시 업데이트
		CachedStagingTexture = stagingTexture;
		CachedStagingWidth = CachedHZBWidth;
		CachedStagingHeight = CachedHZBHeight;
	}

	// 전체 mipLevel 3 텍스처를 스테이징으로 복사
	GetDeviceContext()->CopyResource(stagingTexture, HZBTextures[3]);

	// CPU에서 데이터 읽기
	D3D11_MAPPED_SUBRESOURCE mappedResource = {};
	hr = GetDeviceContext()->Map(stagingTexture, 0, D3D11_MAP_READ, 0, &mappedResource);

	if (SUCCEEDED(hr))
	{
		float* sourceData = static_cast<float*>(mappedResource.pData);
		uint32 sourceRowPitch = mappedResource.RowPitch / sizeof(float);

		// 최적화된 메모리 복사
		if (sourceRowPitch == CachedHZBWidth)
		{
			// RowPitch가 정확히 맞으면 한 번에 복사
			memcpy(CachedHZBLevel3.data(), sourceData, CachedHZBWidth * CachedHZBHeight * sizeof(float));
		}
		else
		{
			// RowPitch가 다르면 행별로 memcpy
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

	// 스테이징 텍스처는 캐시되므로 여기서 해제하지 않음
	// CachedStagingTexture가 캐시에 보관되어 다음 프레임에서 재사용됨
}

/**
 * @brief 캐시된 HZB 데이터 해제 함수
 */
void URenderer::ReleaseCachedHZB() const
{
	CachedHZBLevel3.clear();
	bHZBCacheValid = false;
	CachedHZBWidth = 0;
	CachedHZBHeight = 0;

	// 캐시된 스테이징 텍스처도 해제
	if (CachedStagingTexture)
	{
		CachedStagingTexture->Release();
		CachedStagingTexture = nullptr;
	}
	CachedStagingWidth = 0;
	CachedStagingHeight = 0;
}

__forceinline void URenderer::UpdateLODFast(UStaticMeshComponent* MeshComp, const FVector& CameraPos) const
{
	// 거리 제곱 계산 (매우 빠름)
	const FVector ObjPos = MeshComp->GetRelativeLocation();
	const float dx = CameraPos.X - ObjPos.X;
	const float dy = CameraPos.Y - ObjPos.Y;
	const float dz = CameraPos.Z - ObjPos.Z;
	const float DistSq = dx * dx + dy * dy + dz * dz;

	// Config 기반 LOD 선택
	int NewLOD;
	if (!bLODEnabled)
	{
		NewLOD = 0;  // LOD 비활성화 시 최고 품질
	}
	else if (DistSq > LODDistanceSquared1)
		NewLOD = 2;
	else if (DistSq > LODDistanceSquared0)
		NewLOD = 1;
	else
		NewLOD = 0;

	// LOD 변경이 필요할 때만 업데이트
	if (MeshComp->GetCurrentLODIndex() != NewLOD)
	{
		MeshComp->SetCurrentLODIndex(NewLOD);

		// 통계 업데이트
		LODStats.LODUpdatesPerFrame++;
		switch (NewLOD)
		{
		case 0: LODStats.LOD0Count++; break;
		case 1: LODStats.LOD1Count++; break;
		case 2: LODStats.LOD2Count++; break;
		}
	}
}

void URenderer::LoadLODSettings()
{
	// Config Manager를 통해 LOD 설정 로드
	UConfigManager& ConfigManager = UConfigManager::GetInstance();

	// LOD 활성화 설정
	bLODEnabled = ConfigManager.GetConfigValueBool("LODEnabled", true);

	// LOD 거리 설정 (일반 거리 값을 제곱해서 저장)
	float LODDistance0 = ConfigManager.GetConfigValueFloat("LODDistance0", 10.0f);
	float LODDistance1 = ConfigManager.GetConfigValueFloat("LODDistance1", 80.0f);

	LODDistanceSquared0 = LODDistance0 * LODDistance0;
	LODDistanceSquared1 = LODDistance1 * LODDistance1;

	UE_LOG("LOD Settings Loaded - Enabled: %s, Distance0: %.1f (%.0f), Distance1: %.1f (%.0f)",
		bLODEnabled ? "true" : "false",
		LODDistance0, LODDistanceSquared0,
		LODDistance1, LODDistanceSquared1);
}
