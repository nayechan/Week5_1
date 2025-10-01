#include "pch.h"
#include "Manager/Asset/Public/AssetManager.h"

#include "Render/Renderer/Public/Renderer.h"
#include "DirectXTK/WICTextureLoader.h"
#include "DirectXTK/DDSTextureLoader.h"
#include "Component/Mesh/Public/VertexDatas.h"
#include "Physics/Public/AABB.h"
#include "Texture/Public/TextureRenderProxy.h"
#include "Texture/Public/Texture.h"
#include "Manager/Asset/Public/ObjManager.h"
#include "Utility/Public/MeshSimplifier.h"
#include "Utility/Public/ObjExporter.h"
#include "Utility/Public/StaticMeshSerializer.h"

IMPLEMENT_SINGLETON_CLASS_BASE(UAssetManager)

UAssetManager::UAssetManager() = default;

UAssetManager::~UAssetManager() = default;

void UAssetManager::Initialize()
{
	URenderer& Renderer = URenderer::GetInstance();

	// Data 폴더 속 모든 .obj 파일 로드 및 캐싱
	LoadAllObjStaticMesh();

	// TMap.Add()
	VertexDatas.emplace(EPrimitiveType::Cube, &VerticesCube);
	VertexDatas.emplace(EPrimitiveType::Sphere, &VerticesSphere);
	VertexDatas.emplace(EPrimitiveType::Triangle, &VerticesTriangle);
	VertexDatas.emplace(EPrimitiveType::Square, &VerticesSquare);
	VertexDatas.emplace(EPrimitiveType::Billboard, &VerticesBillboard);
	VertexDatas.emplace(EPrimitiveType::Torus, &VerticesTorus);
	VertexDatas.emplace(EPrimitiveType::Arrow, &VerticesArrow);
	VertexDatas.emplace(EPrimitiveType::CubeArrow, &VerticesCubeArrow);
	VertexDatas.emplace(EPrimitiveType::Ring, &VerticesRing);
	VertexDatas.emplace(EPrimitiveType::Line, &VerticesLine);

	IndexDatas.emplace(EPrimitiveType::Cube, &IndicesCube);
	IndexBuffers.emplace(EPrimitiveType::Cube,
		Renderer.CreateIndexBuffer(IndicesCube.data(), static_cast<int>(IndicesCube.size()) * sizeof(uint32)));
	NumIndices.emplace(EPrimitiveType::Cube, static_cast<uint32>(IndicesCube.size()));

	// TArray.GetData(), TArray.Num()*sizeof(FVertexSimple), TArray.GetTypeSize()
	VertexBuffers.emplace(EPrimitiveType::Cube, Renderer.CreateVertexBuffer(
		VerticesCube.data(), static_cast<int>(VerticesCube.size()) * sizeof(FNormalVertex)));
	VertexBuffers.emplace(EPrimitiveType::Sphere, Renderer.CreateVertexBuffer(
		VerticesSphere.data(), static_cast<int>(VerticesSphere.size()) * sizeof(FNormalVertex)));
	VertexBuffers.emplace(EPrimitiveType::Triangle, Renderer.CreateVertexBuffer(
		VerticesTriangle.data(), static_cast<int>(VerticesTriangle.size()) * sizeof(FNormalVertex)));
	VertexBuffers.emplace(EPrimitiveType::Square, Renderer.CreateVertexBuffer(
		VerticesSquare.data(), static_cast<int>(VerticesSquare.size()) * sizeof(FNormalVertex)));
	VertexBuffers.emplace(EPrimitiveType::Billboard, Renderer.CreateVertexBuffer(
		VerticesBillboard.data(), static_cast<int>(VerticesBillboard.size()) * sizeof(FNormalVertex)));
	VertexBuffers.emplace(EPrimitiveType::Torus, Renderer.CreateVertexBuffer(
		VerticesTorus.data(), static_cast<int>(VerticesTorus.size()) * sizeof(FNormalVertex)));
	VertexBuffers.emplace(EPrimitiveType::Arrow, Renderer.CreateVertexBuffer(
		VerticesArrow.data(), static_cast<int>(VerticesArrow.size()) * sizeof(FNormalVertex)));
	VertexBuffers.emplace(EPrimitiveType::CubeArrow, Renderer.CreateVertexBuffer(
		VerticesCubeArrow.data(), static_cast<int>(VerticesCubeArrow.size()) * sizeof(FNormalVertex)));
	VertexBuffers.emplace(EPrimitiveType::Ring, Renderer.CreateVertexBuffer(
		VerticesRing.data(), static_cast<int>(VerticesRing.size()) * sizeof(FNormalVertex)));
	VertexBuffers.emplace(EPrimitiveType::Line, Renderer.CreateVertexBuffer(
		VerticesLine.data(), static_cast<int>(VerticesLine.size()) * sizeof(FNormalVertex)));

	NumVertices.emplace(EPrimitiveType::Cube, static_cast<uint32>(VerticesCube.size()));
	NumVertices.emplace(EPrimitiveType::Sphere, static_cast<uint32>(VerticesSphere.size()));
	NumVertices.emplace(EPrimitiveType::Triangle, static_cast<uint32>(VerticesTriangle.size()));
	NumVertices.emplace(EPrimitiveType::Square, static_cast<uint32>(VerticesSquare.size()));
	NumVertices.emplace(EPrimitiveType::Billboard, static_cast<uint32>(VerticesBillboard.size()));
	NumVertices.emplace(EPrimitiveType::Torus, static_cast<uint32>(VerticesTorus.size()));
	NumVertices.emplace(EPrimitiveType::Arrow, static_cast<uint32>(VerticesArrow.size()));
	NumVertices.emplace(EPrimitiveType::CubeArrow, static_cast<uint32>(VerticesCubeArrow.size()));
	NumVertices.emplace(EPrimitiveType::Ring, static_cast<uint32>(VerticesRing.size()));
	NumVertices.emplace(EPrimitiveType::Line, static_cast<uint32>(VerticesLine.size()));

	// Calculate AABB for all primitive types (excluding StaticMesh)
	for (const auto& Pair : VertexDatas)
	{
		EPrimitiveType Type = Pair.first;
		const auto* Vertices = Pair.second;
		if (!Vertices || Vertices->empty())
			continue;

		AABBs[Type] = CalculateAABB(*Vertices);
	}

	// Calculate AABB for each StaticMesh
	for (const auto& MeshPair : StaticMeshCache)
	{
		const FName& ObjPath = MeshPair.first;
		const auto& Mesh = MeshPair.second;
		if (!Mesh || !Mesh->IsValid())
			continue;

		const auto& Vertices = Mesh->GetVertices();
		if (Vertices.empty())
			continue;

		StaticMeshAABBs[ObjPath] = CalculateAABB(Vertices);
	}

	// Initialize Shaders
	ID3D11VertexShader* vertexShader;
	ID3D11InputLayout* inputLayout;
	TArray<D3D11_INPUT_ELEMENT_DESC> layoutDesc =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};
	URenderer::GetInstance().CreateVertexShaderAndInputLayout(L"Asset/Shader/BatchLineVS.hlsl", layoutDesc,
		&vertexShader, &inputLayout);
	VertexShaders.emplace(EShaderType::BatchLine, vertexShader);
	InputLayouts.emplace(EShaderType::BatchLine, inputLayout);

	ID3D11PixelShader* PixelShader;
	URenderer::GetInstance().CreatePixelShader(L"Asset/Shader/BatchLinePS.hlsl", &PixelShader);
	PixelShaders.emplace(EShaderType::BatchLine, PixelShader);

	// 텍스처 생성
	CreateTexture("Asset/Editor/Icon/Pawn_64x.png");
	CreateTexture("Asset/Editor/Icon/PointLight_64x.png");
	CreateTexture("Asset/Editor/Icon/SpotLight_64x.png");
}

