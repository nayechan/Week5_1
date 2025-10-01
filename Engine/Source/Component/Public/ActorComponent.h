#pragma once
#include "Core/Public/Object.h"

// Forward declaration
namespace EEndPlayReason { enum Type; }

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
	virtual void TickComponent(float DeltaTime);
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);

	// Tick 상태 관리
	bool IsComponentTickEnabled() const { return bComponentTickEnabled; }
	void SetComponentTickEnabled(bool bEnabled) { bComponentTickEnabled = bEnabled; }

	EComponentType GetComponentType() { return ComponentType; }

	void SetOwner(AActor* InOwner) { Owner = InOwner; }
	AActor* GetOwner() const { return Owner; }

	EComponentType GetComponentType() const { return ComponentType; }

	virtual TObjectPtr<UClass> GetSpecificWidgetClass() const;

protected:
	EComponentType ComponentType;
	bool bComponentTickEnabled = true;

private:
	AActor* Owner;
};
