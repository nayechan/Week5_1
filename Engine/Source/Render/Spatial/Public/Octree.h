#pragma once
#include "Physics/Public/AABB.h"
#include "Global/Types.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Render/Spatial/Public/Frustum.h"
#include <array>

class UPrimitiveComponent;

struct FOctree
{
private:
	struct FOctreeNode;

	struct FOctreeNode
	{
		FAABB Bounds;
		TArray<UPrimitiveComponent*> Objects;
		std::array<FOctreeNode*, 8> Children;  // 8개 자식 (고정 크기)
		bool bIsLeaf = true;

		// 계층적 노드 컬링 (n 프레임 연속 시스템)
		mutable int ConsecutiveOccludedFrames = 0;
		mutable bool bShouldCullForOcclusion = false;
		static constexpr int NODE_OCCLUSION_FRAME_THRESHOLD = 3;  // n 프레임 임계값

		static constexpr int MAX_OBJECTS_PER_NODE = 10;
		static constexpr int MAX_DEPTH = 32;  // Very high limit, effectively unlimited

		FOctreeNode(const FAABB& InBounds) : Bounds(InBounds) { Children.fill(nullptr); }
		~FOctreeNode();

		void Subdivide();
		void Insert(UPrimitiveComponent* Object, const FAABB& ObjectBounds, int Depth);
		void Query(const FAABB& QueryBounds, TArray<UPrimitiveComponent*>& Results) const;
		void QueryFrustum(const FFrustum& Frustum, TArray<UPrimitiveComponent*>& Results) const;
		void QueryFrustumWithOcclusion(const FFrustum& Frustum, TArray<UPrimitiveComponent*>& Results,
			bool (*IsOccludedFunc)(const FAABB&, const void*), const void* OcclusionContext) const;
		template<typename RenderCallbackType>
		void QueryFrustumWithRenderCallback(const FFrustum& Frustum,
			bool (*IsOccludedFunc)(const FAABB&, const void*), const void* OcclusionContext,
			RenderCallbackType RenderFunc, const void* RenderContext) const
		{
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

			// 3. 노드가 가려지지 않았으므로 이 노드의 모든 객체들을 즉시 렌더링
			for (UPrimitiveComponent* Object : Objects)
			{
				FVector Min, Max;
				Object->GetWorldAABB(Min, Max);
				FAABB ObjectBounds(Min, Max);

				if (Frustum.IsBoxInFrustum(ObjectBounds))
				{
					RenderFunc(Object, RenderContext);
				}
			}

			// 4. 자식들도 재귀적으로 처리
			if (!bIsLeaf)
			{
				for (FOctreeNode* Child : Children)
				{
					if (Child)
					{
						Child->QueryFrustumWithRenderCallback(Frustum, IsOccludedFunc, OcclusionContext, RenderFunc, RenderContext);
					}
				}
			}
		}
		void Clear();

		// 노드 레벨 옥클루전 관리
		void UpdateNodeOcclusionState(bool bIsOccludedThisFrame) const;
		bool ShouldCullNodeForOcclusion() const { return bShouldCullForOcclusion; }


		FAABB GetChildBounds(int ChildIndex) const;
		FAABB GetChildLooseBounds(int ChildIndex) const;  // 1.5x expanded bounds
		int GetBestChildIndex(const FAABB& ObjectBounds) const;
	};

	FOctreeNode* Root = nullptr;
	FAABB WorldBounds;

public:
	FOctree() = default;
	FOctree(const FAABB& InWorldBounds) : WorldBounds(InWorldBounds)
	{
		Root = new FOctreeNode(InWorldBounds);
	}
	~FOctree() { delete Root; }

	FOctree(const FOctree&) = delete;
	FOctree& operator=(const FOctree&) = delete;
	FOctree(FOctree&& Other) noexcept
		: Root(Other.Root), WorldBounds(Other.WorldBounds)
	{
		Other.Root = nullptr;
	}

	void Initialize(const FAABB& InWorldBounds);
	void Clear();

	bool Insert(UPrimitiveComponent* Object);
	bool Insert(UPrimitiveComponent* Object, const FAABB& ObjectBounds);

	void Remove(UPrimitiveComponent* Object);
	bool Update(UPrimitiveComponent* Object);

	TArray<UPrimitiveComponent*> Query(const FAABB& QueryBounds) const;
	TArray<UPrimitiveComponent*> QueryFrustum(const FFrustum& Frustum) const;
	TArray<UPrimitiveComponent*> QueryFrustumWithOcclusion(const FFrustum& Frustum,
		bool (*IsOccludedFunc)(const FAABB&, const void*), const void* OcclusionContext) const;

	// 람다 기반 렌더링 방식
	template<typename RenderCallbackType>
	void QueryFrustumWithRenderCallback(const FFrustum& Frustum,
		bool (*IsOccludedFunc)(const FAABB&, const void*), const void* OcclusionContext,
		RenderCallbackType RenderFunc, const void* RenderContext) const
	{
		if (Root)
		{
			Root->QueryFrustumWithRenderCallback(Frustum, IsOccludedFunc, OcclusionContext, RenderFunc, RenderContext);
		}
	}


	// Statistics
	uint32 GetObjectCount() const;
	uint32 GetNodeCount() const;

	const FAABB& GetWorldBounds() const { return WorldBounds; }
	uint32 GetMaxNodeObjectCount() const;
	
private:
	// Helper Functions
	bool IsValidObject(UPrimitiveComponent* Object, const FAABB& ObjectBounds) const;
	uint32 CountObjects(FOctreeNode* Node) const;
	uint32 CountNodes(FOctreeNode* Node) const;
	uint32 GetMaxNodeObjectCount(FOctreeNode* Node) const;
	bool RemoveFromNodeRecursive(FOctreeNode* Node, UPrimitiveComponent* Object);
};
