#pragma once
#include "Core/Public/Object.h"

class AActor;
class UWidget;

UCLASS()
class UActorComponent : public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UActorComponent, UObject)

public:
	UActorComponent();
	~UActorComponent();
	/*virtual void Render(const URenderer& Renderer) const
	{

	}*/

	virtual void BeginPlay();
	virtual void TickComponent();
	virtual void EndPlay();

	EComponentType GetComponentType() { return ComponentType; }

	void SetOwner(AActor* InOwner) { Owner = InOwner; }
	AActor* GetOwner() const { return Owner; }

	EComponentType GetComponentType() const { return ComponentType; }

	virtual TObjectPtr<UClass> GetSpecificWidgetClass() const;

protected:
	EComponentType ComponentType;
private:
	AActor* Owner;
};
