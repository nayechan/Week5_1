#include "pch.h"
#include "Factory/Component/Public/ComponentFactory.h"

#include "Component/Public/ActorComponent.h"
#include "Core/Public/Class.h"

IMPLEMENT_CLASS(UComponentFactory, UFactory)

UComponentFactory::UComponentFactory()
{
    SupportedClass = UActorComponent::StaticClass();
    Description = "Component Factory";
}

/**
 * Factory 기본 규격을 맞추기 위한 함수
 * @return CreateComponent 호출
 */
TObjectPtr<UObject> UComponentFactory::CreateNew()
{
    return TObjectPtr<UObject>(CreateComponent());
}

/**
 * @brief 컴포넌트별 특화된 생성 로직
 * 파생 클래스에서 오버라이드하여 구체적인 컴포넌트 생성 구현
 */
UActorComponent* UComponentFactory::CreateComponent()
{
    return nullptr;
}
