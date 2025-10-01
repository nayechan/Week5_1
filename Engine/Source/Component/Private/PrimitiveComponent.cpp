#include "pch.h"
#include "Component/Public/PrimitiveComponent.h"

#include "Manager/Asset/Public/AssetManager.h"
#include "Physics/Public/AABB.h"

IMPLEMENT_CLASS(UPrimitiveComponent, USceneComponent)

UPrimitiveComponent::UPrimitiveComponent()
{
	ComponentType = EComponentType::Primitive;
}

const TArray<FNormalVertex>* UPrimitiveComponent::GetVerticesData() const
{
	return Vertices;
}

const TArray<uint32>* UPrimitiveComponent::GetIndicesData() const
{
	return Indices;
}

ID3D11Buffer* UPrimitiveComponent::GetVertexBuffer() const
{
	return VertexBuffer;
}

ID3D11Buffer* UPrimitiveComponent::GetIndexBuffer() const
{
	return IndexBuffer;
}

const uint32 UPrimitiveComponent::GetNumVertices() const
{
	return NumVertices;
}

const uint32 UPrimitiveComponent::GetNumIndices() const
{
	return NumIndices;
}

void UPrimitiveComponent::SetTopology(D3D11_PRIMITIVE_TOPOLOGY InTopology)
{
	Topology = InTopology;
}

D3D11_PRIMITIVE_TOPOLOGY UPrimitiveComponent::GetTopology() const
{
	return Topology;
}

void UPrimitiveComponent::GetWorldAABB(FVector& OutMin, FVector& OutMax) const
{
	if (!bWorldAABBDirty)
	{
		OutMin = CachedWorldAABB.Min;
		OutMax = CachedWorldAABB.Max;
		return;
	}
	if (!BoundingBox) { return; }

	if (BoundingBox->GetType() == EBoundingVolumeType::AABB)
	{
		const FAABB* LocalAABB = static_cast<const FAABB*>(BoundingBox);
		FVector LocalCorners[8] =
		{
			FVector(LocalAABB->Min.X, LocalAABB->Min.Y, LocalAABB->Min.Z), FVector(LocalAABB->Max.X, LocalAABB->Min.Y, LocalAABB->Min.Z),
			FVector(LocalAABB->Min.X, LocalAABB->Max.Y, LocalAABB->Min.Z), FVector(LocalAABB->Max.X, LocalAABB->Max.Y, LocalAABB->Min.Z),
			FVector(LocalAABB->Min.X, LocalAABB->Min.Y, LocalAABB->Max.Z), FVector(LocalAABB->Max.X, LocalAABB->Min.Y, LocalAABB->Max.Z),
			FVector(LocalAABB->Min.X, LocalAABB->Max.Y, LocalAABB->Max.Z), FVector(LocalAABB->Max.X, LocalAABB->Max.Y, LocalAABB->Max.Z)
		};
		const FMatrix& WorldTransform = GetWorldTransform();
		FVector WorldMin(+FLT_MAX, +FLT_MAX, +FLT_MAX);
		FVector WorldMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);

		for (int i = 0; i < 8; i++)
		{
			FVector4 WorldCorner = FVector4(LocalCorners[i].X, LocalCorners[i].Y, LocalCorners[i].Z, 1.0f) * WorldTransform;
			WorldMin.X = std::min(WorldMin.X, WorldCorner.X);
			WorldMin.Y = std::min(WorldMin.Y, WorldCorner.Y);
			WorldMin.Z = std::min(WorldMin.Z, WorldCorner.Z);
			WorldMax.X = std::max(WorldMax.X, WorldCorner.X);
			WorldMax.Y = std::max(WorldMax.Y, WorldCorner.Y);
			WorldMax.Z = std::max(WorldMax.Z, WorldCorner.Z);
		}
		OutMin = WorldMin;
		OutMax = WorldMax;
		CachedWorldAABB = FAABB(WorldMin, WorldMax);
		bWorldAABBDirty = false;
	}
}

void UPrimitiveComponent::UpdateOcclusionState(bool bIsOccludedThisFrame) const
{
	if (bIsOccludedThisFrame)
	{
		// 가려진 프레임 수 증가
		ConsecutiveOccludedFrames++;

		// 5프레임 이상 연속으로 가려지면 실제 컬링
		if (ConsecutiveOccludedFrames >= 5)
		{
			bShouldCullForOcclusion = true;
		}
	}
	else
	{
		// 보이는 순간 즉시 컬링 해제
		ConsecutiveOccludedFrames = 0;
		bShouldCullForOcclusion = false;
	}
}

UObject* UPrimitiveComponent::Duplicate()
{
	// Call parent class Duplicate (SceneComponent)
	USceneComponent* NewSceneComponent = static_cast<USceneComponent*>(USceneComponent::Duplicate());
	UPrimitiveComponent* NewComponent = Cast<UPrimitiveComponent>(NewSceneComponent);

	if (!NewComponent)
	{
		return NewSceneComponent;
	}

	// Copy PrimitiveComponent-specific properties
	// NOTE: Vertices, Indices, VertexBuffer, IndexBuffer, BoundingBox are all pointers
	// to AssetManager-owned resources, so they should be set by the constructor
	// when NewObject creates the component. We don't need to copy them manually.

	NewComponent->Color = Color;
	NewComponent->Topology = Topology;
	NewComponent->RenderState = RenderState;
	NewComponent->Type = Type;
	NewComponent->bVisible = bVisible;

	// Reset cached data
	NewComponent->bWorldAABBDirty = true;
	NewComponent->ConsecutiveOccludedFrames = 0;
	NewComponent->bShouldCullForOcclusion = false;

	return NewComponent;
}
