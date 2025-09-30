#pragma once
#include "Factory/Actor/Public/ActorFactory.h"

class ASquareActor;

UCLASS()
class UStaticMeshActorFactory :
	public UActorFactory
{
	GENERATED_BODY()
	DECLARE_CLASS(UStaticMeshActorFactory, UActorFactory)

public:
	UStaticMeshActorFactory();
	~UStaticMeshActorFactory() override = default;

protected:
	TObjectPtr<AActor> CreateNewActor() override;
};
