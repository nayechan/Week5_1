#pragma once
#include "Core/Public/Object.h"
#include "Factory/Public/FactorySystem.h"
#include "Factory/Public/NewObject.h"
#include "Render/Spatial/Public/Octree.h"
#include "Editor/Public/Camera.h"

namespace json { class JSON; }
using JSON = json::JSON;

class AAxis;
class AGizmo;
class AGrid;
class AActor;
class UPrimitiveComponent;

/**
 * @brief Level Show Flag Enum
 */
enum class EEngineShowFlags : uint64
{
	SF_Primitives = 0x01,
	SF_BillboardText = 0x10,
	SF_Bounds = 0x20,
};

inline uint64 operator|(EEngineShowFlags lhs, EEngineShowFlags rhs)
{
	return static_cast<uint64>(lhs) | static_cast<uint64>(rhs);
}

inline uint64 operator&(uint64 lhs, EEngineShowFlags rhs)
{
	return lhs & static_cast<uint64>(rhs);
}

UCLASS()
class ULevel :
	public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(ULevel, UObject)

public:
	ULevel();
	ULevel(const FName& InName);
	~ULevel() override;

	virtual void Init();
	virtual void Update();
	virtual void Render();
	virtual void Cleanup();

	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

	// Actor 관리 (외부 인터페이스 - 소유권 없음)
	const TArray<AActor*>& GetActors() const { return ActorsPtrCache; }
	TArray<AActor*>& GetActors() { return ActorsPtrCache; }

	const TArray<TObjectPtr<UPrimitiveComponent>>& GetLevelPrimitiveComponents() const
	{
		return LevelPrimitiveComponents;
	}

	void AddLevelPrimitiveComponent(AActor* Actor);
	void AddActorToDynamic(AActor* Actor);

	AActor* SpawnActorToLevel(UClass* InActorClass, const FName& InName = FName::GetNone());

	// World 복제 시 사용 (BeginPlay 호출하지 않음)
	void AddActorDirect(AActor* InActor);

	bool DestroyActor(AActor* InActor);
	void MarkActorForDeletion(AActor* InActor);

	void SetSelectedActor(AActor* InActor);
	TObjectPtr<AActor> GetSelectedActor() const { return SelectedActor; }

	uint64 GetShowFlags() const { return ShowFlags; }
	void SetShowFlags(uint64 InShowFlags) { ShowFlags = InShowFlags; }

	// Spatial Index
	FOctree& GetStaticOctree() { return StaticOctree; }
	const FOctree& GetStaticOctree() const { return StaticOctree; }
	const TArray<TObjectPtr<UPrimitiveComponent>>& GetDynamicPrimitives() const { return DynamicPrimitives; }
	void MoveToDynamic(UPrimitiveComponent* InPrim);

private:
	// 액터 저장소 (소유권 있음)
	TArray<TObjectPtr<AActor>> Actors;

	// 외부 인터페이스용 캐시 (소유권 없음)
	mutable TArray<AActor*> ActorsPtrCache;

	TArray<TObjectPtr<UPrimitiveComponent>> LevelPrimitiveComponents;	// 액터의 하위 컴포넌트는 액터에서 관리&해제됨

	// 지연 삭제를 위한 리스트
	TArray<AActor*> ActorsToDelete;

	// 캐시 동기화
	void SyncActorsPtrCache() const;

	TObjectPtr<AActor> SelectedActor = nullptr;

	uint64 ShowFlags = static_cast<uint64>(EEngineShowFlags::SF_Primitives) |
		static_cast<uint64>(EEngineShowFlags::SF_BillboardText) |
		static_cast<uint64>(EEngineShowFlags::SF_Bounds);

	/**
	 * @brief Level에서 Actor를 실질적으로 제거하는 함수
	 * 이전 Tick에서 마킹된 Actor를 제거한다
	 */
	void ProcessPendingDeletions();

	// Spatial Index
	FOctree StaticOctree;
	TArray<TObjectPtr<UPrimitiveComponent>> DynamicPrimitives;
};
