#pragma once
#include "Factory/Public/Factory.h"

class UActorComponent;

/**
 * @brief ComponentFactory Base Class
 */
UCLASS()
class UComponentFactory : public UFactory
{
	GENERATED_BODY()
	DECLARE_CLASS(UComponentFactory, UFactory)

public:
	UComponentFactory();
	~UComponentFactory() override = default;

	TObjectPtr<UObject> CreateNew() override;

protected:
	virtual UActorComponent* CreateComponent();
};

/**
 * @brief 템플릿 기반 컴포넌트 팩토리
 * 각 컴포넌트 타입별로 특화된 팩토리를 쉽게 만들 수 있도록 구현
 */
template <typename T>
class TComponentFactory : public UComponentFactory
{
public:
	TComponentFactory()
	{
		SupportedClass = T::StaticClass();
		Description = FString("Factory for ") + T::StaticClass()->GetClass().ToString();
	}

protected:
	UActorComponent* CreateComponent() override
	{
		T* NewComponent = new T();
		return NewComponent;
	}
};
