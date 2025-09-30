#include "pch.h"
#include "Component/Public/ActorComponent.h"

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

void UActorComponent::TickComponent()
{

}

void UActorComponent::EndPlay()
{

}

/**
 * @brief 특정 컴포넌트 전용 Widget이 필요할 경우 재정의 필요 
 */
TObjectPtr<UClass> UActorComponent::GetSpecificWidgetClass() const
{
	return nullptr;
}
