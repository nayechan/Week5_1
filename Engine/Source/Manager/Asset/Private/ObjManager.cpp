#include "pch.h"

#include "Core/Public/ObjectIterator.h"
#include "Manager/Asset/Public/ObjManager.h"
#include "Manager/Asset/Public/ObjImporter.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Texture/Public/Material.h"
#include "Texture/Public/Texture.h"
#include <filesystem>

/** @brief: Vertex Key for creating index buffer */
using VertexKey = std::tuple<size_t, size_t, size_t>;

struct VertexKeyHash
{
	std::size_t operator() (VertexKey Key) const
	{
		auto Hash1 = std::hash<size_t>{}(std::get<0>(Key));
		auto Hash2 = std::hash<size_t>{}(std::get<1>(Key));
		auto Hash3 = std::hash<size_t>{}(std::get<2>(Key));

		std::size_t Seed = Hash1;
		Seed ^= Hash2 + 0x9e3779b97f4a7c15ULL + (Seed << 6) + (Seed >> 2);
		Seed ^= Hash3 + 0x9e3779b97f4a7c15ULL + (Seed << 6) + (Seed >> 2);

		return Seed;
	}
};

/** @todo: std::filesystem으로 변경 */
FStaticMesh* FObjManager::LoadObjStaticMeshAsset(const FName& PathFileName, const FObjImporter::Configuration& Config)
{
	// This function no longer caches. It creates a new FStaticMesh and returns it.
	// The caller is now responsible for this memory.

	/** #1. '.obj' 파일로부터 오브젝트 정보를 로드 */
	FObjInfo ObjInfo;
	if (!FObjImporter::LoadObj(PathFileName.ToString(), &ObjInfo, Config))
	{
		UE_LOG_ERROR("파일 정보를 읽어오는데 실패했습니다: %s", PathFileName.ToString());
		return nullptr;
	}

	FStaticMesh* StaticMesh = new FStaticMesh();
	StaticMesh->PathFileName = PathFileName;

	if (ObjInfo.ObjectInfoList.size() == 0)
	{
		UE_LOG_ERROR("오브젝트 정보를 찾을 수 없습니다");
        delete StaticMesh;
		return nullptr;
	}

	/** #2. 오브젝트 정보로부터 버텍스 배열과 인덱스 배열을 구성 */
	/** @note: Use only first object in '.obj' file to create FStaticMesh. */
	FObjectInfo& ObjectInfo = ObjInfo.ObjectInfoList[0];

	TMap<VertexKey, size_t, VertexKeyHash> VertexMap;
	for (size_t i = 0; i < ObjectInfo.VertexIndexList.size(); ++i)
	{
		size_t VertexIndex = ObjectInfo.VertexIndexList[i];

		size_t NormalIndex = INVALID_INDEX;
		if (!ObjectInfo.NormalIndexList.empty())
		{
			NormalIndex = ObjectInfo.NormalIndexList[i];
		}

		size_t TexCoordIndex = INVALID_INDEX;
		if (!ObjectInfo.TexCoordIndexList.empty())
		{
			TexCoordIndex = ObjectInfo.TexCoordIndexList[i];
		}

		VertexKey Key{ VertexIndex, NormalIndex, TexCoordIndex };
		auto It = VertexMap.find(Key);
		if (It == VertexMap.end())
		{
			FNormalVertex Vertex = {};
			Vertex.Position = ObjInfo.VertexList[VertexIndex];

			if (NormalIndex != INVALID_INDEX)
			{
				assert("Vertex normal index out of range" && NormalIndex < ObjInfo.NormalList.size());
				Vertex.Normal = ObjInfo.NormalList[NormalIndex];
			}

			if (TexCoordIndex != INVALID_INDEX)
			{
				assert("Texture coordinate index out of range" && TexCoordIndex < ObjInfo.TexCoordList.size());
				Vertex.TexCoord = ObjInfo.TexCoordList[TexCoordIndex];
			}

			size_t Index = StaticMesh->Vertices.size();
			StaticMesh->Vertices.push_back(Vertex);
			StaticMesh->Indices.push_back(Index);
			VertexMap[Key] = Index;
		}
		else
		{
			StaticMesh->Indices.push_back(It->second);
		}
	}

	/** #3. 오브젝트가 사용하는 머티리얼의 목록을 저장 */
	TSet<FName> UniqueMaterialNames;
	for (const auto& MaterialName : ObjectInfo.MaterialNameList)
	{
		UniqueMaterialNames.insert(MaterialName);
	}

	StaticMesh->MaterialInfo.resize(UniqueMaterialNames.size());
	TMap<FName, int32> MaterialNameToSlot;
	int32 CurrentMaterialSlot = 0;

	for (const auto& MaterialName : UniqueMaterialNames)
	{
		for (size_t j = 0; j < ObjInfo.ObjectMaterialInfoList.size(); ++j)
		{
			if (MaterialName == ObjInfo.ObjectMaterialInfoList[j].Name)
			{
				StaticMesh->MaterialInfo[CurrentMaterialSlot].Name = std::move(ObjInfo.ObjectMaterialInfoList[j].Name);
				StaticMesh->MaterialInfo[CurrentMaterialSlot].Ka = std::move(ObjInfo.ObjectMaterialInfoList[j].Ka);
				StaticMesh->MaterialInfo[CurrentMaterialSlot].Kd = std::move(ObjInfo.ObjectMaterialInfoList[j].Kd);
				StaticMesh->MaterialInfo[CurrentMaterialSlot].Ks = std::move(ObjInfo.ObjectMaterialInfoList[j].Ks);
				StaticMesh->MaterialInfo[CurrentMaterialSlot].Ke = std::move(ObjInfo.ObjectMaterialInfoList[j].Ke);
				StaticMesh->MaterialInfo[CurrentMaterialSlot].Ns = std::move(ObjInfo.ObjectMaterialInfoList[j].Ns);
				StaticMesh->MaterialInfo[CurrentMaterialSlot].Ni = std::move(ObjInfo.ObjectMaterialInfoList[j].Ni);
				StaticMesh->MaterialInfo[CurrentMaterialSlot].D = std::move(ObjInfo.ObjectMaterialInfoList[j].D);
				StaticMesh->MaterialInfo[CurrentMaterialSlot].Illumination = std::move(ObjInfo.ObjectMaterialInfoList[j].Illumination);
				StaticMesh->MaterialInfo[CurrentMaterialSlot].KaMap = std::move(ObjInfo.ObjectMaterialInfoList[j].KaMap);
				StaticMesh->MaterialInfo[CurrentMaterialSlot].KdMap = std::move(ObjInfo.ObjectMaterialInfoList[j].KdMap);
				StaticMesh->MaterialInfo[CurrentMaterialSlot].KsMap = std::move(ObjInfo.ObjectMaterialInfoList[j].KsMap);
				StaticMesh->MaterialInfo[CurrentMaterialSlot].NsMap = std::move(ObjInfo.ObjectMaterialInfoList[j].NsMap);
				StaticMesh->MaterialInfo[CurrentMaterialSlot].DMap = std::move(ObjInfo.ObjectMaterialInfoList[j].DMap);
				StaticMesh->MaterialInfo[CurrentMaterialSlot].BumpMap = std::move(ObjInfo.ObjectMaterialInfoList[j].BumpMap);

				MaterialNameToSlot.emplace(MaterialName, CurrentMaterialSlot);
				CurrentMaterialSlot++;
				break;
			}
		}
	}
	/** #4. 오브젝트의 서브메쉬 정보를 저장 */
	StaticMesh->Sections.resize(ObjectInfo.MaterialIndexList.size());
	for (size_t i = 0; i < ObjectInfo.MaterialIndexList.size(); ++i)
	{
		StaticMesh->Sections[i].StartIndex = ObjectInfo.MaterialIndexList[i] * 3;
		if (i < ObjectInfo.MaterialIndexList.size() - 1)
		{
			StaticMesh->Sections[i].IndexCount = (ObjectInfo.MaterialIndexList[i + 1] - ObjectInfo.MaterialIndexList[i]) * 3;
		}
		else
		{
			StaticMesh->Sections[i].IndexCount = (StaticMesh->Indices.size() / 3 - ObjectInfo.MaterialIndexList[i]) * 3;
		}

		const FName& MaterialName = ObjectInfo.MaterialNameList[i];
		auto It = MaterialNameToSlot.find(MaterialName);
		if (It != MaterialNameToSlot.end())
		{
			StaticMesh->Sections[i].MaterialSlot = It->second;
		}
		else
		{
			StaticMesh->Sections[i].MaterialSlot = INVALID_INDEX;
		}
	}

	return StaticMesh;
}

