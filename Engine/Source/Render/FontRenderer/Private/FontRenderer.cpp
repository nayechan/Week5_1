#include "pch.h"
#include "Render/FontRenderer/Public/FontRenderer.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Manager/Asset/Public/AssetManager.h"

UFontRenderer::UFontRenderer()
{
}

UFontRenderer::~UFontRenderer()
{
    Release();
}

/// @brief 폰트 렌더러 초기화
/// 셰이더 컴파일, 텍스처 로드, 버퍼 생성 등을 수행
bool UFontRenderer::Initialize()
{
    // 렌더러에서 Device와 DeviceContext 가져오기
    URenderer& Renderer = URenderer::GetInstance();
    ID3D11Device* Device = Renderer.GetDevice();
    ID3D11DeviceContext* DeviceContext = Renderer.GetDeviceContext();

    if (!Device || !DeviceContext)
    {
        UE_LOG_ERROR("FontRenderer: Device 또는 DeviceContext가 null입니다");
        return false;
    }

    // 셰이더 컴파일 및 생성
    if (!CreateShaders())
    {
        UE_LOG_ERROR("FontRenderer: 셰이더 생성 실패");
        return false;
    }

    // 폰트 아틀라스 텍스처 로드
    if (!LoadFontTexture())
    {
        UE_LOG_ERROR("FontRenderer: 텍스처 로드 실패");
        return false;
    }

    // 샘플러 스테이트 생성
    if (!CreateSamplerState())
    {
        UE_LOG_ERROR("FontRenderer: 샘플러 생성 실패");
        return false;
    }

    // 상수 버퍼 생성
    if (!CreateConstantBuffer())
    {
        UE_LOG_ERROR("FontRenderer: 상수 버퍼 생성 실패");
        return false;
    }

    // 간단한 테스트로 "HI" 문자만 렌더링하여 테스트
    if (!CreateVertexBufferForText("HI", -30.0f, 0.0f, 30.0f, 30.0f))
    {
        UE_LOG_ERROR("FontRenderer: 정점 버퍼 생성 실패");
        return false;
    }

    // FontRenderer 초기화 완료
    return true;
}

/// @brief 리소스 해제
void UFontRenderer::Release()
{
    // 버퍼 해제
    if (FontVertexBuffer)
    {
        FontVertexBuffer->Release();
        FontVertexBuffer = nullptr;
    }

    if (FontConstantBuffer)
    {
        FontConstantBuffer->Release();
        FontConstantBuffer = nullptr;
    }

    // 텍스처 및 샘플러 해제
    // if (FontAtlasTexture)
    // {
    //     FontAtlasTexture->Release();
    //     FontAtlasTexture = nullptr;
    // }

    if (FontSampler)
    {
        FontSampler->Release();
        FontSampler = nullptr;
    }

    // 셰이더 해제
    if (FontVertexShader)
    {
        FontVertexShader->Release();
        FontVertexShader = nullptr;
    }

    if (FontPixelShader)
    {
        FontPixelShader->Release();
        FontPixelShader = nullptr;
    }

    if (FontInputLayout)
    {
        FontInputLayout->Release();
        FontInputLayout = nullptr;
    }
}

/// @brief Hello World 렌더링 (호환성을 위해 유지)
/// @param WorldMatrix 월드 변환 행렬
/// @param ViewProjectionMatrix 뷰-프로젝션 변환 행렬
//void UFontRenderer::RenderHelloWorld(const FMatrix& WorldMatrix, const FMatrix& ViewProjectionMatrix)
//{
//    // "Hello, World!" 텍스트 렌더링
//    RenderText("Hello, World!", WorldMatrix, ViewProjectionMatrix);
//}

