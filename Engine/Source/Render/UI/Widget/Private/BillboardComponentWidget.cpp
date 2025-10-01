#include "pch.h"
#include "Render/UI/Widget/Public/BillboardComponentWidget.h"
#include "Manager/Level/Public/LevelManager.h"
#include "Level/Public/Level.h"
#include "Core/Public/ObjectIterator.h"
#include "Texture/Public/Texture.h"
#include "Component/Public/BillboardComponent.h"
#include "Manager/Asset/Public/AssetManager.h"

IMPLEMENT_CLASS(UBillboardComponentWidget, UWidget)

void UBillboardComponentWidget::RenderWidget()
{
	TObjectPtr<ULevel> CurrentLevel = ULevelManager::GetInstance().GetCurrentLevel();
	if (!CurrentLevel)
	{
		ImGui::TextUnformatted("No Level Loaded");
		return;
	}
	TObjectPtr<AActor> SelectedActor = CurrentLevel->GetSelectedActor();
	if (!SelectedActor)
	{
		ImGui::TextUnformatted("No Object Selected");
		return;
	}
	for (const TObjectPtr<UActorComponent>& Component : SelectedActor->GetOwnedComponents())
	{
		BillboardComponent = Cast<UBillboardComponent>(Component);
		// 위젯이 편집해야 할 대상 컴포넌트가 유효한지 확인합니다.
		if (BillboardComponent)
		{
			break;
		}
	}
	if (!BillboardComponent)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Target Component is not valid.");
		return;
	}

	RenderSpriteSelector();
	ImGui::Separator();
}

void UBillboardComponentWidget::RenderSpriteSelector()
{
	// Current assigned sprite on the component
	UTexture* CurrentSprite = BillboardComponent->GetSprite();
	const FName PreviewName = CurrentSprite ? CurrentSprite->GetFilePath() : FName("None");

	// Combo labeled "Sprite" showing current selection
	if (ImGui::BeginCombo("Sprite", PreviewName.ToString().c_str()))
	{
		// Iterate all loaded UTexture assets
		for (auto& Pair : UAssetManager::GetInstance().GetAllTextures())
		{
			UTexture* Tex = Pair.second;
			if (!Tex) continue;

			// Display name for the texture (use filepath filename if available)
			FString DisplayName = GetSpriteDisplayName(Tex);
			const bool bIsSelected = (CurrentSprite == Tex);

			// If user selects this texture, assign it to the component
			if (ImGui::Selectable(DisplayName.c_str(), bIsSelected))
			{
				BillboardComponent->SetSprite(Tex->GetFilePath());
			}

			// Keep focus on the currently selected item
			if (bIsSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}

		ImGui::EndCombo();
	}
}

FString UBillboardComponentWidget::GetSpriteDisplayName(UTexture* Sprite) const
{
	if (!Sprite) return "None";

	// Prefer a clean filename from the stored file path
	FString Path = Sprite->GetFilePath().ToString();
	if (Path.empty()) return Sprite->GetName().ToString();

	// Extract file name (without extension)
	size_t LastSlash = Path.find_last_of("/\\");
	size_t LastDot = Path.find_last_of(".");
	if (LastSlash != std::string::npos)
	{
		FString FileName = Path.substr(LastSlash + 1);
		if (LastDot != std::string::npos && LastDot > LastSlash)
		{
			FileName = FileName.substr(0, LastDot - LastSlash - 1);
		}
		return FileName;
	}

	// Fallback to whole path or object name
	if (LastDot != std::string::npos)
	{
		return Path.substr(0, LastDot);
	}
	return Path;
}