/**
 * @brief MTL 정보를 바탕으로 UStaticMesh에 재질을 설정하는 함수
 */
void FObjManager::CreateMaterialsFromMTL(UStaticMesh* StaticMesh, FStaticMesh* StaticMeshAsset, const FName& ObjFilePath)
{
	if (!StaticMesh || !StaticMeshAsset || StaticMeshAsset->MaterialInfo.empty())
	{
		return;
	}

	// OBJ 파일이 있는 디렉토리 경로 추출
	std::filesystem::path ObjDirectory = std::filesystem::path(ObjFilePath.ToString()).parent_path();

	UAssetManager& AssetManager = UAssetManager::GetInstance();

	size_t MaterialCount = StaticMeshAsset->MaterialInfo.size();
	for (size_t i = 0; i < MaterialCount; ++i)
	{
		const FMaterial& MaterialInfo = StaticMeshAsset->MaterialInfo[i];
		auto* Material = new UMaterial();

		// Diffuse 텍스처 로드 (map_Kd)
		if (!MaterialInfo.KdMap.empty())
		{
			// .generic_string()을 사용하여 경로를 '/'로 통일하고 std::replace 제거
			FString TexturePathStr = (ObjDirectory / MaterialInfo.KdMap).generic_string();

			if (std::filesystem::exists(TexturePathStr))
			{
				UTexture* DiffuseTexture = AssetManager.CreateTexture(TexturePathStr);
				if (DiffuseTexture)
				{
					Material->SetDiffuseTexture(DiffuseTexture);
				}
			}
		}

		// Ambient 텍스처 로드 (map_Ka)
		if (!MaterialInfo.KaMap.empty())
		{
			FString TexturePathStr = (ObjDirectory / MaterialInfo.KaMap).generic_string();

			if (std::filesystem::exists(TexturePathStr))
			{
				UTexture* AmbientTexture = AssetManager.CreateTexture(TexturePathStr);
				if (AmbientTexture)
				{
					Material->SetAmbientTexture(AmbientTexture);
				}
			}
		}

		// Specular 텍스처 로드 (map_Ks)
		if (!MaterialInfo.KsMap.empty())
		{
			FString TexturePathStr = (ObjDirectory / MaterialInfo.KsMap).generic_string();

			if (std::filesystem::exists(TexturePathStr))
			{
				UTexture* SpecularTexture = AssetManager.CreateTexture(TexturePathStr);
				if (SpecularTexture)
				{
					Material->SetSpecularTexture(SpecularTexture);
				}
			}
		}

		// Alpha 텍스처 로드 (map_d)
		if (!MaterialInfo.DMap.empty())
		{
			FString TexturePathStr = (ObjDirectory / MaterialInfo.DMap).generic_string();

			if (std::filesystem::exists(TexturePathStr))
			{
				UTexture* AlphaTexture = AssetManager.CreateTexture(TexturePathStr);
				if (AlphaTexture)
				{
					Material->SetAlphaTexture(AlphaTexture);
				}
			}
		}

		StaticMesh->SetMaterial(static_cast<int32>(i), Material);
	}
}

UStaticMesh* FObjManager::LoadObjStaticMesh(const FName& PathFileName, const FObjImporter::Configuration& Config)
{
	if (UStaticMesh* StaticMesh = UAssetManager::GetInstance().GetStaticMesh(PathFileName))
	{
		return StaticMesh;
	}

	FStaticMesh* StaticMeshAsset = FObjManager::LoadObjStaticMeshAsset(PathFileName, Config);
	if (StaticMeshAsset)
	{
		UStaticMesh* StaticMesh = new UStaticMesh();
		StaticMesh->AddLOD(StaticMeshAsset); // Add the base mesh as LOD 0

		// MTL 정보를 바탕으로 재질 객체 생성
		CreateMaterialsFromMTL(StaticMesh, StaticMeshAsset, PathFileName);

		return StaticMesh;
	}

	return nullptr;
}
