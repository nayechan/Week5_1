#include "pch.h"
#include "Component/Public/LineComponent.h"

#include "Manager/Asset/Public/AssetManager.h"

IMPLEMENT_CLASS(ULineComponent, UPrimitiveComponent)

ULineComponent::ULineComponent()
{
	UAssetManager& ResourceManager = UAssetManager::GetInstance();
	Type = EPrimitiveType::Line;
	Vertices = ResourceManager.GetVertexData(Type);
	VertexBuffer = ResourceManager.GetVertexbuffer(Type);
	NumVertices = ResourceManager.GetNumVertices(Type);
	Topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	RenderState.CullMode = ECullMode::None;
	RenderState.FillMode = EFillMode::WireFrame;
}
