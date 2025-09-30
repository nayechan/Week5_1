#pragma once
#include "Component/Public/SceneComponent.h"
#include "Physics/Public/BoundingVolume.h"
#include "Physics/Public/AABB.h"

UCLASS()
class UPrimitiveComponent : public USceneComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UPrimitiveComponent, USceneComponent)

public:
	UPrimitiveComponent();

	const TArray<FNormalVertex>* GetVerticesData() const;
	const TArray<uint32>* GetIndicesData() const;
	ID3D11Buffer* GetVertexBuffer() const;
	ID3D11Buffer* GetIndexBuffer() const;
	const uint32 GetNumVertices() const;
	const uint32 GetNumIndices() const;

	const FRenderState& GetRenderState() const { return RenderState; }

	void SetTopology(D3D11_PRIMITIVE_TOPOLOGY InTopology);
	D3D11_PRIMITIVE_TOPOLOGY GetTopology() const;
	//void Render(const URenderer& Renderer) const override;

	bool IsVisible() const { return bVisible; }
	void SetVisibility(bool bVisibility) { bVisible = bVisibility; }

	FVector4 GetColor() const { return Color; }
	void SetColor(const FVector4& InColor) { Color = InColor; }

	const IBoundingVolume* GetBoundingBox() const { return BoundingBox; }
	void GetWorldAABB(FVector& OutMin, FVector& OutMax) const;
	void MarkWorldAABBDirty() { bWorldAABBDirty = true; }

	EPrimitiveType GetPrimitiveType() const { return Type; }

	// Occlusion Culling 관리 메서드
	void UpdateOcclusionState(bool bIsOccludedThisFrame) const;
	bool ShouldCullForOcclusion() const { return bShouldCullForOcclusion; }
	int GetConsecutiveOccludedFrames() const { return ConsecutiveOccludedFrames; }

protected:
	const TArray<FNormalVertex>* Vertices = nullptr;
	const TArray<uint32>* Indices = nullptr;

	ID3D11Buffer* VertexBuffer = nullptr;
	ID3D11Buffer* IndexBuffer = nullptr;

	uint32 NumVertices = 0;
	uint32 NumIndices = 0;

	FVector4 Color = FVector4{ 1.f,1.f,1.f,1.f };

	D3D11_PRIMITIVE_TOPOLOGY Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	FRenderState RenderState = {};
	EPrimitiveType Type = EPrimitiveType::Cube;

	bool bVisible = true;

	const IBoundingVolume* BoundingBox = nullptr;
	mutable FAABB CachedWorldAABB;
	mutable bool bWorldAABBDirty = true;

	// Occlusion Culling 상태 (3프레임 연속 시스템)
	mutable int ConsecutiveOccludedFrames = 0;  // 연속으로 가려진 프레임 수
	mutable bool bShouldCullForOcclusion = false;  // 실제 컬링 여부
};
