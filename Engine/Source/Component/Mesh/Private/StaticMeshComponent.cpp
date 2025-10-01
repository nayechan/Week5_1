#include "pch.h"
#include "Core/Public/Class.h"       // UObject 기반 클래스 및 매크로
#include "Core/Public/ObjectPtr.h"
#include "Core/Public/ObjectIterator.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Component/Mesh/Public/MeshComponent.h"
#include "Manager/Asset/Public/ObjManager.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Physics/Public/AABB.h"
#include "Render/UI/Widget/Public/StaticMeshComponentWidget.h"
#include "Utility/Public/JsonSerializer.h"
#include "Texture/Public/Texture.h"

#include <json.hpp>

IMPLEMENT_CLASS(UStaticMeshComponent, UMeshComponent)

UStaticMeshComponent::UStaticMeshComponent()
	: bIsScrollEnabled(false)
{
	Type = EPrimitiveType::StaticMesh;

	// Set default LOD distances (squared) - Updated to match config values
	LODDistancesSquared.push_back(0.0f);        // LOD 0
	LODDistancesSquared.push_back(10.0f * 10.0f); // LOD 1 starts at 10 units (100 squared)
	LODDistancesSquared.push_back(80.0f * 80.0f); // LOD 2 starts at 80 units (6400 squared)

	FName DefaultObjPath = "Data/Cube/Cube.obj";
	SetStaticMesh(DefaultObjPath);
}
  
UStaticMeshComponent::~UStaticMeshComponent()
{
}

void UStaticMeshComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	// 불러오기
	if (bInIsLoading)
	{
		FString AssetPath;
		FJsonSerializer::ReadString(InOutHandle, "ObjStaticMeshAsset", AssetPath);
		SetStaticMesh(AssetPath);

		JSON OverrideMaterialJson;
		if (FJsonSerializer::ReadObject(InOutHandle, "OverrideMaterial", OverrideMaterialJson, nullptr, false))
		{
			for (auto& Pair : OverrideMaterialJson.ObjectRange())
			{
				const FString& IdString = Pair.first;
				JSON& MaterialPathDataJson = Pair.second;

				int32 MaterialId;
				try { MaterialId = std::stoi(IdString); }
				catch (const std::exception&) { continue; }

				FString MaterialPath;
				FJsonSerializer::ReadString(MaterialPathDataJson, "Path", MaterialPath);

				for (TObjectIterator<UMaterial> It; It; ++It)
				{
					UMaterial* Mat = *It;
					if (!Mat) continue;

					if (Mat->GetDiffuseTexture()->GetFilePath() == MaterialPath)
					{
						SetMaterial(MaterialId, Mat);
						break;
					}
				}
			}
		}
	}
	// 저장
	else
	{
		if (StaticMesh)
		{
			InOutHandle["ObjStaticMeshAsset"] = StaticMesh->GetAssetPathFileName().ToString();

			if (0 < OverrideMaterials.size())
			{
				int Idx = 0;
				JSON MaterialsJson = json::Object();
				for (const UMaterial* Material : OverrideMaterials)
				{
					JSON MaterialJson;
					MaterialJson["Path"] = Material->GetDiffuseTexture()->GetFilePath().ToString();

					MaterialsJson[std::to_string(Idx++)] = MaterialJson;
				}
				InOutHandle["OverrideMaterial"] = MaterialsJson;
			}
		}
	}
}

TObjectPtr<UClass> UStaticMeshComponent::GetSpecificWidgetClass() const
{
	return UStaticMeshComponentWidget::StaticClass();
}

void UStaticMeshComponent::SetStaticMesh(const FName& InObjPath)
{
	UAssetManager& AssetManager = UAssetManager::GetInstance();

	UStaticMesh* NewStaticMesh = AssetManager.GetStaticMesh(InObjPath);

	if (NewStaticMesh)
	{
		StaticMesh = NewStaticMesh;
        VertexBuffers = AssetManager.GetVertexBuffers(InObjPath);
        IndexBuffers = AssetManager.GetIndexBuffers(InObjPath);

		RenderState.CullMode = ECullMode::Back;
		RenderState.FillMode = EFillMode::Solid;
		BoundingBox = &AssetManager.GetStaticMeshAABB(InObjPath);
	}
}

ID3D11Buffer* UStaticMeshComponent::GetVertexBuffer(int32 LODIndex) const
{
    if (VertexBuffers && LODIndex >= 0 && LODIndex < VertexBuffers->size())
    {
        return (*VertexBuffers)[LODIndex];
    }
    return nullptr;
}

ID3D11Buffer* UStaticMeshComponent::GetIndexBuffer(int32 LODIndex) const
{
    if (IndexBuffers && LODIndex >= 0 && LODIndex < IndexBuffers->size())
    {
        return (*IndexBuffers)[LODIndex];
    }
    return nullptr;
}

UMaterial* UStaticMeshComponent::GetMaterial(int32 Index) const
{
	if (Index >= 0 && Index < OverrideMaterials.size() && OverrideMaterials[Index])
	{
		return OverrideMaterials[Index];
	}
	return StaticMesh ? StaticMesh->GetMaterial(Index) : nullptr;
}

void UStaticMeshComponent::SetMaterial(int32 Index, UMaterial* InMaterial)
{
	if (Index < 0) return;

	if (Index >= OverrideMaterials.size())
	{
		OverrideMaterials.resize(Index + 1, nullptr);
	}
	OverrideMaterials[Index] = InMaterial;
}

void UStaticMeshComponent::DuplicateSubObjects()
{
	Super::DuplicateSubObjects();
	
	UE_LOG("StaticMeshComponent::DuplicateSubObjects: Duplicating mesh data for %s", GetName().ToString().data());
	
	// StaticMesh 정보는 공유되므로 복사하지 않음 (이미 AssetManager에 공유됨)
	// VertexBuffers와 IndexBuffers는 AssetManager로부터 다시 가져와야 함
	if (StaticMesh)
	{
		// 원본 메시의 에셋 경로를 사용하여 다시 로드
		FName AssetPath = StaticMesh->GetAssetPathFileName();
		SetStaticMesh(AssetPath);
		
		UE_LOG("StaticMeshComponent::DuplicateSubObjects: StaticMesh data copied from %s", 
		       AssetPath.ToString().data());
	}
	else
	{
		UE_LOG("StaticMeshComponent::DuplicateSubObjects: Warning - No StaticMesh to duplicate!");
	}
	
	// OverrideMaterials 복사 (딩 복사 필요)
	// 이미 복사된 소스의 OverrideMaterials를 가져와서 복사
	// 하지만 이 시점에서는 원본에 접근할 수 없으므로 일단 로그만 출력
	UE_LOG("StaticMeshComponent::DuplicateSubObjects: Component duplication completed");
}
