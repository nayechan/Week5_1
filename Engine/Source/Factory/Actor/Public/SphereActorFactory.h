#pragma once
#include "Factory/Actor/Public/ActorFactory.h"

class ASphereActor;

/**
 * @brief SphereActor Factory
 */
UCLASS()
class USphereActorFactory :
	public UActorFactory
{
	GENERATED_BODY()
	DECLARE_CLASS(USphereActorFactory, UActorFactory)

public:
	USphereActorFactory();
	~USphereActorFactory() override = default;

protected:
	TObjectPtr<AActor> CreateNewActor() override;
};
