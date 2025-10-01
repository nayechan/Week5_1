#include "pch.h"
#include "Component/Public/BillboardComponent.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Texture/Public/Texture.h"
#include "Editor/Public/Camera.h"

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

	constexpr float EPS = 1e-6f;
	constexpr float RAD2DEG = 180.0f / 3.14159265358979323846f;

	// World position of this component (owner location + component relative location).
	// For attached components use GetWorldTransformMatrix() if you need full hierarchy correctness.
	const FVector WorldPos = GetOwner()->GetActorLocation() + GetRelativeLocation();

	// Forward: direction from billboard to camera (pointing toward camera)
	FVector Forward = InCamera->GetLocation() - WorldPos;
	const float DistSq = Forward.LengthSquared();
	if (DistSq < EPS) return;
	Forward.Normalize();

	// Use camera's up vector so billboard also follows camera roll/tilt
	FVector CamUp = InCamera->GetUp();

	// If camera up is (almost) parallel to Forward, fallback to world up
	const FVector WorldUp = FVector{ 0.0f, 0.0f, 1.0f };
	if (fabsf(CamUp.Dot(Forward)) > 0.999f)
	{
		CamUp = WorldUp;
	}

	// Right = CamUp x Forward
	FVector Right = CamUp.Cross(Forward);
	if (Right.LengthSquared() < EPS)
	{
		// Degenerate — pick an arbitrary right axis
		Right = FVector{ 1.0f, 0.0f, 0.0f };
	}
	else
	{
		Right.Normalize();
	}

	// Recompute Up to ensure orthonormal basis
	FVector Up = Forward.Cross(Right);
	Up.Normalize();

	// Extract Euler angles (engine expects (Roll, Pitch, Yaw) => (X, Y, Z))
	// Yaw  = rotation about Z so forward projected onto XY-plane
	const float yawRad = atan2f(Forward.Y, Forward.X);
	const float yawDeg = yawRad * RAD2DEG;

	// Pitch = rotation about Y (tilt up/down). Use forward Z and horizontal length.
	const float horizLen = sqrtf(Forward.X * Forward.X + Forward.Y * Forward.Y);
	const float pitchRad = atan2f(-Forward.Z, horizLen); // sign chosen to match existing convention
	const float pitchDeg = pitchRad * RAD2DEG;

	// Roll = rotation about X (twist). Derive from right/up vertical components.
	const float rollRad = atan2f(Right.Z, Up.Z);
	const float rollDeg = rollRad * RAD2DEG;

	if (bYawOnly)
	{
		// Keep upright, only apply yaw (put yaw into Z)
		SetRelativeRotation(FVector(0.0f, 0.0f, yawDeg));
	}
	else
	{
		// Store as (Roll, Pitch, Yaw) -> (X, Y, Z)
		SetRelativeRotation(FVector(rollDeg, pitchDeg, yawDeg));
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
}
