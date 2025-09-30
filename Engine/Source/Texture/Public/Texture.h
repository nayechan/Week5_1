#pragma once
#include "Core/Public/Object.h"

class FTextureRenderProxy;

UCLASS()
class UTexture :
	public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UTexture, UObject)

public:
	// Special member function
	UTexture();
	UTexture(const FName& InFilePath, FName InName);
	~UTexture() override;

	// Getter & Setter
	uint32 GetWidth() const { return Width; }
	uint32 GetHeight() const { return Height; }

	FName GetFilePath() const { return TextureFilePath.ToString();  }

	const FTextureRenderProxy* GetRenderProxy() const { return RenderProxy; }
	void SetRenderProxy(FTextureRenderProxy* InProxy) { RenderProxy = InProxy; }

private:
	FName TextureFilePath;
	uint32 Width = 0;
	uint32 Height = 0;

	FTextureRenderProxy* RenderProxy = nullptr;

	friend class UAssetManager;
};