void UAssetManager::Release()
{
	// Texture Resource 해제
	ReleaseAllTextures();

	// TMap.Value()
	for (auto& Pair : VertexBuffers)
	{
		URenderer::GetInstance().ReleaseVertexBuffer(Pair.second);
	}
	for (auto& Pair : IndexBuffers)
	{
		URenderer::GetInstance().ReleaseIndexBuffer(Pair.second);
	}

    for (auto& Pair : StaticMeshVertexBuffers)
    {
        for (ID3D11Buffer* Buffer : Pair.second)
        {
            if(Buffer) Buffer->Release();
        }
    }
    for (auto& Pair : StaticMeshIndexBuffers)
    {
        for (ID3D11Buffer* Buffer : Pair.second)
        {
            if(Buffer) Buffer->Release();
        }
    }

	StaticMeshBVHs.clear();
	StaticMeshCache.clear();	// unique ptr 이라서 자동으로 해제됨
	StaticMeshVertexBuffers.clear();
	StaticMeshIndexBuffers.clear();

	// TMap.Empty()
	VertexBuffers.clear();
	IndexBuffers.clear();
}

/**
 * @brief Data/ 경로 하위에 모든 .obj 파일을 로드 후 캐싱한다
 */
void UAssetManager::LoadAllObjStaticMesh()
{
	URenderer& Renderer = URenderer::GetInstance();

	TArray<FName> ObjList;
	const FString DataDirectory = "Data/"; // 검색할 기본 디렉토리
	// 디렉토리가 실제로 존재하는지 먼저 확인합니다.
	if (std::filesystem::exists(DataDirectory) && std::filesystem::is_directory(DataDirectory))
	{
		// recursive_directory_iterator를 사용하여 디렉토리와 모든 하위 디렉토리를 순회합니다.
		for (const auto& Entry : std::filesystem::recursive_directory_iterator(DataDirectory))
		{
			// 현재 항목이 일반 파일이고, 확장자가 ".obj"인지 확인합니다.
			if (Entry.is_regular_file() && Entry.path().extension() == ".obj")
			{
				// .generic_string()을 사용하여 OS에 상관없이 '/' 구분자를 사용하는 경로를 바로 얻습니다.
				FString PathString = Entry.path().generic_string();
			
				if (PathString.find("_lod_") != FString::npos)
				{
					continue;
				}
			
				// 찾은 파일 경로를 FName으로 변환하여 ObjList에 추가합니다.
				ObjList.push_back(FName(PathString));
			}
		}
	}

	// Enable winding order flip for this OBJ file
	FObjImporter::Configuration Config;
	Config.bFlipWindingOrder = false;
	Config.bIsBinaryEnabled = true;
	Config.bUVToUEBasis = true;
	Config.bPositionToUEBasis = true;

	for (const FName& ObjPath : ObjList)
	{
		//UE_LOG("Processing Mesh: %s", ObjPath.ToString().c_str());
		UStaticMesh* LoadedMesh = FObjManager::LoadObjStaticMesh(ObjPath, Config);

		if (LoadedMesh && LoadedMesh->IsValid())
		{
			//UE_LOG("  -> Loaded UStaticMesh asset with path: %s (Addr: 0x%p)", LoadedMesh->GetAssetPathFileName().ToString().c_str(), (void*)LoadedMesh);

			// 1. Try to load LODs from cache or generate them
			FStaticMesh* BaseMesh = LoadedMesh->GetLOD(0);
			if (BaseMesh && BaseMesh->Indices.size() > 200) // Don't simplify very small meshes
			{
				std::filesystem::path OriginalPath(ObjPath.ToString());
				FString BaseName = OriginalPath.stem().string();
				std::filesystem::path ParentPath = OriginalPath.parent_path();

				// Handle LOD 1 (0.5f)
				std::filesystem::path LOD1Path = ParentPath / (BaseName + "_lod_050.objbin");
				FStaticMesh* LOD1 = nullptr;
				if (std::filesystem::exists(LOD1Path) && std::filesystem::last_write_time(LOD1Path) > std::filesystem::last_write_time(OriginalPath))
				{
					LOD1 = StaticMeshSerializer::LoadFStaticMeshFromBin(LOD1Path);
				}
				else
				{
					LOD1 = UMeshSimplifier::Simplify(BaseMesh, 0.5f);
					if (LOD1)
					{
						std::filesystem::path LOD1ObjPath = ParentPath / (BaseName + "_lod_050.obj");
						FString MtlName = OriginalPath.stem().string() + ".mtl";
						UObjExporter::ExportMesh(LOD1, LOD1ObjPath.string(), MtlName);
						StaticMeshSerializer::SaveFStaticMeshToBin(LOD1, LOD1Path);
					}
				}
				if (LOD1) LoadedMesh->AddLOD(LOD1);

				// Handle LOD 2 (0.25f)
				std::filesystem::path LOD2Path = ParentPath / (BaseName + "_lod_025.objbin");
				FStaticMesh* LOD2 = nullptr;
				if (std::filesystem::exists(LOD2Path) && std::filesystem::last_write_time(LOD2Path) > std::filesystem::last_write_time(OriginalPath))
				{
					LOD2 = StaticMeshSerializer::LoadFStaticMeshFromBin(LOD2Path);
				}
				else
				{
					LOD2 = UMeshSimplifier::Simplify(BaseMesh, 0.25f);
					if (LOD2)
					{
						std::filesystem::path LOD2ObjPath = ParentPath / (BaseName + "_lod_025.obj");
						FString MtlName = OriginalPath.stem().string() + ".mtl";
						UObjExporter::ExportMesh(LOD2, LOD2ObjPath.string(), MtlName);
						StaticMeshSerializer::SaveFStaticMeshToBin(LOD2, LOD2Path);
					}
				}
				if (LOD2) LoadedMesh->AddLOD(LOD2);
			}

            // 2. Create GPU buffers for all generated LOD levels
            TArray<ID3D11Buffer*> VBs;
            TArray<ID3D11Buffer*> IBs;
            for (int32 i = 0; i < LoadedMesh->GetNumLODs(); ++i)
            {
                FStaticMesh* LODData = LoadedMesh->GetLOD(i);
                if (LODData)
                {
                    VBs.push_back(CreateVertexBuffer(LODData->Vertices));
                    IBs.push_back(CreateIndexBuffer(LODData->Indices));
                }
            }

            // 3. Store the buffer arrays in the AssetManager's maps
            StaticMeshVertexBuffers.emplace(ObjPath, VBs);
            StaticMeshIndexBuffers.emplace(ObjPath, IBs);

			// 4. Cache the UStaticMesh asset itself
            StaticMeshCache.emplace(ObjPath, LoadedMesh);
			UE_LOG("  -> Cached asset 0x%p under key '%s'", (void*)LoadedMesh, ObjPath.ToString().c_str());

			// 5. Build BVH for the base mesh
			FStaticMeshBVH StaticMeshBVH;
			StaticMeshBVH.Build(LoadedMesh->GetVertices(0), &LoadedMesh->GetIndices(0));
			StaticMeshBVHs.emplace(ObjPath, std::move(StaticMeshBVH));
		}
	}
}
bool UAssetManager::HasStaticMesh(const FName& InFilePath) const
{
	return StaticMeshCache.find(InFilePath) != StaticMeshCache.end();
}

