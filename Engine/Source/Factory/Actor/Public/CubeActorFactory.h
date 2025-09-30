#pragma once
#include "Factory/Actor/Public/ActorFactory.h"

class ACubeActor;

/**
 * @brief CubeActor Factory
 */
UCLASS()
class UCubeActorFactory :
	public UActorFactory
{
	GENERATED_BODY()
	DECLARE_CLASS(UCubeActorFactory, UActorFactory)

public:
	UCubeActorFactory();
	~UCubeActorFactory() override = default;

protected:
	TObjectPtr<AActor> CreateNewActor() override;
};
