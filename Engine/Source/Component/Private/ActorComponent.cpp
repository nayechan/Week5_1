#include "pch.h"
#include "Component/Public/ActorComponent.h"
#include "Actor/Public/Actor.h"

IMPLEMENT_CLASS(UActorComponent, UObject)

UActorComponent::UActorComponent()
{
	ComponentType = EComponentType::Actor;
}

UActorComponent::~UActorComponent()
{
	SetOuter(nullptr);
}

void UActorComponent::BeginPlay()
{

}

void UActorComponent::TickComponent(float DeltaTime)
{
	// 기본 구현은 비어있음
}

void UActorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 기본 구현은 비어있음
}

/**
 * @brief 특정 컴포넌트 전용 Widget이 필요할 경우 재정의 필요 
 */
TObjectPtr<UClass> UActorComponent::GetSpecificWidgetClass() const
{
	return nullptr;
}

void UActorComponent::DuplicateSubObjects()
{
	Super::DuplicateSubObjects();
	// ActorComponent는 기본적으로 서브오브젝트가 없음
}

UObject* UActorComponent::Duplicate()
{
	UE_LOG("UActorComponent::Duplicate: Starting duplication of %s (UUID: %u)", GetName().ToString().data(), GetUUID());
	
	// NewObject를 사용하여 새로운 Component 생성
	UActorComponent* NewComponent = NewObject<UActorComponent>(nullptr, GetClass());
	if (!NewComponent)
	{
		UE_LOG("UActorComponent::Duplicate: Failed to create new component!");
		return nullptr;
	}
	
	UE_LOG("UActorComponent::Duplicate: New component created with UUID: %u", NewComponent->GetUUID());
	
	// UActorComponent 고유 속성들 복사
	NewComponent->ComponentType = ComponentType;
	NewComponent->bComponentTickEnabled = bComponentTickEnabled;
	
	// 서브 오브젝트 복제
	NewComponent->DuplicateSubObjects();
	
	// Owner는 복제 후에 다시 설정된다
	NewComponent->Owner = nullptr;
	
	UE_LOG("UActorComponent::Duplicate: Duplication completed for %s (UUID: %u) -> %s (UUID: %u)", 
	       GetName().ToString().data(), GetUUID(), 
	       NewComponent->GetName().ToString().data(), NewComponent->GetUUID());
	
	return NewComponent;
}