UStaticMesh* UAssetManager::GetStaticMesh(const FName& InFilePath) const
{
	const auto& Iterator = StaticMeshCache.find(InFilePath);
	return Iterator != StaticMeshCache.end() ? Iterator->second.get() : nullptr;
}

const FStaticMeshBVH* UAssetManager::GetStaticMeshBVH(const FName& InFilePath) const
{
	auto It = StaticMeshBVHs.find(InFilePath);
	return (It != StaticMeshBVHs.end()) ? &It->second : nullptr;
}

const FStaticMeshBVH* UAssetManager::GetStaticMeshBVH(const UStaticMesh* Mesh) const
{
	if (!Mesh || !Mesh->IsValid())
	{
		return nullptr;
	}

	const FName& Path = Mesh->GetAssetPathFileName();
	auto It = StaticMeshBVHs.find(Path);

	return (It != StaticMeshBVHs.end()) ? &It->second : nullptr;
}

const TArray<ID3D11Buffer*>* UAssetManager::GetVertexBuffers(FName InObjPath) const
{
    auto it = StaticMeshVertexBuffers.find(InObjPath);
    if (it != StaticMeshVertexBuffers.end())
    {
        return &it->second;
    }
    return nullptr;
}

const TArray<ID3D11Buffer*>* UAssetManager::GetIndexBuffers(FName InObjPath) const
{
    auto it = StaticMeshIndexBuffers.find(InObjPath);
    if (it != StaticMeshIndexBuffers.end())
    {
        return &it->second;
    }
    return nullptr;
}

