#include "pch.h"
#include "Render/Spatial/Public/Octree.h"
#include "Render/Spatial/Public/CullingStrategy.h"
#include "Component/Public/PrimitiveComponent.h"

FOctree::FOctreeNode::~FOctreeNode()
{
	for (FOctreeNode* Child : Children)
	{
		delete Child;
	}
}

void FOctree::FOctreeNode::Subdivide()
{
	/**
	 * @brief 노드를 8개의 자식 노드로 분할합니다
	 * Octree는 3D 공간을 8개의 사분면으로 나눅니다
	 */

	if (!bIsLeaf) return;  // 이미 분할된 노드

	const FVector Center = (Bounds.Min + Bounds.Max) * 0.5f;
	const FVector Size = (Bounds.Max - Bounds.Min) * 0.5f;

	// 8개 자식 노드 생성
	for (int i = 0; i < 8; ++i)
	{
		Children[i] = new FOctreeNode(GetChildBounds(i));
	}

	bIsLeaf = false;
}

void FOctree::FOctreeNode::Insert(UPrimitiveComponent* Object, const FAABB& ObjectBounds, int Depth)
{
	/**
	 * @brief 오브젝트를 Octree에 삽입합니다
	 * 리프 노드이고 용량이 있으면 여기에 저장
	 * 그렇지 않으면 분할 후 적절한 자식에게 위임
	 */

	 // 최대 깊이 체크
	if (Depth >= MAX_DEPTH)
	{
		Objects.push_back(Object);
		return;
	}

	// 리프 노드이고 용량에 여유가 있으면 여기에 저장
	if (bIsLeaf && Objects.size() < MAX_OBJECTS_PER_NODE)
	{
		Objects.push_back(Object);
		return;
	}

	// 분할 필요 → 자식 생성 후 재배치
	if (bIsLeaf)
	{
		Subdivide();

		// 기존 오브젝트들을 자식에게 재배치
		for (UPrimitiveComponent* ExistingObject : Objects)
		{
			FVector Min, Max;
			ExistingObject->GetWorldAABB(Min, Max);
			FAABB ExistingBounds(Min, Max);

			int BestChild = GetBestChildIndex(ExistingBounds);
			if (BestChild >= 0)
			{
				Children[BestChild]->Insert(ExistingObject, ExistingBounds, Depth + 1);
			}
		}
		Objects.clear();
	}

	// 새 오브젝트를 적절한 자식에게 삽입
	int BestChild = GetBestChildIndex(ObjectBounds);
	if (BestChild >= 0)
	{
		Children[BestChild]->Insert(Object, ObjectBounds, Depth + 1);
	}
	else
	{
		// 여러 자식에 걸칩 - 여기에 저장
		Objects.push_back(Object);
	}
}


void FOctree::FOctreeNode::Query(const FAABB& QueryBounds, TArray<UPrimitiveComponent*>& Results) const
{
	/**
	 * @brief AABB와 교집합하는 모든 오브젝트를 찾습니다
	 */

	 // 이 노드가 쿼리 영역과 교집합하는지 확인
	if (!Bounds.Intersects(QueryBounds))
		return;

	// 이 노드의 오브젝트들 추가
	for (UPrimitiveComponent* Object : Objects)
	{
		FVector Min, Max;
		Object->GetWorldAABB(Min, Max);
		FAABB ObjectBounds(Min, Max);

		if (ObjectBounds.Intersects(QueryBounds))
		{
			Results.push_back(Object);
		}
	}

	// 자식들도 재귀적으로 쿼리
	if (!bIsLeaf)
	{
		for (FOctreeNode* Child : Children)
		{
			if (Child)
			{
				Child->Query(QueryBounds, Results);
			}
		}
	}
}

void FOctree::FOctreeNode::QueryFrustum(const FFrustum& Frustum, TArray<UPrimitiveComponent*>& Results) const
{
	/**
	 * @brief Frustum과 교집합하는 모든 오브젝트를 찾습니다
	 * 핵심 기능: Spatial Partitioning + Frustum Culling 통합!
	 */

	 // 1. 이 노드가 Frustum과 교집합하는지 확인
	if (!Frustum.IsBoxInFrustum(Bounds))
		return;  // 이 노드와 모든 자식 스킵

	// 2. 이 노드의 오브젝트들 추가
	for (UPrimitiveComponent* Object : Objects)
	{
		FVector Min, Max;
		Object->GetWorldAABB(Min, Max);
		FAABB ObjectBounds(Min, Max);

		if (Frustum.IsBoxInFrustum(ObjectBounds))
		{
			Results.push_back(Object);
		}
	}

	// 3. 자식들도 재귀적으로 쿼리
	if (!bIsLeaf)
	{
		for (FOctreeNode* Child : Children)
		{
			if (Child)
			{
				Child->QueryFrustum(Frustum, Results);
			}
		}
	}
}

