#pragma once

/// @brief 폰트 아틀라스를 사용한 텍스트 렌더링 클래스
/// DejaVu Sans Mono.png 512x512 아틀라스에서 16x16 픽셀 글자를 렌더링
class UFontRenderer
{
public:
    /// @brief 폰트 정점 구조체 - 위치, UV, 문자 인덱스 포함
    struct FFontVertex
    {
        FVector Position;        // 3D 월드 좌표
        FVector2 TexCoord;      // 쿼드 내 UV 좌표 (0~1)
        uint32 CharIndex;       // ASCII 문자 코드
    };

    /// @brief 폰트 데이터 상수 버퍼 구조체 (HLSL FontDataBuffer와 일치)
    struct FFontConstantBuffer
    {
        FVector2 AtlasSize = FVector2(512.0f, 512.0f);      // 아틀라스 전체 크기
        FVector2 GlyphSize = FVector2(16.0f, 16.0f);         // 한 글자 크기
        FVector2 GridSize = FVector2(32.0f, 32.0f);          // 아틀라스 그리드 (32x32 글자)
        FVector2 Padding = FVector2(0.0f, 0.0f);             // 여백
    };

    // 생성자와 소멸자
    UFontRenderer();
    ~UFontRenderer();

    /// @brief 폰트 렌더러 초기화 - 셰이더, 텍스처, 버퍼 생성
    bool Initialize();

    /// @brief 리소스 해제
    void Release();

    /// @brief "Hello, World!" 텍스트를 화면에 렌더링 (호환성을 위해 유지)
    /// @param WorldMatrix 월드 변환 행렬
    /// @param ViewProjectionMatrix 뷰-프로젝션 변환 행렬
    //void RenderHelloWorld(const FMatrix& WorldMatrix, const FMatrix& ViewProjectionMatrix);

    /// @brief 임의의 텍스트를 화면에 렌더링
    /// @param Text 렌더링할 텍스트 문자열
    /// @param WorldMatrix 월드 변환 행렬
    /// @param ViewProjectionMatrix 뷰-프로젝션 변환 행렬
    /// @param CenterY 중앙 Y 좌표 (모델 좌표계)
    /// @param StartZ시작 Z 좌표 (모델 좌표계)
    /// @param CharWidth 문자 너비
    /// @param CharHeight 문자 높이
    void RenderText(const char* Text, const FMatrix& WorldMatrix, const FViewProjConstants& ViewProjectionCostants,
                    float CenterY = 0.0f, float StartZ = -2.5f, float CharWidth = 1.0f, float CharHeight = 2.0f);

private:
    /// @brief 텍스트를 위한 정점 버퍼 생성
    /// @param Text 렌더링할 텍스트 문자열
    /// @param StartX 시작 X 좌표
    /// @param StartY 시작 Y 좌표
    /// @param CharWidth 문자 너비
    /// @param CharHeight 문자 높이
    bool CreateVertexBufferForText(const char* Text, float StartX, float StartY, float CharWidth, float CharHeight);

    /// @brief 셰이더 생성
    bool CreateShaders();

    /// @brief 폰트 텍스처 로드
    bool LoadFontTexture();

    /// @brief 샘플러 스테이트 생성
    bool CreateSamplerState();

    /// @brief 상수 버퍼 생성
    bool CreateConstantBuffer();

    ID3D11VertexShader* FontVertexShader = nullptr;
    ID3D11PixelShader* FontPixelShader = nullptr;
    ID3D11InputLayout* FontInputLayout = nullptr;

    /// @brief 텍스처 리소스
    ID3D11ShaderResourceView* FontAtlasTexture = nullptr;
    ID3D11SamplerState* FontSampler = nullptr;

    ID3D11Buffer* FontVertexBuffer = nullptr;
    ID3D11Buffer* FontConstantBuffer = nullptr;

    uint32 VertexCount = 0;
    FFontConstantBuffer ConstantBufferData;
};
