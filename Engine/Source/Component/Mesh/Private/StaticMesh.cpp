#include "pch.h"
#include "Source/Component/Mesh/Public/StaticMesh.h"

IMPLEMENT_CLASS(UStaticMesh, UObject)

UStaticMesh::UStaticMesh()
{
}

UStaticMesh::~UStaticMesh()
{
    for (FStaticMesh* lod : AllLODs)
    {
        SafeDelete(lod);
    }
    AllLODs.clear();

	for (UMaterial* Material : Materials)
	{
		SafeDelete(Material);
	}
	Materials.clear();
}

void UStaticMesh::AddLOD(FStaticMesh* InLODData)
{
    if (InLODData)
    {
        AllLODs.push_back(InLODData);
    }
}

FStaticMesh* UStaticMesh::GetLOD(int32 LODIndex) const
{
    if (LODIndex >= 0 && LODIndex < AllLODs.size())
    {
        return AllLODs[LODIndex];
    }
    return nullptr;
}

int32 UStaticMesh::GetNumLODs() const
{
    return AllLODs.size();
}

const FName& UStaticMesh::GetAssetPathFileName() const
{
    if (IsValid())
    {
        return AllLODs[0]->PathFileName;
    }
    static const FName EmptyString = "";
    return EmptyString;
}

const TArray<FNormalVertex>& UStaticMesh::GetVertices(int32 LODIndex) const
{
    if (FStaticMesh* LOD = GetLOD(LODIndex))
    {
        return LOD->Vertices;
    }
    static const TArray<FNormalVertex> EmptyVertices;
    return EmptyVertices;
}

TArray<FNormalVertex>& UStaticMesh::GetVertices(int32 LODIndex)
{
    if (FStaticMesh* LOD = GetLOD(LODIndex))
    {
        return LOD->Vertices;
    }
    static TArray<FNormalVertex> EmptyVertices;
    return EmptyVertices;
}

const TArray<uint32>& UStaticMesh::GetIndices(int32 LODIndex) const
{
    if (FStaticMesh* LOD = GetLOD(LODIndex))
    {
        return LOD->Indices;
    }
    static const TArray<uint32> EmptyIndices;
    return EmptyIndices;
}

UMaterial* UStaticMesh::GetMaterial(int32 MaterialIndex) const
{
    return (MaterialIndex >= 0 && MaterialIndex < Materials.size()) ? Materials[MaterialIndex] : nullptr;
}

void UStaticMesh::SetMaterial(int32 MaterialIndex, UMaterial* Material)
{
    if (MaterialIndex >= 0)
    {
        if (MaterialIndex >= static_cast<int32>(Materials.size()))
        {
            Materials.resize(MaterialIndex + 1, nullptr);
        }
        Materials[MaterialIndex] = Material;
    }
}

int32 UStaticMesh::GetNumMaterials() const
{
    return Materials.size();
}

const TArray<FMeshSection>& UStaticMesh::GetSections(int32 LODIndex) const
{
    if (FStaticMesh* LOD = GetLOD(LODIndex))
    {
        return LOD->Sections;
    }
    static const TArray<FMeshSection> EmptySections;
    return EmptySections;
}

bool UStaticMesh::IsValid() const
{
    return !AllLODs.empty() && AllLODs[0] != nullptr;
}