void FOctree::FOctreeNode::QueryFrustumWithOcclusion(const FFrustum& Frustum, TArray<UPrimitiveComponent*>& Results,
	bool (*IsOccludedFunc)(const FAABB&, const void*), const void* OcclusionContext) const
{
	/**
	 * @brief Frustum과 교집합하고 옥클루전되지 않은 모든 오브젝트를 찾습니다
	 * 노드 레벨에서 옥클루전 컬링을 수행합니다
	 */

	 // 1. 이 노드가 Frustum과 교집합하는지 확인
	if (!Frustum.IsBoxInFrustum(Bounds))
		return;  // 이 노드와 모든 자식 스킵

	// 2. 계층적 노드 옥클루전 - n 프레임 연속 시스템
	if (IsOccludedFunc)
	{
		bool bIsNodeOccluded = IsOccludedFunc(Bounds, OcclusionContext);

		// 노드 옥클루전 상태 업데이트
		UpdateNodeOcclusionState(bIsNodeOccluded);

		// n 프레임 이상 연속으로 가려진 경우에만 실제 컬링
		if (ShouldCullNodeForOcclusion())
		{
			// 노드 레벨 컬링: 이 노드와 모든 자식, 객체들을 스킵
			return;
		}
	}

	// 3. 노드가 가려지지 않았으므로 이 노드의 모든 객체들을 추가
	for (UPrimitiveComponent* Object : Objects)
	{
		FVector Min, Max;
		Object->GetWorldAABB(Min, Max);
		FAABB ObjectBounds(Min, Max);

		if (Frustum.IsBoxInFrustum(ObjectBounds))
		{
			Results.push_back(Object);
		}
	}

	// 5. 자식들도 재귀적으로 쿼리
	if (!bIsLeaf)
	{
		for (FOctreeNode* Child : Children)
		{
			if (Child)
			{
				Child->QueryFrustumWithOcclusion(Frustum, Results, IsOccludedFunc, OcclusionContext);
			}
		}
	}
}


void FOctree::FOctreeNode::Clear()
{
	/**
	 * @brief 노드의 모든 데이터를 삭제합니다
	 */

	Objects.clear();

	if (!bIsLeaf)
	{
		for (FOctreeNode* Child : Children)
		{
			delete Child;
		}
		Children.fill(nullptr);
		bIsLeaf = true;
	}
}

FAABB FOctree::FOctreeNode::GetChildBounds(int ChildIndex) const
{
	/**
	 * @brief 자식 노드의 경계 계산
	 * Octree는 3D 공간을 8개로 나눈: 전후(2) × 좌우(2) × 상하(2) = 8
	 */

	const FVector Center = (Bounds.Min + Bounds.Max) * 0.5f;
	const FVector Size = (Bounds.Max - Bounds.Min) * 0.5f;

	// ChildIndex를 3비트로 해석: XYZ 각각 1비트
	float X = (ChildIndex & 1) ? Center.X : Bounds.Min.X;
	float Y = (ChildIndex & 2) ? Center.Y : Bounds.Min.Y;
	float Z = (ChildIndex & 4) ? Center.Z : Bounds.Min.Z;

	float MaxX = (ChildIndex & 1) ? Bounds.Max.X : Center.X;
	float MaxY = (ChildIndex & 2) ? Bounds.Max.Y : Center.Y;
	float MaxZ = (ChildIndex & 4) ? Bounds.Max.Z : Center.Z;

	return FAABB(FVector(X, Y, Z), FVector(MaxX, MaxY, MaxZ));
}

FAABB FOctree::FOctreeNode::GetChildLooseBounds(int ChildIndex) const
{
	/**
	 * @brief 자식 노드의 loose 경계 계산 (2배 확장)
	 * 자식 노드의 중심은 유지하되, 크기만 2배로 확장
	 */

	 // 일반 octree에서 자식의 중심점 계산
	const FVector ParentCenter = (Bounds.Min + Bounds.Max) * 0.5f;
	const FVector ParentHalfSize = (Bounds.Max - Bounds.Min) * 0.5f;

	// 자식의 중심점 (일반 octree와 동일)
	FVector ChildCenter;
	ChildCenter.X = (ChildIndex & 1) ? (ParentCenter.X + ParentHalfSize.X * 0.5f) : (ParentCenter.X - ParentHalfSize.X * 0.5f);
	ChildCenter.Y = (ChildIndex & 2) ? (ParentCenter.Y + ParentHalfSize.Y * 0.5f) : (ParentCenter.Y - ParentHalfSize.Y * 0.5f);
	ChildCenter.Z = (ChildIndex & 4) ? (ParentCenter.Z + ParentHalfSize.Z * 0.5f) : (ParentCenter.Z - ParentHalfSize.Z * 0.5f);

	// 자식 노드의 기본 크기 (부모의 1/2)
	FVector ChildHalfSize = ParentHalfSize * 0.5f;

	// Loose factor 적용 (2배 확장)
	FVector LooseHalfSize = ChildHalfSize * 2.0f;

	return FAABB(ChildCenter - LooseHalfSize, ChildCenter + LooseHalfSize);
}

