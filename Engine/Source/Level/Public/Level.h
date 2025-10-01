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

	// Object Duplication Override
	virtual void DuplicateSubObjects() override;
	virtual UObject* Duplicate() override;

	// 통합된 Actor 관리 인터페이스
	const TArray<TObjectPtr<AActor>>& GetActors() const { return LevelActors; }
	TArray<TObjectPtr<AActor>>& GetActors() { return LevelActors; }

	// PIE 호환을 위한 원시 포인터 배열 반환 (동적으로 생성)
	TArray<AActor*> GetActorsPtrs() const;

	// 기존 인터페이스 유지 (호환성)
	const TArray<TObjectPtr<AActor>>& GetLevelActors() const { return LevelActors; }
	TArray<TObjectPtr<AActor>>& GetLevelActors() { return LevelActors; }

	const TArray<TObjectPtr<UPrimitiveComponent>>& GetLevelPrimitiveComponents() const
	{
		return LevelPrimitiveComponents;
	}

	void AddLevelPrimitiveComponent(AActor* Actor);
	void AddActorToDynamic(AActor* Actor);

	AActor* SpawnActorToLevel(UClass* InActorClass, const FName& InName = FName::GetNone());

	bool DestroyActor(AActor* InActor);
	void MarkActorForDeletion(AActor* InActor);

	void SetSelectedActor(AActor* InActor);
	TObjectPtr<AActor> GetSelectedActor() const { return SelectedActor; }

	uint64 GetShowFlags() const { return ShowFlags; }
	void SetShowFlags(uint64 InShowFlags) { ShowFlags = InShowFlags; }

	void RegisterPrimitiveComponent(UPrimitiveComponent* NewPrimitive);

	// Spatial Index
	FOctree& GetStaticOctree() { return StaticOctree; }
	const FOctree& GetStaticOctree() const { return StaticOctree; }
	const TArray<TObjectPtr<UPrimitiveComponent>>& GetDynamicPrimitives() const { return DynamicPrimitives; }
	void MoveToDynamic(UPrimitiveComponent* InPrim);

private:
	// 통합된 Actor 관리 배열
	TArray<TObjectPtr<AActor>> LevelActors;
	TArray<TObjectPtr<UPrimitiveComponent>> LevelPrimitiveComponents;	// 액터의 하위 컴포넌트는 액터에서 관리&해제됨

	// 지연 삭제를 위한 리스트
	TArray<AActor*> ActorsToDelete;

	TObjectPtr<AActor> SelectedActor = nullptr;

	uint64 ShowFlags = static_cast<uint64>(EEngineShowFlags::SF_Primitives) |
		static_cast<uint64>(EEngineShowFlags::SF_BillboardText) |
		static_cast<uint64>(EEngineShowFlags::SF_Bounds);

	/**
	 * @brief Level에서 Actor를 실질적으로 제거하는 함수
	 * 이전 Tick에서 마킹된 Actor를 제거한다
	 */
	void ProcessPendingDeletions();
	
	/**
	 * @brief Actor를 Init 과정에서 처리하는 헬퍼 함수
	 * Octree에 삽입하고 LevelPrimitiveComponents에 추가
	 */
	void ProcessActorForInit(AActor* Actor);

	// Spatial Index
	FOctree StaticOctree;
	TArray<TObjectPtr<UPrimitiveComponent>> DynamicPrimitives;
};
