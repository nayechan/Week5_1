#pragma once
#include "Core/Public/Object.h"

class UWorld;
class ULevel;

UCLASS()
class UWorldManager :
	public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(UWorldManager, UObject)

public:
	void Update(float DeltaTime) const;
	void Shutdown();

	// World 관리
	void CreateEditorWorld();
	void SetCurrentLevel(ULevel* InLevel);

	// Getter
	TObjectPtr<UWorld> GetCurrentWorld() const { return CurrentWorld; }
	ULevel* GetCurrentLevel() const;
	
	// PIE Duplication Guard
	bool IsDuplicatingForPIE() const { return bIsDuplicatingForPIE; }
	void SetDuplicatingForPIE(bool bInDuplicating) { bIsDuplicatingForPIE = bInDuplicating; }

private:
	TObjectPtr<UWorld> CurrentWorld;
	bool bIsDuplicatingForPIE = false;
};