int FOctree::FOctreeNode::GetBestChildIndex(const FAABB& ObjectBounds) const
{
	/**
	 * @brief Loose octree에서 객체 중심점에 기반한 자식 노드 선택
	 * 객체 중심점이 속하는 자식의 loose bounds에 완전히 포함되는지 확인
	 */

	 // Debug logging for first few calls
	static int DebugCallCount = 0;
	bool bShouldLog = (DebugCallCount < 10);

	const FVector ParentCenter = (Bounds.Min + Bounds.Max) * 0.5f;
	const FVector ObjectCenter = (ObjectBounds.Min + ObjectBounds.Max) * 0.5f;

	// 객체 중심점을 기준으로 자식 인덱스 계산
	int ChildIndex = 0;
	if (ObjectCenter.X >= ParentCenter.X) ChildIndex |= 1;
	if (ObjectCenter.Y >= ParentCenter.Y) ChildIndex |= 2;
	if (ObjectCenter.Z >= ParentCenter.Z) ChildIndex |= 4;

	// 해당 자식의 loose bounds가 객체를 완전히 포함할 수 있는지 확인
	FAABB ChildLooseBounds = GetChildLooseBounds(ChildIndex);
	if (ChildLooseBounds.Contains(ObjectBounds))
	{
		if (bShouldLog)
		{
			UE_LOG("GetBestChildIndex [%d]: Object center (%.3f,%.3f,%.3f) -> Child %d",
				DebugCallCount, ObjectCenter.X, ObjectCenter.Y, ObjectCenter.Z, ChildIndex);
			DebugCallCount++;
		}
		return ChildIndex;
	}

	// 완전히 포함되지 않으면 부모 노드에 저장
	if (bShouldLog)
	{
		UE_LOG("GetBestChildIndex [%d]: Object doesn't fit in target child %d - staying in parent",
			DebugCallCount, ChildIndex);
		DebugCallCount++;
	}
	return -1;
}

void FOctree::Initialize(const FAABB& InWorldBounds)
{
	Clear();
	WorldBounds = InWorldBounds;
	Root = new FOctreeNode(InWorldBounds);
}

void FOctree::Clear()
{
	delete Root;
	Root = nullptr;
}

bool FOctree::Insert(UPrimitiveComponent* Object)
{
	if (!Object) return false;

	FVector Min, Max;
	Object->GetWorldAABB(Min, Max);
	FAABB ObjectBounds(Min, Max);

	return Insert(Object, ObjectBounds);
}

bool FOctree::Insert(UPrimitiveComponent* Object, const FAABB& ObjectBounds)
{
	if (Root && IsValidObject(Object, ObjectBounds))
	{
		Root->Insert(Object, ObjectBounds, 0);
		return true;
	}
	return false;
}

void FOctree::Remove(UPrimitiveComponent* Object)
{
	if (!Object) return;

	uint32 ObjectUUID = Object->GetUUID();
	// Object not found in location map - perform exhaustive search
	// This handles cases where ObjectLocationMap is out of sync
	if (Root)
	{
		RemoveFromNodeRecursive(Root, Object);
	}
}

bool FOctree::Update(UPrimitiveComponent* Object)
{
	if (!Object) return false;

	// Simple approach: remove and reinsert
	Remove(Object);
	Insert(Object);
	return true;
}

TArray<UPrimitiveComponent*> FOctree::Query(const FAABB& QueryBounds) const
{
	TArray<UPrimitiveComponent*> Results;
	if (Root)
	{
		Root->Query(QueryBounds, Results);
	}
	return Results;
}

TArray<UPrimitiveComponent*> FOctree::QueryFrustum(const FFrustum& Frustum) const
{
	TArray<UPrimitiveComponent*> Results;
	if (Root)
	{
		Root->QueryFrustum(Frustum, Results);
	}
	return Results;
}


