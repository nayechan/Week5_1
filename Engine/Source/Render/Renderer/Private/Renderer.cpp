#include "pch.h"
#include "Render/Renderer/Public/Renderer.h"
#include <chrono>
#include "Render/Renderer/Public/Pipeline.h"
#include "Render/Renderer/Public/RenderCommand.h"
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
#include "Render/Spatial/Public/CullingStrategy.h"
#include "Utility/Public/ScopeCycleCounter.h"
#include "Render/Culling/Public/CullingManager.h"
#include "Render/Culling/Public/LODManager.h"
#include "Render/Culling/Public/OcclusionCuller.h"
#include <immintrin.h>
#include "Utility/Public/RadixSort.h" 

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

	// Culling Manager 초기화
	CullingManager = &UCullingManager::GetInstance();
	CullingManager->Initialize(DeviceResources);

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
}

void URenderer::Release()
{
	// Culling Manager 해제
	if (CullingManager)
	{
		CullingManager->Release();
	}

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
	if (CullingManager && CullingManager->GetOcclusionCuller())
	{
		CullingManager->GetOcclusionCuller()->GenerateHZB();
		// HZB 생성 직후 Occlusion Culling용 캐시 생성
		CullingManager->GetOcclusionCuller()->CacheHZBForOcclusion();
	}


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
 * @brief Buffer에 데이터 입력 및 Draw (리팩토링: 상태 변경 최소화)
 */
void URenderer::RenderLevel(UCamera* InCurrentCamera)
{
    // Level 없으면 Early Return
    ULevel* CurrentLevel = ULevelManager::GetInstance().GetCurrentLevel();
    if (!CurrentLevel)
        return;

	// Get view mode from editor
    const EViewModeIndex ViewMode = ULevelManager::GetInstance().GetEditor()->GetViewMode();

	// Frustum Culling Strategy 생성
	FFrustum ViewFrustum = InCurrentCamera->GetViewFrustum();
	UOcclusionCuller* OcclusionCuller = CullingManager ? CullingManager->GetOcclusionCuller() : nullptr;
	bool bHZBValid = OcclusionCuller && OcclusionCuller->IsHZBCacheValid();

	FFrustumWithOcclusionStrategy CullingStrategy(
		ViewFrustum,
		bHZBValid ? OcclusionCuller : nullptr,
		InCurrentCamera,
		2.5f  // Expansion factor
	);

	// Step 1: 가시적인 Primitive들을 수집 (캐시 친화적 순회)
	TArray<FRenderCommand> RenderCommands;

	// Octree + Dynamic Primitives 전체 크기로 미리 할당 (재할당 방지)
	// 50000개 * 16바이트 = 800KB로 메모리 부담 없음
	const uint32 TotalObjectCount = CurrentLevel->GetStaticOctree().GetObjectCount() +
	                                  static_cast<uint32>(CurrentLevel->GetDynamicPrimitives().size());
	RenderCommands.reserve(TotalObjectCount);

	{
		FScopeCycleCounter Counter(GetCullingStatId());

		// Static Primitives 수집
		CurrentLevel->GetStaticOctree().TraverseVisible(CullingStrategy,
			[&](UPrimitiveComponent* Prim)
			{
				if (Prim && Prim->IsVisible())
				{
					// LOD 업데이트 (StaticMesh인 경우에만, 6프레임마다)
					static int lodFrameCounter = 0;
					if (Prim->GetPrimitiveType() == EPrimitiveType::StaticMesh && (++lodFrameCounter % 6) == 0)
					{
						if (auto* MeshComp = Cast<UStaticMeshComponent>(Prim))
						{
							if (CullingManager && CullingManager->GetLODManager())
							{
								CullingManager->GetLODManager()->UpdateLOD(MeshComp, InCurrentCamera->GetLocation());
							}
						}
					}

					RenderCommands.push_back({Prim, FRenderCommand::BuildSortKey(Prim)});
				}
			});

		// Dynamic Primitives 수집
		const auto& DynamicPrimitives = CurrentLevel->GetDynamicPrimitives();
		for (UPrimitiveComponent* DynPrim : DynamicPrimitives)
		{
			if (!DynPrim || !DynPrim->IsVisible()) continue;

			FVector WorldMin, WorldMax;
			DynPrim->GetWorldAABB(WorldMin, WorldMax);
			if (ViewFrustum.IsBoxInFrustum(FAABB(WorldMin, WorldMax)))
			{
				RenderCommands.push_back({DynPrim, FRenderCommand::BuildSortKey(DynPrim)});
			}
		}
	}

	// Step 2: Material/Shader 기준으로 정렬 (상태 변경 최소화)
	// SIMD 최적화된 Radix Sort 사용 (O(n) vs std::sort의 O(n log n))
	RadixSortUtil::RadixSort64Direct(RenderCommands);

	// Step 3: Batch 렌더링 (상태 변경 최소화)
	RenderBatched(RenderCommands, InCurrentCamera, ViewMode);
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

/**
 * @brief Batch 렌더링 (상태 변경 최소화)
 * @param Commands 정렬된 렌더 커맨드 배열
 * @param InCamera 현재 카메라
 * @param ViewMode 뷰 모드 (Wireframe 등)
 */
void URenderer::RenderBatched(const TArray<FRenderCommand>& Commands, UCamera* InCamera, EViewModeIndex ViewMode)
{
	if (Commands.empty())
		return;

	uint64 CurrentSortKey = ~0ULL;  // 초기값: 항상 다른 값

	for (const FRenderCommand& Cmd : Commands)
	{
		UPrimitiveComponent* Primitive = Cmd.Primitive;
		if (!Primitive)
			continue;

		// SortKey가 변경되면 상태 업데이트 필요
		bool bNeedStateChange = (Cmd.SortKey != CurrentSortKey);

		// 렌더 상태 설정
		FRenderState RenderState = Primitive->GetRenderState();
		if (ViewMode == EViewModeIndex::VMI_Wireframe)
		{
			RenderState.CullMode = ECullMode::None;
			RenderState.FillMode = EFillMode::WireFrame;
		}

		ID3D11RasterizerState* RasterizerState = GetRasterizerState(RenderState);

		// Primitive 타입에 따라 렌더링
		switch (Primitive->GetPrimitiveType())
		{
		case EPrimitiveType::StaticMesh:
		{
			UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(Primitive);
			if (MeshComp)
			{
				RenderStaticMesh(MeshComp, RasterizerState);
			}
			break;
		}
		default:
			RenderPrimitiveDefault(Primitive, RasterizerState);
			break;
		}

		CurrentSortKey = Cmd.SortKey;
	}
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
	if (CullingManager)
	{
		CullingManager->OnResize();
	}
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

	// Compute constant buffer 생성
	D3D11_BUFFER_DESC BufferDescription = {};
	BufferDescription.Usage = D3D11_USAGE_DYNAMIC;
	BufferDescription.ByteWidth = sizeof(FComputeConstants);
	BufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	BufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	GetDevice()->CreateBuffer(&BufferDescription, nullptr, &ComputeConstantBuffer);
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

	if (ComputeConstantBuffer)
	{
		ComputeConstantBuffer->Release();
		ComputeConstantBuffer = nullptr;
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

