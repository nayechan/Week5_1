#pragma once
#include "Factory/Public/Factory.h"

class AActor;
class ULevel;

/**
 * @brief Actor Factory
 */
UCLASS()
class UActorFactory :
	public UFactory
{
	GENERATED_BODY()
	DECLARE_CLASS(UActorFactory, UFactory)

public:
	virtual TObjectPtr<AActor> CreateActor(TObjectPtr<UObject> InWorld, TObjectPtr<ULevel> InLevel,
	                            const FTransform& InTransform = FTransform(), uint32 InObjectFlags = 0);

	bool IsActorFactory() const override { return true; }

	// Special member function
	UActorFactory();
	~UActorFactory() override = default;

protected:
	// Inheritable function
	virtual TObjectPtr<AActor> CreateNewActor() { return nullptr; }
	TObjectPtr<UObject> CreateNew() override;

	virtual void PostCreateActor(AActor* InActor, const FTransform& InTransform);
};
