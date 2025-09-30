#include "pch.h"
#include "Texture/Public/Texture.h"

IMPLEMENT_CLASS(UTexture, UObject)

/**
 * @brief Default Constructor
 * 따로 사용할 의도는 없지만 매크로 규격에 맞게 추가
 * 생성에 필요한 내용을 갖추도록 구현
 */
UTexture::UTexture()
	: TextureFilePath(FName::GetNone())
{
	SetName(FName::GetNone());
}

UTexture::UTexture(const FName& InFilePath, FName InName)
	: TextureFilePath(InFilePath)
{
	SetName(InName);
}

UTexture::~UTexture()
{
	if (RenderProxy)
	{
		delete RenderProxy;
	}
}
