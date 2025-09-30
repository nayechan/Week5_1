#pragma once
#include "Core/Public/Object.h"

class UEditor;
class ULevel;

UCLASS()
class ULevelManager :
	public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(ULevelManager, UObject)

public:
	void Update() const;
	bool LoadLevel(const FString& InFilePath);
	void Shutdown();

	// Getter
	TObjectPtr<ULevel> GetCurrentLevel() const { return CurrentLevel; }

	// Save & Load System
	bool SaveCurrentLevel(const FString& InFilePath) const;
	bool CreateNewLevel(const FString& InLevelName = "Untitled");
	static path GetLevelDirectory();
	static path GenerateLevelFilePath(const FString& InLevelName);

	TObjectPtr<UEditor> GetEditor() const { return Editor; }

private:
	void SwitchToLevel(ULevel* InNewLevel);

private:
	TObjectPtr<ULevel> CurrentLevel;
	TObjectPtr<UEditor> Editor;
};
