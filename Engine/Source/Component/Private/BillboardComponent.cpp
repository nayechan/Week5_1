#include "pch.h"
#include "Component/Public/BillboardComponent.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Texture/Public/Texture.h"
#include "Editor/Public/Camera.h"
#include "Render/UI/Widget/Public/BillboardComponentWidget.h"
#include "Manager/Level/Public/LevelManager.h"

IMPLEMENT_CLASS(UBillboardComponent, UPrimitiveComponent)

UBillboardComponent::UBillboardComponent()
{
	UAssetManager& ResourceManager = UAssetManager::GetInstance();
	Type = EPrimitiveType::Billboard;
	Vertices = ResourceManager.GetVertexData(Type);
	VertexBuffer = ResourceManager.GetVertexbuffer(Type);
	NumVertices = ResourceManager.GetNumVertices(Type);
	RenderState.CullMode = ECullMode::None;
	RenderState.FillMode = EFillMode::Solid;
	BoundingBox = &ResourceManager.GetAABB(Type);
}

UBillboardComponent::~UBillboardComponent()
{
}

void UBillboardComponent::UpdateFacingCamera(const UCamera* InCamera, bool bYawOnly)
{
	if (!InCamera) return;
	if (!GetOwner()) return;

	constexpr float EPS = 1e-8f;
	constexpr float RAD2DEG = 180.0f / 3.14159265358979323846f;

	// Use the component's world transform so attachments/parents are respected
	const FMatrix WorldTM = GetWorldTransform();
	const FVector WorldPos = WorldTM.GetLocation();

	// Direction from billboard to camera in world space
	FVector ToCamera = InCamera->GetLocation() - WorldPos;
	if (ToCamera.LengthSquared() < EPS)
	{
		// Camera is essentially on the billboard — nothing to do
		return;
	}
	ToCamera.Normalize();

	// Compute yaw from projection on XY plane (Z-up, X-forward, Y-right)
	const float yawRad = atan2f(ToCamera.Y, ToCamera.X);
	const float yawDeg = yawRad * RAD2DEG;

	if (bYawOnly)
	{
		// Keep upright: roll = 0, pitch = 0, yaw in Z component
		// Engine rotation vector layout used here: (Roll, Pitch, Yaw) -> (X, Y, Z)
		SetRelativeRotation(FVector(0.0f, 0.0f, yawDeg));
	}
	else
	{
		// Full facing but keep upright (no roll)
		const float horizLen = sqrtf(ToCamera.X * ToCamera.X + ToCamera.Y * ToCamera.Y);
		const float pitchRad = atan2f(-ToCamera.Z, horizLen); // sign chosen to match engine convention
		const float pitchDeg = pitchRad * RAD2DEG;

		// Store as (Roll, Pitch, Yaw) -> (X, Y, Z); roll forced to 0 to avoid twist
		SetRelativeRotation(FVector(0.0f, pitchDeg, yawDeg));
	}

	MarkAsDirty();
}

void UBillboardComponent::SetSprite(const FName& InFilePath)
{
	// Clear sprite when no path provided
	if (InFilePath == FName::GetNone())
	{
		Sprite = nullptr;
		return;
	}

	// If already set to the same file, do nothing
	if (Sprite)
	{
		// Compare using file path string to avoid depending on UTexture internals
		// GetFilePath returns a string representation; guard against nullptr.
		try
		{
			if (Sprite->GetFilePath() == InFilePath.ToString())
			{
				return;
			}
		}
		catch (...)
		{
			// fall through and attempt to set new sprite
		}
	}

	UAssetManager& AssetManager = UAssetManager::GetInstance();

	// CreateTexture will load or create a UTexture wrapper and associated SRV.
	UTexture* NewTex = AssetManager.CreateTexture(InFilePath);

	if (!NewTex)
	{
		UE_LOG("UBillboardComponent::SetSprite: Failed to create or load texture '%s'", InFilePath.ToString().c_str());
		Sprite = nullptr;
		return;
	}

	Sprite = NewTex;
	UE_LOG("UBillboardComponent::SetSprite: Assigned texture '%s' to billboard", InFilePath.ToString().c_str());

	MarkWorldAABBDirty();
	if (UEditor* Editor = ULevelManager::GetInstance().GetEditor())
	{
		Editor->MarkPrimitiveDirty(this);
	}
}

TObjectPtr<UClass> UBillboardComponent::GetSpecificWidgetClass() const
{
	return UBillboardComponentWidget::StaticClass();
}