ID3D11Buffer* UAssetManager::CreateVertexBuffer(TArray<FNormalVertex> InVertices)
{
	return URenderer::GetInstance().CreateVertexBuffer(InVertices.data(), static_cast<int>(InVertices.size()) * sizeof(FNormalVertex));
}

ID3D11Buffer* UAssetManager::CreateIndexBuffer(TArray<uint32> InIndices)
{
	return URenderer::GetInstance().CreateIndexBuffer(InIndices.data(), static_cast<int>(InIndices.size()) * sizeof(uint32));
}

TArray<FNormalVertex>* UAssetManager::GetVertexData(EPrimitiveType InType)
{
	return VertexDatas[InType];
}

ID3D11Buffer* UAssetManager::GetVertexbuffer(EPrimitiveType InType)
{
	return VertexBuffers[InType];
}

uint32 UAssetManager::GetNumVertices(EPrimitiveType InType)
{
	return NumVertices[InType];
}

TArray<uint32>* UAssetManager::GetIndexData(EPrimitiveType InType)
{
	auto it = IndexDatas.find(InType);
	return (it != IndexDatas.end()) ? it->second : nullptr;
}

ID3D11Buffer* UAssetManager::GetIndexbuffer(EPrimitiveType InType)
{
	auto it = IndexBuffers.find(InType);
	return (it != IndexBuffers.end()) ? it->second : nullptr;
}

