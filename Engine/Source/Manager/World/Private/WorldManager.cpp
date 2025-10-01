#include "pch.h"
#include "Manager/World/Public/WorldManager.h"

#include "Core/Public/World.h"
#include "Level/Public/Level.h"
#include "Factory/Public/NewObject.h"
#include "Manager/Time/Public/TimeManager.h"

IMPLEMENT_SINGLETON_CLASS_BASE(UWorldManager)

UWorldManager::UWorldManager()
{
	CreateEditorWorld();
}

UWorldManager::~UWorldManager()
{
	Shutdown();
}

void UWorldManager::Update(float DeltaTime) const
{
	if (CurrentWorld)
	{
		CurrentWorld->Tick(DeltaTime);
	}
}

void UWorldManager::Shutdown()
{
	if (CurrentWorld)
	{
		CurrentWorld->CleanupWorld();
		delete CurrentWorld;
		CurrentWorld = nullptr;
	}
}

void UWorldManager::CreateEditorWorld()
{
	// 에디터 월드 생성
	CurrentWorld = NewObject<UWorld>(nullptr, UWorld::StaticClass(), FName("EditorWorld"));
	if (CurrentWorld)
	{
		CurrentWorld->SetWorldType(EWorldType::Editor);
	}
}

void UWorldManager::SetCurrentLevel(ULevel* InLevel)
{
	if (CurrentWorld && InLevel)
	{
		CurrentWorld->SetLevel(InLevel);
	}
}

ULevel* UWorldManager::GetCurrentLevel() const
{
	if (CurrentWorld)
	{
		return CurrentWorld->GetLevel();
	}
	return nullptr;
}
