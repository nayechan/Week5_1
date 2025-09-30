#pragma once
#include "Editor/Public/Gizmo.h"
#include "Utility/Public/StaticMeshBVH.h"

class UPrimitiveComponent;
class AActor;
class ULevel;
class UCamera;
class UGizmo;
struct FRay;

class UObjectPicker : public UObject
{
public:
	UObjectPicker() = default;
	UPrimitiveComponent* PickPrimitive(UCamera* InActiveCamera, const FRay& WorldRay, TArray<UPrimitiveComponent*>& Candidate, float* Distance);
	bool PickPrimitive(UCamera* InActiveCamera, const FRay& WorldRay, UPrimitiveComponent* Primitive, float* OutDistance);
	void PickGizmo(UCamera* InActiveCamera, const FRay& WorldRay, UGizmo& Gizmo, FVector& CollisionPoint);
	bool IsRayCollideWithPlane(const FRay& WorldRay, FVector PlanePoint, FVector Normal, FVector& PointOnPlane);

private:
	bool IsRayPrimitiveCollided(UCamera* InActiveCamera, const FRay& ModelRay, UPrimitiveComponent* Primitive, const FMatrix& ModelMatrix, float* ShortestDistance);
	// Updated: pass precomputed per-primitive world direction length and worldDir dot camera forward.
	bool IsRayTriangleCollided(UCamera* InActiveCamera, const FRay& Ray, const FVector& Vertex1, const FVector& Vertex2, const FVector& Vertex3, float WorldDirLen, float WorldDirDotCam, float* Distance);

	FRay GetModelRay(const FRay& Ray, UPrimitiveComponent* Primitive);
	bool IsRayTriangleCollided(UCamera* InActiveCamera, const FRay& Ray, const FVector& Vertex1, const FVector& Vertex2, const FVector& Vertex3,
		const FMatrix& ModelMatrix, float* Distance) = delete; // avoid old overload
};
