#pragma once
#include "Core/Public/Object.h"

UCLASS()
class UPathManager :
	public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(UPathManager, UObject)

public:
	void Init();

	// Base Path
	const path& GetRootPath() const { return RootPath; }
	const path& GetAssetPath() const { return AssetPath; }

	// Detailed Asset Path
	const path& GetShaderPath() const { return ShaderPath; }
	const path& GetTexturePath() const { return TexturePath; }
	const path& GetModelPath() const { return ModelPath; }
	const path& GetAudioPath() const { return AudioPath; }
	const path& GetWorldPath() const { return WorldPath; }
	const path& GetConfigPath() const { return ConfigPath; }
	const path& GetFontPath() const { return FontPath; }

private:
	path RootPath;
	path AssetPath;
	path ShaderPath;
	path TexturePath;
	path ModelPath;
	path AudioPath;
	path WorldPath;
	path ConfigPath;
	path FontPath;

	void InitializeRootPath();
	void GetEssentialPath();
	void ValidateAndCreateDirectories() const;
};
