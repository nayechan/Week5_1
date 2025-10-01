#include "pch.h"

#include <string>
#include <filesystem>

#include <Windows.h>
#include <ShObjIdl.h>

#include "Actor/Public/Actor.h"
#include "Actor/Public/StaticMeshActor.h"
#include "Core/Public/ObjectIterator.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Global/Macro.h"
#include "Level/Public/Level.h"
#include "Manager/Level/Public/LevelManager.h"
#include "Utility/Public/FileDialog.h"

std::filesystem::path OpenFileDialog()
{
	IFileOpenDialog* FileDialog = nullptr;
	HRESULT Result = CoCreateInstance(
		CLSID_FileOpenDialog,
		nullptr,
		CLSCTX_ALL,
		IID_IFileOpenDialog,
		reinterpret_cast<void**>(&FileDialog)
	);

	if (FAILED(Result) || !FileDialog)
		return {};

	Result = FileDialog->Show(nullptr);
	if (FAILED(Result))
	{
		FileDialog->Release();
		return {};
	}

	IShellItem* ShellItem = nullptr;
	Result = FileDialog->GetResult(&ShellItem);
	if (FAILED(Result) || !ShellItem)
	{
		FileDialog->Release();
		return {};
	}

	PWSTR RawFilePath = nullptr;
	Result = ShellItem->GetDisplayName(SIGDN_FILESYSPATH, &RawFilePath);
	if (FAILED(Result) || !RawFilePath)
	{
		ShellItem->Release();
		FileDialog->Release();
		return {};
	}

	std::filesystem::path SelectedFilePath(RawFilePath);
	CoTaskMemFree(RawFilePath);

	ShellItem->Release();
	FileDialog->Release();

	return SelectedFilePath;
}

bool OpenObjFromFileDialog()
{
	std::filesystem::path FilePath = OpenFileDialog();

	if (!std::filesystem::exists(FilePath))
	{
		UE_LOG_ERROR("해당 파일이 존재하지 않습니다.");
		return false;
	}

	if (FilePath.extension() != ".obj")
	{
		UE_LOG_ERROR("지원하지 않는 파일 형식입니다: %s", FilePath.extension().c_str());
		return false;
	}

	ULevelManager& LevelManager = ULevelManager::GetInstance();
	ULevel* CurrentLevel = LevelManager.GetCurrentLevel();

	if (!CurrentLevel)
	{
		UE_LOG_ERROR("현재 레벨이 존재하지 않습니다.");
		return false;
	}

	AActor* NewActor = CurrentLevel->SpawnActorToLevel(AStaticMeshActor::StaticClass());
	UStaticMeshComponent* StaticMeshComponent = nullptr;
	for (const auto& Component : NewActor->GetOwnedComponents())
	{
		StaticMeshComponent = Cast<UStaticMeshComponent>(Component);

		if (StaticMeshComponent)
		{
			break;
		}
	}

	if (!StaticMeshComponent)
	{
		UE_LOG_ERROR("스태틱 메시 컴포넌트가 존재하지 않습니다.");
		return false;
	}

	for (TObjectIterator<UStaticMesh> It; It; ++It)
	{
		UStaticMesh* MeshInList = *It;
		if (!MeshInList) { continue; }

		std::filesystem::path AssetPath = std::filesystem::current_path() / MeshInList->GetAssetPathFileName().ToString();
		if (AssetPath == FilePath)
		{
			StaticMeshComponent->SetStaticMesh(MeshInList->GetAssetPathFileName());

			return true;
		}
	}

	UE_LOG_ERROR("해당하는 파일을 찾지 못했습니다: %s", FilePath.filename().string().c_str());
	return false;
}