uint32 UAssetManager::GetNumIndices(EPrimitiveType InType)
{
	auto it = NumIndices.find(InType);
	return (it != NumIndices.end()) ? it->second : 0;
}

ID3D11VertexShader* UAssetManager::GetVertexShader(EShaderType Type)
{
	return VertexShaders[Type];
}

ID3D11PixelShader* UAssetManager::GetPixelShader(EShaderType Type)
{
	return PixelShaders[Type];
}

ID3D11InputLayout* UAssetManager::GetIputLayout(EShaderType Type)
{
	return InputLayouts[Type];
}

const FAABB& UAssetManager::GetAABB(EPrimitiveType InType)
{
	auto it = AABBs.find(InType);
	if (it != AABBs.end())
	{
		return it->second;
	}

	// Fallback: return a default AABB (will cause issues but better than crash)
	static FAABB DefaultAABB(FVector(-1.0f, -1.0f, -1.0f), FVector(1.0f, 1.0f, 1.0f));
	UE_LOG("AssetManager::GetAABB: Warning - AABB not found for primitive type %d, returning default", static_cast<int>(InType));
	return DefaultAABB;
}

const FAABB& UAssetManager::GetStaticMeshAABB(FName InName)
{
	auto it = StaticMeshAABBs.find(InName);
	if (it != StaticMeshAABBs.end())
	{
		return it->second;
	}

	// Fallback: return a default AABB
	static FAABB DefaultAABB(FVector(-1.0f, -1.0f, -1.0f), FVector(1.0f, 1.0f, 1.0f));
	UE_LOG("AssetManager::GetStaticMeshAABB: Warning - AABB not found for mesh %s, returning default", InName.ToString().c_str());
	return DefaultAABB;
}

