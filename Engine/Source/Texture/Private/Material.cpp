#include "pch.h"
#include "Texture/Public/Material.h"
#include "Texture/Public/Texture.h"
#include "Texture/Public/TextureRenderProxy.h"

IMPLEMENT_CLASS(UMaterial, UObject)

UMaterial::~UMaterial()
{
	SafeDelete(DiffuseTexture);
	SafeDelete(AmbientTexture);
	SafeDelete(SpecularTexture);
	SafeDelete(NormalTexture);
	SafeDelete(AlphaTexture);
	SafeDelete(BumpTexture);
}