TArray<UPrimitiveComponent*> FOctree::QueryFrustumWithOcclusion(const FFrustum& Frustum,
	bool (*IsOccludedFunc)(const FAABB&, const void*), const void* OcclusionContext) const
{
	TArray<UPrimitiveComponent*> Results;
	if (Root)
	{
		Root->QueryFrustumWithOcclusion(Frustum, Results, IsOccludedFunc, OcclusionContext);
	}
	return Results;
}

TArray<UPrimitiveComponent*> FOctree::GatherVisible(const ICullingStrategy& Strategy) const
{
	TArray<UPrimitiveComponent*> Results;
	if (Root)
	{
		Root->GatherVisible(Strategy, Results);
	}
	return Results;
}


void FOctree::FOctreeNode::GatherVisible(const ICullingStrategy& Strategy, TArray<UPrimitiveComponent*>& Results) const
{
	// 1. 노드 레벨 컬링
	if (Strategy.ShouldCullNode(Bounds))
		return;  // 이 노드와 모든 자식 스킵

	// 2. 노드 내 객체들 수집
	for (UPrimitiveComponent* Object : Objects)
	{
		FVector Min, Max;
		Object->GetWorldAABB(Min, Max);
		FAABB ObjectBounds(Min, Max);

		if (!Strategy.ShouldCullPrimitive(ObjectBounds))
		{
			Results.push_back(Object);
		}
	}

	// 3. 자식들도 재귀적으로 처리
	if (!bIsLeaf)
	{
		for (FOctreeNode* Child : Children)
		{
			if (Child)
			{
				Child->GatherVisible(Strategy, Results);
			}
		}
	}
}

void FOctree::FOctreeNode::UpdateNodeOcclusionState(bool bIsOccludedThisFrame) const
{
	if (bIsOccludedThisFrame)
	{
		ConsecutiveOccludedFrames++;
		if (ConsecutiveOccludedFrames >= NODE_OCCLUSION_FRAME_THRESHOLD)
		{
			bShouldCullForOcclusion = true;
		}
	}
	else
	{
		ConsecutiveOccludedFrames = 0;
		bShouldCullForOcclusion = false;
	}
}

uint32 FOctree::GetObjectCount() const
{
	return Root ? CountObjects(Root) : 0;
}

uint32 FOctree::GetNodeCount() const
{
	return Root ? CountNodes(Root) : 0;
}

uint32 FOctree::GetMaxNodeObjectCount() const
{
	return Root ? GetMaxNodeObjectCount(Root) : 0;
}

bool FOctree::IsValidObject(UPrimitiveComponent* Object, const FAABB& ObjectBounds) const
{
	return Object != nullptr && WorldBounds.Contains(ObjectBounds);
}

uint32 FOctree::CountObjects(FOctreeNode* Node) const
{
	if (!Node) return 0;

	uint32 Count = Node->Objects.size();
	if (!Node->bIsLeaf)
	{
		for (FOctreeNode* Child : Node->Children)
		{
			Count += CountObjects(Child);
		}
	}
	return Count;
}

uint32 FOctree::CountNodes(FOctreeNode* Node) const
{
	if (!Node) return 0;

	uint32 Count = 1;
	if (!Node->bIsLeaf)
	{
		for (FOctreeNode* Child : Node->Children)
		{
			Count += CountNodes(Child);
		}
	}
	return Count;
}

uint32 FOctree::GetMaxNodeObjectCount(FOctreeNode* Node) const
{
	if (!Node) return 0;

	uint32 MaxCount = Node->Objects.size();

	if (!Node->bIsLeaf)
	{
		for (FOctreeNode* Child : Node->Children)
		{
			uint32 ChildMaxCount = GetMaxNodeObjectCount(Child);
			MaxCount = (ChildMaxCount > MaxCount) ? ChildMaxCount : MaxCount;
		}
	}

	return MaxCount;
}

bool FOctree::RemoveFromNodeRecursive(FOctreeNode* Node, UPrimitiveComponent* Object)
{
	if (!Node || !Object) return false;

	// Check if object is in this node's Objects array
	auto& Objects = Node->Objects;
	auto FoundIt = std::find(Objects.begin(), Objects.end(), Object);
	if (FoundIt != Objects.end())
	{
		Objects.erase(FoundIt);
		return true; // Found and removed
	}

	// Search in children if this is not a leaf node
	if (!Node->bIsLeaf)
	{
		for (FOctreeNode* Child : Node->Children)
		{
			if (Child && RemoveFromNodeRecursive(Child, Object))
			{
				return true; // Found and removed in child
			}
		}
	}

	return false; // Not found in this subtree
}