/**
 * @brief 파일에서 텍스처를 로드하고 캐시에 저장하는 함수
 * 중복 로딩을 방지하기 위해 이미 로드된 텍스처는 캐시에서 반환
 * @param InFilePath 로드할 텍스처 파일의 경로
 * @return 성공시 ID3D11ShaderResourceView 포인터, 실패시 nullptr
 */
ComPtr<ID3D11ShaderResourceView> UAssetManager::LoadTexture(const FName& InFilePath, const FName& InName)
{
	// 이미 로드된 텍스처가 있는지 확인
	auto Iter = TextureCache.find(InFilePath);
	if (Iter != TextureCache.end())
	{
		return Iter->second;
	}

	// 새로운 텍스처 로드
	ID3D11ShaderResourceView* TextureSRV = CreateTextureFromFile(InFilePath.ToString());
	if (TextureSRV)
	{
		TextureCache[InFilePath] = TextureSRV;
	}

	return TextureSRV;
}

UTexture* UAssetManager::CreateTexture(const FName& InFilePath, const FName& InName)
{
	auto SRV = LoadTexture(InFilePath);
	if (!SRV)	return nullptr;

	ID3D11SamplerState* Sampler = nullptr;
	D3D11_SAMPLER_DESC SamplerDesc = {};
	SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;       // UV가 범위를 벗어나면 클램프
	SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	SamplerDesc.MinLOD = 0;
	SamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	HRESULT hr = URenderer::GetInstance().GetDevice()->CreateSamplerState(&SamplerDesc, &Sampler);
	if (FAILED(hr))
	{
		UE_LOG_ERROR("CreateSamplerState failed (HRESULT: 0x%08lX)", hr);
		return nullptr;
	}

	auto* Proxy = new FTextureRenderProxy(SRV, Sampler);
	auto* Texture = new UTexture(InFilePath, InName);
	Texture->SetRenderProxy(Proxy);

	Textures.emplace(InFilePath, Texture);

	return Texture;
}

/**
 * @brief 캐시된 텍스처를 가져오는 함수
 * 이미 로드된 텍스처만 반환하고 새로 로드하지는 않음
 * @param InFilePath 가져올 텍스처 파일의 경로
 * @return 캐시에 있으면 ID3D11ShaderResourceView 포인터, 없으면 nullptr
 */
ComPtr<ID3D11ShaderResourceView> UAssetManager::GetTexture(const FName& InFilePath)
{
	auto Iter = TextureCache.find(InFilePath);
	if (Iter != TextureCache.end())
	{
		return Iter->second;
	}

	return nullptr;
}

/**
 * @brief 특정 텍스처를 캐시에서 해제하는 함수
 * DirectX 리소스를 해제하고 캐시에서 제거
 * @param InFilePath 해제할 텍스처 파일의 경로
 */
void UAssetManager::ReleaseTexture(const FName& InFilePath)
{
	auto Iter = TextureCache.find(InFilePath);
	if (Iter != TextureCache.end())
	{
		if (Iter->second)
		{
			Iter->second->Release();
		}

		TextureCache.erase(Iter);
	}
}

/**
 * @brief 특정 텍스처가 캐시에 있는지 확인하는 함수
 * @param InFilePath 확인할 텍스처 파일의 경로
 * @return 캐시에 있으면 true, 없으면 false
 */
bool UAssetManager::HasTexture(const FName& InFilePath) const
{
	return TextureCache.find(InFilePath) != TextureCache.end();
}

/**
 * @brief 모든 텍스처 리소스를 해제하는 함수
 * 캐시된 모든 텍스처의 DirectX 리소스를 해제하고 캐시를 비움
 */
void UAssetManager::ReleaseAllTextures()
{
	for (auto& Pair : TextureCache)
	{
		if (Pair.second)
		{
			Pair.second->Release();
		}
	}
	TextureCache.clear();

	for (auto& Pair : Textures)
	{
		if (Pair.second)
		{
			delete Pair.second;
		}
	}
}

