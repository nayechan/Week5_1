#include "pch.h"
#include "Component/Mesh/Public/TriangleComponent.h"

#include "Manager/Asset/Public/AssetManager.h"
#include "Physics/Public/AABB.h"

IMPLEMENT_CLASS(UTriangleComponent, UPrimitiveComponent)

UTriangleComponent::UTriangleComponent()
{
	UAssetManager& ResourceManager = UAssetManager::GetInstance();
	Type = EPrimitiveType::Triangle;
	Vertices = ResourceManager.GetVertexData(Type);
	VertexBuffer = ResourceManager.GetVertexbuffer(Type);
	NumVertices = ResourceManager.GetNumVertices(Type);

	Indices = ResourceManager.GetIndexData(Type);
	IndexBuffer = ResourceManager.GetIndexbuffer(Type);
	NumIndices = ResourceManager.GetNumIndices(Type);

	RenderState.CullMode = ECullMode::None;
	RenderState.FillMode = EFillMode::Solid;
	BoundingBox = &ResourceManager.GetAABB(Type);
}
