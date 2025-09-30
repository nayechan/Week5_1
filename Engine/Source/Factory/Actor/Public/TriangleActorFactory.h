#pragma once
#include "Factory/Actor/Public/ActorFactory.h"

class ATriangleActor;

/**
 * @brief TriangleActor Factory
 */
UCLASS()
class UTriangleActorFactory :
	public UActorFactory
{
	GENERATED_BODY()
	DECLARE_CLASS(UTriangleActorFactory, UActorFactory)

public:
	UTriangleActorFactory();
	~UTriangleActorFactory() override = default;

protected:
	TObjectPtr<AActor> CreateNewActor() override;
};
