#include "pch.h"
#include "Render/UI/Widget/Public/StaticMeshComponentWidget.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Component/Mesh/Public/StaticMesh.h"
#include "Manager/UI/Public/UIManager.h"
#include "Core/Public/ObjectIterator.h"
#include "Texture/Public/Material.h"
#include "Texture/Public/Texture.h"

IMPLEMENT_CLASS(UStaticMeshComponentWidget, UWidget)

void UStaticMeshComponentWidget::RenderWidget()
{
    TObjectPtr<UObject> SelectedObject = UUIManager::GetInstance().GetSelectedObject();
    UStaticMeshComponent* TargetComponent = Cast<UStaticMeshComponent>(SelectedObject.Get());

    if (!TargetComponent || !TargetComponent->GetStaticMesh())
    {
        return;
    }

    RenderStaticMeshSelector(TargetComponent);
    ImGui::Separator();
    RenderMaterialSections(TargetComponent);
    ImGui::Separator();
    RenderOptions(TargetComponent);
}

void UStaticMeshComponentWidget::RenderStaticMeshSelector(UStaticMeshComponent* InTargetComponent)
{
    UStaticMesh* CurrentStaticMesh = InTargetComponent->GetStaticMesh();
    const FName PreviewName = CurrentStaticMesh ? CurrentStaticMesh->GetAssetPathFileName() : "None";

    if (ImGui::BeginCombo("Static Mesh", PreviewName.ToString().c_str()))
    {
        for (TObjectIterator<UStaticMesh> It; It; ++It)
        {
            UStaticMesh* MeshInList = *It;
            if (!MeshInList) continue;
            const bool bIsSelected = (CurrentStaticMesh == MeshInList);
            if (ImGui::Selectable(MeshInList->GetAssetPathFileName().ToString().c_str(), bIsSelected))
            {
                InTargetComponent->SetStaticMesh(MeshInList->GetAssetPathFileName());
            }
            if (bIsSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
}

void UStaticMeshComponentWidget::RenderMaterialSections(UStaticMeshComponent* InTargetComponent)
{
    UStaticMesh* CurrentMesh = InTargetComponent->GetStaticMesh();
    FStaticMesh* MeshAsset = CurrentMesh->GetLOD(0);

    ImGui::Text("Material Slots (%d)", static_cast<int>(MeshAsset->MaterialInfo.size()));

    for (int32 SlotIndex = 0; SlotIndex < MeshAsset->MaterialInfo.size(); ++SlotIndex)
    {
        UMaterial* CurrentMaterial = InTargetComponent->GetMaterial(SlotIndex);
        FString PreviewName = CurrentMaterial ? GetMaterialDisplayName(CurrentMaterial) : "None";

        ImGui::PushID(SlotIndex);
        std::string Label = "Element " + std::to_string(SlotIndex);
        if (ImGui::BeginCombo(Label.c_str(), PreviewName.c_str()))
        {
            RenderAvailableMaterials(InTargetComponent, SlotIndex);
            ImGui::EndCombo();
        }
        ImGui::PopID();
    }
}

void UStaticMeshComponentWidget::RenderAvailableMaterials(UStaticMeshComponent* InTargetComponent, int32 TargetSlotIndex)
{
    for (TObjectIterator<UMaterial> It; It; ++It)
    {
        UMaterial* Mat = *It;
        if (!Mat) continue;
        FString MatName = GetMaterialDisplayName(Mat);
        bool bIsSelected = (InTargetComponent->GetMaterial(TargetSlotIndex) == Mat);
        if (ImGui::Selectable(MatName.c_str(), bIsSelected))
        {
            InTargetComponent->SetMaterial(TargetSlotIndex, Mat);
        }
        if (bIsSelected)
        {
            ImGui::SetItemDefaultFocus();
        }
    }
}

void UStaticMeshComponentWidget::RenderOptions(UStaticMeshComponent* InTargetComponent)
{
    bool bIsScrollEnabled = InTargetComponent->IsScrollEnabled();
    if (ImGui::Checkbox("Enable Scroll", &bIsScrollEnabled))
    {
        if (bIsScrollEnabled)
        {
            InTargetComponent->EnableScroll();
        }
        else
        {
            InTargetComponent->DisableScroll();
        }
    }
}

FString UStaticMeshComponentWidget::GetMaterialDisplayName(UMaterial* Material) const
{
    if (!Material) return "None";
    FString ObjectName = Material->GetName().ToString();
    if (!ObjectName.empty() && ObjectName.find("Object_") != 0) return ObjectName;
    UTexture* DiffuseTexture = Material->GetDiffuseTexture();
    if (DiffuseTexture)
    {
        FString TexturePath = DiffuseTexture->GetFilePath().ToString();
        if (!TexturePath.empty())
        {
            size_t LastSlash = TexturePath.find_last_of("/\\");
            size_t LastDot = TexturePath.find_last_of(".");
            if (LastSlash != std::string::npos)
            {
                FString FileName = TexturePath.substr(LastSlash + 1);
                if (LastDot != std::string::npos && LastDot > LastSlash)
                {
                    FileName = FileName.substr(0, LastDot - LastSlash - 1);
                }
                return FileName + " (Mat)";
            }
        }
    }
    return "Material_" + std::to_string(Material->GetUUID());
}
