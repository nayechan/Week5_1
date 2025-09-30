#include "pch.h"
#include "Component/Public/PrimitiveComponent.h"

#include "Manager/Asset/Public/AssetManager.h"
#include "Physics/Public/AABB.h"

IMPLEMENT_CLASS(UPrimitiveComponent, USceneComponent)

UPrimitiveComponent::UPrimitiveComponent()
{
	ComponentType = EComponentType::Primitive;
}

void USceneComponent::SetRelativeLocation(const FVector& Location)
{
	RelativeLocation = Location;
	MarkAsDirty();
}

void USceneComponent::SetRelativeRotation(const FVector& Rotation)
{
	RelativeRotation = Rotation;
	MarkAsDirty();
}
void USceneComponent::SetRelativeScale3D(const FVector& Scale)
{
	FVector ActualScale = Scale;
	if (ActualScale.X < MinScale)
		ActualScale.X = MinScale;
	if (ActualScale.Y < MinScale)
		ActualScale.Y = MinScale;
	if (ActualScale.Z < MinScale)
		ActualScale.Z = MinScale;
	RelativeScale3D = ActualScale;
	MarkAsDirty();
}

void USceneComponent::SetUniformScale(bool bIsUniform)
{
	bIsUniformScale = bIsUniform;
}

bool USceneComponent::IsUniformScale() const
{
	return bIsUniformScale;
}

const FVector& USceneComponent::GetRelativeLocation() const
{
	return RelativeLocation;
}
const FVector& USceneComponent::GetRelativeRotation() const
{
	return RelativeRotation;
}
const FVector& USceneComponent::GetRelativeScale3D() const
{
	return RelativeScale3D;
}

const FMatrix& USceneComponent::GetWorldTransformMatrix() const
{
	if (bIsTransformDirty)
	{
		WorldTransformMatrix = FMatrix::GetModelMatrix(RelativeLocation, FVector::GetDegreeToRadian(RelativeRotation), RelativeScale3D);


		for (USceneComponent* Ancester = ParentAttachment; Ancester && Ancester->ParentAttachment; Ancester = Ancester->ParentAttachment)
		{
			WorldTransformMatrix *= FMatrix::GetModelMatrix(Ancester->RelativeLocation, FVector::GetDegreeToRadian(Ancester->RelativeRotation), Ancester->RelativeScale3D);
		}

		bIsTransformDirty = false;
	}

	return WorldTransformMatrix;
}

const FMatrix& USceneComponent::GetWorldTransformMatrixInverse() const
{

	if (bIsTransformDirtyInverse)
	{
		WorldTransformMatrixInverse = FMatrix::Identity();
		for (USceneComponent* Ancester = ParentAttachment; Ancester && Ancester->ParentAttachment; Ancester = Ancester->ParentAttachment)
		{
			WorldTransformMatrixInverse = FMatrix::GetModelMatrixInverse(Ancester->RelativeLocation, FVector::GetDegreeToRadian(Ancester->RelativeRotation), Ancester->RelativeScale3D) * WorldTransformMatrixInverse;

		}
		WorldTransformMatrixInverse = WorldTransformMatrixInverse * FMatrix::GetModelMatrixInverse(RelativeLocation, FVector::GetDegreeToRadian(RelativeRotation), RelativeScale3D);

		bIsTransformDirtyInverse = false;
	}

	return WorldTransformMatrixInverse;
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
		const FMatrix& WorldTransform = GetWorldTransformMatrix();
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