/**
 * @brief 파일에서 DirectX 텍스처를 생성하는 내부 함수
 * DirectXTK의 WICTextureLoader를 사용
 * @param InFilePath 로드할 이미지 파일의 경로
 * @return 성공시 ID3D11ShaderResourceView 포인터, 실패시 nullptr
 */
ID3D11ShaderResourceView* UAssetManager::CreateTextureFromFile(const path& InFilePath)
{
	URenderer& Renderer = URenderer::GetInstance();
	ID3D11Device* Device = Renderer.GetDevice();
	ID3D11DeviceContext* DeviceContext = Renderer.GetDeviceContext();

	if (!Device || !DeviceContext)
	{
		UE_LOG_ERROR("ResourceManager: Texture 생성 실패 - Device 또는 DeviceContext가 null입니다");
		return nullptr;
	}

	// 파일 확장자에 따라 적절한 로더 선택
	FString FileExtension = InFilePath.extension().string();
	transform(FileExtension.begin(), FileExtension.end(), FileExtension.begin(), ::tolower);

	ID3D11ShaderResourceView* TextureSRV = nullptr;
	HRESULT ResultHandle;

	try
	{
		if (FileExtension == ".dds")
		{
			// DDS 파일은 DDSTextureLoader 사용
			ResultHandle = DirectX::CreateDDSTextureFromFile(
				Device,
				DeviceContext,
				InFilePath.c_str(),
				nullptr,
				&TextureSRV
			);

			if (SUCCEEDED(ResultHandle))
			{
				UE_LOG_SUCCESS("ResourceManager: DDS 텍스처 로드 성공 - %ls", InFilePath.c_str());
			}
			else
			{
				UE_LOG_ERROR("ResourceManager: DDS 텍스처 로드 실패 - %ls (HRESULT: 0x%08lX)",
					InFilePath.c_str(), ResultHandle);
			}
		}
		else
		{
			// 기타 포맷은 WICTextureLoader 사용 (PNG, JPG, BMP, TIFF 등)
			ResultHandle = DirectX::CreateWICTextureFromFile(
				Device,
				DeviceContext,
				InFilePath.c_str(),
				nullptr, // 텍스처 리소스는 필요 없음
				&TextureSRV
			);

			if (SUCCEEDED(ResultHandle))
			{
				UE_LOG_SUCCESS("ResourceManager: WIC 텍스처 로드 성공 - %ls", InFilePath.c_str());
			}
			else
			{
				UE_LOG_ERROR("ResourceManager: WIC 텍스처 로드 실패 - %ls (HRESULT: 0x%08lX)"
					, InFilePath.c_str(), ResultHandle);
			}
		}
	}
	catch (const exception& Exception)
	{
		UE_LOG_ERROR("ResourceManager: 텍스처 로드 중 예외 발생 - %ls: %s", InFilePath.c_str(), Exception.what());
		return nullptr;
	}

	return SUCCEEDED(ResultHandle) ? TextureSRV : nullptr;
}

/**
 * @brief 메모리 데이터에서 DirectX 텍스처를 생성하는 함수
 * DirectXTK의 WICTextureLoader와 DDSTextureLoader를 사용하여 메모리 데이터에서 텍스처 생성
 * @param InData 이미지 데이터의 포인터
 * @param InDataSize 데이터의 크기 (Byte)
 * @return 성공시 ID3D11ShaderResourceView 포인터, 실패시 nullptr
 * @note DDS 포맷 감지를 위해 매직 넘버를 확인하고 적절한 로더 선택
 * @note 네트워크에서 다운로드한 이미지나 리소스 팩에서 추출한 데이터 처리에 유용
 */
