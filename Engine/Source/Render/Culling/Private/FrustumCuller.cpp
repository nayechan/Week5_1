#include "pch.h"
#include "Render/Culling/Public/FrustumCuller.h"
#include "Render/Spatial/Public/Frustum.h"
#include "Physics/Public/AABB.h"
#include "Editor/Public/Camera.h"

IMPLEMENT_SINGLETON_CLASS_BASE(UFrustumCuller)

UFrustumCuller::UFrustumCuller() = default;
UFrustumCuller::~UFrustumCuller() = default;

bool UFrustumCuller::IsInFrustum(const FAABB& Bounds, const FFrustum& Frustum) const
{
	return Frustum.IsBoxInFrustum(Bounds);
}

bool UFrustumCuller::IsInFrustum(const FVector& WorldMin, const FVector& WorldMax, const FFrustum& Frustum) const
{
	FAABB bounds(WorldMin, WorldMax);
	return Frustum.IsBoxInFrustum(bounds);
}
