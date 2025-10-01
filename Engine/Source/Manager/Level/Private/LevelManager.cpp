#include "pch.h"
#include "Manager/Level/Public/LevelManager.h"

#include "Level/Public/Level.h"
#include "Manager/Path/Public/PathManager.h"
#include "Utility/Public/JsonSerializer.h"
#include "Editor/Public/Editor.h"
#include "Manager/Config/Public/ConfigManager.h"

#include <json.hpp>

using JSON = json::JSON;

IMPLEMENT_SINGLETON_CLASS_BASE(ULevelManager)

// =================================================================
// Public Functions
// =================================================================

ULevelManager::ULevelManager()
{
	Editor = new UEditor;
}

ULevelManager::~ULevelManager()
{
	// 소멸자에서 Shutdown을 호출하여 어떤 경우에도 리소스가 누수되지 않도록 보장합니다.
	Shutdown();
}

void ULevelManager::Shutdown()
{
	// 현재 레벨 정리
	if (CurrentLevel)
	{
		delete CurrentLevel;
		CurrentLevel = nullptr;
	}

	if (Editor)
	{
		delete Editor;
		Editor = nullptr;
	}
}

void ULevelManager::Update() const
{
	if (CurrentLevel)
	{
		CurrentLevel->Update();
	}
	if (Editor)
	{
		Editor->Update();
	}
}

bool ULevelManager::SaveCurrentLevel(const FString& InFilePath) const
{
	if (!CurrentLevel)
	{
		UE_LOG("LevelManager: No Current Level To Save");
		return false;
	}

	path FilePath = InFilePath;
	if (FilePath.empty())
	{
		FilePath = GenerateLevelFilePath(CurrentLevel->GetName() == FName::GetNone()
			? "Untitled"
			: CurrentLevel->GetName().ToString());
	}

	UE_LOG("LevelManager: 현재 레벨을 다음 경로에 저장합니다: %s", FilePath.string().c_str());

	try
	{
		JSON LevelJson;
		CurrentLevel->Serialize(false, LevelJson);
		bool bSuccess = FJsonSerializer::SaveJsonToFile(LevelJson, FilePath.string());

		if (bSuccess)
		{
			UConfigManager::GetInstance().SetLastUsedLevelPath(InFilePath);	// 재시작 시 다시 열기 위해 저장

			UE_LOG("LevelManager: 레벨이 성공적으로 저장되었습니다");
		}
		else
		{
			UE_LOG("LevelManager: 레벨을 저장하는 데에 실패했습니다");
		}
		return bSuccess;
	}
	catch (const exception& Exception)
	{
		UE_LOG("LevelManager: 저장 과정에서 Exception 발생: %s", Exception.what());
		return false;
	}
}

ULevel* ULevelManager::CreateLevel(const FString& InFilePath)
{
	UE_LOG("LevelManager: Creating Level From: %s", InFilePath.data());

	path FilePath(InFilePath);
	FString LevelName = FilePath.stem().string();

	ULevel* NewLevel = new ULevel(LevelName);
	try
	{
		JSON LevelJsonData;
		if (FJsonSerializer::LoadJsonFromFile(LevelJsonData, InFilePath))
		{
			NewLevel->Serialize(true, LevelJsonData);
		}
		else
		{
			UE_LOG("LevelManager: Failed To Load Level From: %s", InFilePath.c_str());
			delete NewLevel;
			return nullptr;
		}
	}
	catch (const exception& InException)
	{
		UE_LOG("LevelManager: Exception During Load: %s", InException.what());
		delete NewLevel;
		return nullptr;
	}

	UE_LOG("LevelManager: Level '%s' Created Successfully", LevelName.c_str());
	return NewLevel;
}

bool ULevelManager::LoadLevel(const FString& InFilePath)
{
	UE_LOG("LevelManager: Loading Level: %s", InFilePath.data());

	ULevel* NewLevel = CreateLevel(InFilePath);
	if (!NewLevel)
	{
		return false;
	}

	// 재시작 시 다시 열기 위해 저장
	UConfigManager::GetInstance().SetLastUsedLevelPath(InFilePath);

	// 새 레벨을 등록하고 활성 레벨로 전환합니다.
	SwitchToLevel(NewLevel);

	UE_LOG("LevelManager: Level '%s' (으)로 레벨을 교체 완료했습니다", NewLevel->GetName().ToString().c_str());
	return true;
}

bool ULevelManager::CreateNewLevel(const FString& InLevelName)
{
	// 새 레벨을 생성하고 등록합니다.
	TObjectPtr<ULevel> NewLevel = TObjectPtr(new ULevel(InLevelName));
	// 새 레벨로 전환합니다.
	SwitchToLevel(NewLevel);
	return true;
}

path ULevelManager::GetLevelDirectory()
{
	UPathManager& PathManager = UPathManager::GetInstance();
	return PathManager.GetWorldPath();
}

path ULevelManager::GenerateLevelFilePath(const FString& InLevelName)
{
	path LevelDirectory = GetLevelDirectory();
	path FileName = InLevelName + ".json";
	return LevelDirectory / FileName;
}


// =================================================================
// Private Helper Functions
// =================================================================

// Level 전환
void ULevelManager::SwitchToLevel(ULevel* InNewLevel)
{
	if (CurrentLevel)
	{
		delete CurrentLevel;
	}

	CurrentLevel = InNewLevel;

	// 새로운 레벨로 전환할 때 Editor의 BVH를 리셋
	if (Editor)
	{
		Editor->ResetBVH();
	}

	if (CurrentLevel)
	{
		CurrentLevel->Init();
		UE_LOG("LevelManager: Switched to Level '%s'", CurrentLevel->GetName().ToString().c_str());
	}
	else
	{
		UE_LOG("LevelManager: Switched to null level.");
	}
}