/// @brief 임의의 텍스트 렌더링
void UFontRenderer::RenderText(const char* Text, const FMatrix& WorldMatrix, const FViewProjConstants& ViewProjectionCostants,
	float CenterY, float StartZ, float CharWidth, float CharHeight)
{
    if (!Text || strlen(Text) == 0)
    {
        UE_LOG_WARNING("FontRenderer: 빈 텍스트 시도");
        return;
    }

    // Renderer에서 Device와 DeviceContext 가져오기
    URenderer& Renderer = URenderer::GetInstance();
    ID3D11Device* Device = Renderer.GetDevice();
    ID3D11DeviceContext* DeviceContext = Renderer.GetDeviceContext();

    if (!Device || !DeviceContext || !FontVertexShader || !FontPixelShader ||
        !FontInputLayout || !FontAtlasTexture)
    {
        UE_LOG_ERROR("FontRenderer: 렌더링에 필요한 리소스가 null - Device:%p, Context:%p, VS:%p, PS:%p, Layout:%p, Tex:%p",
            Device, DeviceContext, FontVertexShader, FontPixelShader, FontInputLayout, FontAtlasTexture);
        return;
    }

    // 텍스트를 위한 새 정점 버퍼 생성
    ID3D11Buffer* tempVertexBuffer = nullptr;
    uint32 tempVertexCount = 0;

    // 텍스트 길이 및 정점 개수 계산
    size_t textLength = strlen(Text);
    tempVertexCount = static_cast<uint32>(textLength * 6); // 각 문자당 6개 정점

    // 정점 데이터 생성
    TArray<FFontVertex> vertices;
    vertices.reserve(tempVertexCount);

	float StartY = CenterY - (textLength * CharWidth) / 2.0f;

    // 각 문자에 대해 쿼드 생성
    for (size_t i = 0; i < textLength; ++i)
    {
        char ch = Text[i];
        uint32 asciiCode = static_cast<uint32>(ch);

        // 간단한 쿼드 UV 좌표 (0~1 범위)
        FVector2 uv_topLeft(0.0f, 0.0f);
        FVector2 uv_topRight(1.0f, 0.0f);
        FVector2 uv_bottomLeft(0.0f, 1.0f);
        FVector2 uv_bottomRight(1.0f, 1.0f);

        // 문자의 월드 좌표 계산
        float y = StartY + i * CharWidth;
        float z = StartZ;

        // 쿼드의 4개 모서리 좌표
        /*FVector topLeft(x, y + CharHeight, 0.0f);
        FVector topRight(x + CharWidth, y + CharHeight, 0.0f);
        FVector bottomLeft(x, y, 0.0f);
        FVector bottomRight(x + CharWidth, y, 0.0f);*/

		FVector topLeft(0.0f, y, z + CharHeight);
		FVector topRight(0.0f, y + CharWidth, z + CharHeight);
		FVector bottomLeft(0.0f, y, z);
		FVector bottomRight(0.0f, y + CharWidth, z);

        // 첫 번째 삼각형 (왼쪽 위, 오른쪽 위, 왼쪽 아래)
        FFontVertex vertex1 = { topLeft, uv_topLeft, asciiCode };
        FFontVertex vertex2 = { topRight, uv_topRight, asciiCode };
        FFontVertex vertex3 = { bottomLeft, uv_bottomLeft, asciiCode };
        vertices.push_back(vertex1);
        vertices.push_back(vertex2);
        vertices.push_back(vertex3);

        // 두 번째 삼각형 (오른쪽 위, 오른쪽 아래, 왼쪽 아래)
        FFontVertex vertex4 = { topRight, uv_topRight, asciiCode };
        FFontVertex vertex5 = { bottomRight, uv_bottomRight, asciiCode };
        FFontVertex vertex6 = { bottomLeft, uv_bottomLeft, asciiCode };
        vertices.push_back(vertex4);
        vertices.push_back(vertex5);
        vertices.push_back(vertex6);
    }

    // 임시 정점 버퍼 생성
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(FFontVertex) * tempVertexCount;
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = vertices.data();

    HRESULT hr = Device->CreateBuffer(&bufferDesc, &initData, &tempVertexBuffer);
    if (FAILED(hr))
    {
        UE_LOG_ERROR("FontRenderer: 임시 정점 버퍼 생성 실패 (HRESULT: 0x%08lX)", hr);
        return;
    }

    // 1. 상수 버퍼 업데이트 (월드 행렬과 뷰-프로젝션 행렬)
    struct WorldMatrixBuffer
    {
        FMatrix World;
    } worldBuffer;
    worldBuffer.World = WorldMatrix;

    /*struct ViewProjectionBuffer
    {
        FMatrix ViewProjection;
    } viewProjBuffer;
    viewProjBuffer.ViewProjection = ViewProjectionMatrix;*/

    // ViewProjection 상수 버퍼 생성 (슬롯 1)
    ID3D11Buffer* viewProjConstantBuffer = nullptr;
    D3D11_BUFFER_DESC viewProjBufferDesc = {};
    viewProjBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    viewProjBufferDesc.ByteWidth = sizeof(ViewProjectionCostants);
    viewProjBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    viewProjBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    if (SUCCEEDED(Device->CreateBuffer(&viewProjBufferDesc, nullptr, &viewProjConstantBuffer)))
    {
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        if (SUCCEEDED(DeviceContext->Map(viewProjConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
        {
            memcpy(mappedResource.pData, &ViewProjectionCostants, sizeof(ViewProjectionCostants));
            DeviceContext->Unmap(viewProjConstantBuffer, 0);
        }
    }

    // 월드 매트릭스 상수 버퍼 업데이트 (슬롯 0)
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    if (SUCCEEDED(DeviceContext->Map(FontConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
    {
        memcpy(mappedResource.pData, &worldBuffer, sizeof(worldBuffer));
        DeviceContext->Unmap(FontConstantBuffer, 0);
    }

    // 폰트 데이터 상수 버퍼 생성 (슬롯 2)
    ID3D11Buffer* fontDataBuffer = nullptr;
    D3D11_BUFFER_DESC fontBufferDesc = {};
    fontBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    fontBufferDesc.ByteWidth = sizeof(FFontConstantBuffer);
    fontBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    fontBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    if (SUCCEEDED(Device->CreateBuffer(&fontBufferDesc, nullptr, &fontDataBuffer)))
    {
        if (SUCCEEDED(DeviceContext->Map(fontDataBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
        {
            memcpy(mappedResource.pData, &ConstantBufferData, sizeof(ConstantBufferData));
            DeviceContext->Unmap(fontDataBuffer, 0);
        }
    }

    // 2. 알파 블렌딩 활성화
    ID3D11BlendState* prevBlendState = nullptr;
    FLOAT prevBlendFactor[4];
    UINT prevSampleMask;
    DeviceContext->OMGetBlendState(&prevBlendState, prevBlendFactor, &prevSampleMask);

    ID3D11BlendState* alphaBlendState = nullptr;
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    if (SUCCEEDED(Device->CreateBlendState(&blendDesc, &alphaBlendState)))
    {
        FLOAT blendFactor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        DeviceContext->OMSetBlendState(alphaBlendState, blendFactor, 0xFFFFFFFF);
    }

    // 3. 렌더링 파이프라인 설정
    DeviceContext->IASetInputLayout(FontInputLayout);

    UINT stride = sizeof(FFontVertex);
    UINT offset = 0;
    DeviceContext->IASetVertexBuffers(0, 1, &tempVertexBuffer, &stride, &offset);

    DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	D3D11_RASTERIZER_DESC rasterDesc = {};
	rasterDesc.FillMode = D3D11_FILL_SOLID;   // ← 와이어프레임 대신 Solid
	rasterDesc.CullMode = D3D11_CULL_BACK;    // 보통은 Back-face culling
	rasterDesc.FrontCounterClockwise = FALSE;
	rasterDesc.DepthClipEnable = TRUE;

	ID3D11RasterizerState* solidState = nullptr;
	Device->CreateRasterizerState(&rasterDesc, &solidState);
	DeviceContext->RSSetState(solidState);

	D3D11_DEPTH_STENCIL_DESC dsDesc = {};
	dsDesc.DepthEnable = FALSE;   // 깊이 검사 끄기
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dsDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;

	ID3D11DepthStencilState* depthDisabledState = nullptr;
	Device->CreateDepthStencilState(&dsDesc, &depthDisabledState);
	DeviceContext->OMSetDepthStencilState(depthDisabledState, 1);



	//FRenderState RenderState = PrimitiveComponent->GetRenderState();

	//// Get view mode from editor
	//const EViewModeIndex ViewMode = ULevelManager::GetInstance().GetEditor()->GetViewMode();
	//if (ViewMode == EViewModeIndex::VMI_Wireframe)
	//{
	//	RenderState.CullMode = ECullMode::None;
	//	RenderState.FillMode = EFillMode::WireFrame;
	//}
	//ID3D11RasterizerState* LoadedRasterizerState = GetRasterizerState(RenderState);

    // 4. 셰이더 설정
    DeviceContext->VSSetShader(FontVertexShader, nullptr, 0);
    DeviceContext->PSSetShader(FontPixelShader, nullptr, 0);

    // 5. 상수 버퍼 바인딩
    DeviceContext->VSSetConstantBuffers(0, 1, &FontConstantBuffer);
    DeviceContext->VSSetConstantBuffers(1, 1, &viewProjConstantBuffer);
    DeviceContext->VSSetConstantBuffers(2, 1, &fontDataBuffer);

    // 6. 텍스처 및 샘플러 바인딩
    DeviceContext->PSSetShaderResources(0, 1, &FontAtlasTexture);
    DeviceContext->PSSetSamplers(0, 1, &FontSampler);

    // 7. 드로우 콜
    DeviceContext->Draw(tempVertexCount, 0);

    // 8. 리소스 정리
    DeviceContext->OMSetBlendState(prevBlendState, prevBlendFactor, prevSampleMask);
    if (prevBlendState) prevBlendState->Release();
    if (alphaBlendState) alphaBlendState->Release();
    if (tempVertexBuffer) tempVertexBuffer->Release();
    if (viewProjConstantBuffer) viewProjConstantBuffer->Release();
    if (fontDataBuffer) fontDataBuffer->Release();
	if (solidState) solidState->Release();
	if (depthDisabledState) depthDisabledState->Release();
}

/// @brief 텍스트용 정점 버퍼 생성
/// @param Text 렌더링할 텍스트
/// @param StartX 시작 X 좌표
/// @param StartY 시작 Y 좌표
/// @param CharWidth 문자 너비
/// @param CharHeight 문자 높이
bool UFontRenderer::CreateVertexBufferForText(const char* Text, float StartX, float StartY, float CharWidth, float CharHeight)
{
    URenderer& Renderer = URenderer::GetInstance();
    ID3D11Device* Device = Renderer.GetDevice();

    if (!Text || !Device)
    {
        return false;
    }

    size_t TextLength = strlen(Text);
    if (TextLength == 0)
    {
        return false;
    }

    // 각 문자는 2개의 삼각형(6개의 정점)으로 구성된 쿼드
    TArray<FFontVertex> vertices;
    vertices.reserve(TextLength * 6);

	const float AtlasCols = 16.0f;
	const float AtlasRows = 16.0f;

    // 각 문자에 대해 쿼드 생성
	for (size_t i = 0; i < TextLength; ++i)
	{
		char ch = Text[i];
		uint32 asciiCode = static_cast<uint32>(ch);

		// ★★★ 간단한 쿼드 UV 좌표 (0~1 범위) ★★★
		// 실제 UV 변환은 셰이더에서 처리
		FVector2 uv_topLeft(0.0f, 0.0f);
		FVector2 uv_topRight(1.0f, 0.0f);
		FVector2 uv_bottomLeft(0.0f, 1.0f);
		FVector2 uv_bottomRight(1.0f, 1.0f);

		// 문자의 월드 좌표 계산
		float x = StartX + i * CharWidth;
		float y = StartY;

		// 쿼드의 4개 모서리 좌표
		FVector topLeft(x, y + CharHeight, 0.0f);
		FVector topRight(x + CharWidth, y + CharHeight, 0.0f);
		FVector bottomLeft(x, y, 0.0f);
		FVector bottomRight(x + CharWidth, y, 0.0f);

		// 첫 번째 삼각형 (왼쪽 위, 오른쪽 위, 왼쪽 아래) - 수정된 UV 적용
		FFontVertex vertex1 = { topLeft, uv_topLeft, asciiCode };
		FFontVertex vertex2 = { topRight, uv_topRight, asciiCode };
		FFontVertex vertex3 = { bottomLeft, uv_bottomLeft, asciiCode };
		vertices.push_back(vertex1);
		vertices.push_back(vertex2);
		vertices.push_back(vertex3);

		// 두 번째 삼각형 (오른쪽 위, 오른쪽 아래, 왼쪽 아래) - 수정된 UV 적용
		FFontVertex vertex4 = { topRight, uv_topRight, asciiCode };
		FFontVertex vertex5 = { bottomRight, uv_bottomRight, asciiCode };
		FFontVertex vertex6 = { bottomLeft, uv_bottomLeft, asciiCode };
		vertices.push_back(vertex4);
		vertices.push_back(vertex5);
		vertices.push_back(vertex6);
	}

    VertexCount = static_cast<uint32>(vertices.size());

    // 정점 버퍼 생성
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(FFontVertex) * VertexCount;
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = vertices.data();

    HRESULT hr = Device->CreateBuffer(&bufferDesc, &initData, &FontVertexBuffer);
    if (FAILED(hr))
    {
        UE_LOG_ERROR("FontRenderer: 정점 버퍼 생성 실패 (HRESULT: 0x%08lX)", hr);
        return false;
    }

    // 첫 번째 문자의 정점 데이터 디버그 출력
    if (!vertices.empty())
    {
        const FFontVertex& firstVertex = vertices[0];
        // Debug: 첫 번째 정점 정보
    }

    // 정점 버퍼 생성 완료
    return true;
}

/// @brief 셰이더 생성 및 컴파일
bool UFontRenderer::CreateShaders()
{
    URenderer& Renderer = URenderer::GetInstance();

    // 입력 레이아웃 정의 (위치 + UV + 문자 인덱스)
    TArray<D3D11_INPUT_ELEMENT_DESC> layoutDesc = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 1, DXGI_FORMAT_R32_UINT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    // 버텍스 셰이더 및 입력 레이아웃 생성
    // 셰이더 컴파일 시작
    Renderer.CreateVertexShaderAndInputLayout(
        L"Asset/Shader/ShaderFont.hlsl",
        layoutDesc,
        &FontVertexShader,
        &FontInputLayout
    );
    // 셰이더 컴파일 완료

    if (!FontVertexShader || !FontInputLayout)
    {
        UE_LOG_ERROR("FontRenderer: 버텍스 셰이더 생성 실패");
        return false;
    }

    // 픽셀 셰이더 생성
    Renderer.CreatePixelShader(L"Asset/Shader/ShaderFont.hlsl", &FontPixelShader);
    if (!FontPixelShader)
    {
        UE_LOG_ERROR("FontRenderer: 픽셀 셰이더 생성 실패");
        return false;
    }

    // 셰이더 생성 완뢬
    return true;
}

/// @brief 폰트 텍스처 로드
bool UFontRenderer::LoadFontTexture()
{
    UAssetManager& ResourceManager = UAssetManager::GetInstance();

    // DejaVu Sans Mono.png 폰트 아틀라스 로드
    // 폰트 텍스처 로드
    auto TextureComPtr = ResourceManager.LoadTexture("Asset/Texture/DejaVu Sans Mono.png");
    FontAtlasTexture = TextureComPtr.Get(); // ComPtr에서 원시 포인터 추출

    if (!FontAtlasTexture)
    {
        UE_LOG_ERROR("FontRenderer: 폰트 텍스처 로드 실패");
        return false;
    }
    // 폰트 텍스처 로드 성공

    // 폰트 텍스처 로드 완료
    return true;
}

/// @brief 샘플러 스테이트 생성
bool UFontRenderer::CreateSamplerState()
{
    URenderer& Renderer = URenderer::GetInstance();
    ID3D11Device* Device = Renderer.GetDevice();

    D3D11_SAMPLER_DESC samplerDesc = {};
    // 폰트 텍스처는 선형 필터링을 사용해야 부드럽게 보임
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;       // UV가 범위를 벗어나면 클램프
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    HRESULT hr = Device->CreateSamplerState(&samplerDesc, &FontSampler);
    if (FAILED(hr))
    {
        UE_LOG_ERROR("FontRenderer: 샘플러 스테이트 생성 실패 (HRESULT: 0x%08lX)", hr);
        return false;
    }

    // 샘플러 스테이트 생성 완료
    return true;
}

/// @brief 상수 버퍼 생성
bool UFontRenderer::CreateConstantBuffer()
{
    URenderer& Renderer = URenderer::GetInstance();
    ID3D11Device* Device = Renderer.GetDevice();

    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.ByteWidth = sizeof(FMatrix);  // 월드 매트릭스용
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    HRESULT hr = Device->CreateBuffer(&bufferDesc, nullptr, &FontConstantBuffer);
    if (FAILED(hr))
    {
        UE_LOG_ERROR("FontRenderer: 상수 버퍼 생성 실패 (HRESULT: 0x%08lX)", hr);
        return false;
    }

    // 상수 버퍼 생성 완료
    return true;
}
