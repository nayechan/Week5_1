#pragma once
#include "Factory/Actor/Public/ActorFactory.h"

class ASquareActor;

/**
 * @brief SquareActor Factory
 */
UCLASS()
class USquareActorFactory :
	public UActorFactory
{
	GENERATED_BODY()
	DECLARE_CLASS(USquareActorFactory, UActorFactory)

public:
	USquareActorFactory();
	~USquareActorFactory() override = default;

protected:
	TObjectPtr<AActor> CreateNewActor() override;
};
