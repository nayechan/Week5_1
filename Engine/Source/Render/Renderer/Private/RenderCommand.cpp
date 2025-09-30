#include "pch.h"
#include "Render/Renderer/Public/RenderCommand.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Texture/Public/Material.h"

uint64 FRenderCommand::BuildSortKey(UPrimitiveComponent* Primitive)
{
	if (!Primitive)
		return 0;

	uint64 SortKey = 0;

	// Shader Type 결정 (상위 16비트)
	EPrimitiveType PrimType = Primitive->GetPrimitiveType();
	uint16 ShaderType = 0;

	switch (PrimType)
	{
	case EPrimitiveType::StaticMesh:
		ShaderType = 1;  // Texture Shader
		break;
	case EPrimitiveType::BillBoard:
		ShaderType = 2;  // Billboard Shader
		break;
	default:
		ShaderType = 0;  // Default Shader
		break;
	}

	SortKey |= (static_cast<uint64>(ShaderType) << 48);

	// Material ID 추출 (중간 16비트)
	if (PrimType == EPrimitiveType::StaticMesh)
	{
		UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(Primitive);
		if (MeshComp)
		{
			UMaterial* Material = MeshComp->GetMaterial(0);  // 첫 번째 Material 사용
			if (Material)
			{
				// Material의 고유 ID 또는 해시값 사용
				uint16 MaterialID = static_cast<uint16>(Material->GetUUID() & 0xFFFF);
				SortKey |= (static_cast<uint64>(MaterialID) << 32);

				// Texture Hash 추출 (하위 16비트)
				// 간단하게 Diffuse Texture의 포인터 해시 사용
				if (Material->GetDiffuseTexture())
				{
					uint16 TextureHash = static_cast<uint16>(
						reinterpret_cast<uintptr_t>(Material->GetDiffuseTexture()) & 0xFFFF);
					SortKey |= (static_cast<uint64>(TextureHash) << 16);
				}
			}
		}
	}

	// [15:0]은 예약 (투명 객체 깊이 정렬용)

	return SortKey;
}