#pragma once
#include "Core/Public/Object.h"
#include "Core/Public/ObjectPtr.h"
#include "Component/Public/ActorComponent.h"
#include "Component/Public/SceneComponent.h"
#include "Factory/Public/NewObject.h"

/**
 * @brief EndPlay 이유를 나타내는 enum
 */
namespace EEndPlayReason
{
	enum Type
	{
		Destroyed,
		LevelTransition,
		EndPlay,
		RemovedFromWorld,
		Quit
	};
}

class UBillBoardComponent;
/**
 * @brief Level에서 렌더링되는 UObject 클래스
 * UWorld로부터 업데이트 함수가 호출되면 component들을 순회하며 위치, 애니메이션, 상태 처리
 */
UCLASS()

class AActor : public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(AActor, UObject)

public:
	AActor();
	AActor(UObject* InOuter);
	~AActor() override;

	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

	void SetActorLocation(const FVector& InLocation) const;
	void SetActorRotation(const FVector& InRotation) const;
	void SetActorScale3D(const FVector& InScale) const;
	void SetUniformScale(bool IsUniform);

	bool IsUniformScale() const;

	bool bTickInEditor = false;

	virtual void BeginPlay();
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);
	virtual void Tick(float DeltaTime);

	// Tick 상태 관리
	bool IsActorTickEnabled() const { return bActorTickEnabled; }
	void SetActorTickEnabled(bool bEnabled) { bActorTickEnabled = bEnabled; }

	// Getter & Setter
	USceneComponent* GetRootComponent() const { return RootComponent.Get(); }
	const TArray<TObjectPtr<UActorComponent>>& GetOwnedComponents() const { return OwnedComponents; }

	void SetRootComponent(USceneComponent* InOwnedComponents) { RootComponent = InOwnedComponents; }

	const FVector& GetActorLocation() const;
	const FVector& GetActorRotation() const;
	const FVector& GetActorScale3D() const;

	template<class T>
	T* CreateDefaultSubobject(const FName& InName)
	{
		static_assert(is_base_of_v<UObject, T>, "생성할 클래스는 UObject를 반드시 상속 받아야 합니다");

		// 1. 템플릿 타입 T로부터 UClass 정보를 가져옵니다.
		TObjectPtr<UClass> ComponentClass = T::StaticClass();

		// 2. NewObject를 호출할 때도 템플릿 타입 T를 사용하여 정확한 타입의 컴포넌트를 생성합니다.
		TObjectPtr<T> NewComponent = NewObject<T>(TObjectPtr<UObject>(this), ComponentClass, InName);

		// 3. 컴포넌트 생성이 성공했는지 확인하고 기본 설정을 합니다.
		if (NewComponent)
		{
			NewComponent->SetOwner(this);
			OwnedComponents.push_back(NewComponent);
		}

		// 4. 정확한 타입(T*)으로 캐스팅 없이 바로 반환합니다.
		return NewComponent;
	}

private:
	TObjectPtr<USceneComponent> RootComponent = nullptr;
	TObjectPtr<UBillBoardComponent> BillBoardComponent = nullptr;
	TArray<TObjectPtr<UActorComponent>> OwnedComponents;

	// Tick 상태
	bool bActorTickEnabled = true;
};