ID3D11ShaderResourceView* UAssetManager::CreateTextureFromMemory(const void* InData, size_t InDataSize)
{
	if (!InData || InDataSize == 0)
	{
		UE_LOG_ERROR("ResourceManager: 메모리 텍스처 생성 실패 - 잘못된 데이터");
		return nullptr;
	}

	URenderer& Renderer = URenderer::GetInstance();
	ID3D11Device* Device = Renderer.GetDevice();
	ID3D11DeviceContext* DeviceContext = Renderer.GetDeviceContext();

	if (!Device || !DeviceContext)
	{
		UE_LOG_ERROR("ResourceManager: 메모리 텍스처 생성 실패 - Device 또는 DeviceContext가 null입니다");
		return nullptr;
	}

	ID3D11ShaderResourceView* TextureSRV = nullptr;
	HRESULT ResultHandle;

	try
	{
		// DDS 매직 넘버 확인 (DDS 파일은 "DDS " 로 시작)
		const uint32 DDS_MAGIC = 0x20534444; // "DDS " in little-endian
		bool bIsDDS = (InDataSize >= 4 && *static_cast<const uint32*>(InData) == DDS_MAGIC);

		if (bIsDDS)
		{
			// DDS 데이터는 DDSTextureLoader 사용
			ResultHandle = DirectX::CreateDDSTextureFromMemory(
				Device,
				DeviceContext,
				static_cast<const uint8*>(InData),
				InDataSize,
				nullptr, // 텍스처 리소스는 필요 없음
				&TextureSRV
			);

			if (SUCCEEDED(ResultHandle))
			{
				UE_LOG_SUCCESS("ResourceManager: DDS 메모리 텍스처 생성 성공 (크기: %zu bytes)", InDataSize);
			}
			else
			{
				UE_LOG_ERROR("ResourceManager: DDS 메모리 텍스처 생성 실패 (HRESULT: 0x%08lX)", ResultHandle);
			}
		}
		else
		{
			// 기타 포맷은 WICTextureLoader 사용 (PNG, JPG, BMP, TIFF 등)
			ResultHandle = DirectX::CreateWICTextureFromMemory(
				Device,
				DeviceContext,
				static_cast<const uint8*>(InData),
				InDataSize,
				nullptr, // 텍스처 리소스는 필요 없음
				&TextureSRV
			);

			if (SUCCEEDED(ResultHandle))
			{
				UE_LOG_SUCCESS("ResourceManager: WIC 메모리 텍스처 생성 성공 (크기: %zu bytes)", InDataSize);
			}
			else
			{
				UE_LOG_ERROR("ResourceManager: WIC 메모리 텍스처 생성 실패 (HRESULT: 0x%08lX)", ResultHandle);
			}
		}
	}
	catch (const exception& Exception)
	{
		UE_LOG_ERROR("ResourceManager: 메모리 텍스처 생성 중 예외 발생: %s", Exception.what());
		return nullptr;
	}

	return SUCCEEDED(ResultHandle) ? TextureSRV : nullptr;
}

/**
 * @brief Vertex 배열로부터 AABB(Axis-Aligned Bounding Box)를 계산하는 헬퍼 함수
 * @param vertices 정점 데이터 배열
 * @return 계산된 FAABB 객체
 */
FAABB UAssetManager::CalculateAABB(const TArray<FNormalVertex>& Vertices)
{
	FVector MinPoint(+FLT_MAX, +FLT_MAX, +FLT_MAX);
	FVector MaxPoint(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for (const auto& Vertex : Vertices)
	{
		MinPoint.X = std::min(MinPoint.X, Vertex.Position.X);
		MinPoint.Y = std::min(MinPoint.Y, Vertex.Position.Y);
		MinPoint.Z = std::min(MinPoint.Z, Vertex.Position.Z);

		MaxPoint.X = std::max(MaxPoint.X, Vertex.Position.X);
		MaxPoint.Y = std::max(MaxPoint.Y, Vertex.Position.Y);
		MaxPoint.Z = std::max(MaxPoint.Z, Vertex.Position.Z);
	}

	return FAABB(MinPoint, MaxPoint);
}
