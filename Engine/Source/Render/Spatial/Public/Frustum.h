#pragma once
#include "Global/Vector.h"
#include "Global/Matrix.h"
#include "Global/Types.h"
#include "Global/Constant.h"
#include "Physics/Public/AABB.h"

/**
 * @brief 3D Plane (Ax + By + Cz + D = 0)
 */
struct FPlane
{
	FVector Normal;   // (A,B,C), normalized
	float Distance;   // (D), distance from origin

	FPlane() : Normal(0, 0, 0), Distance(0) {}
	FPlane(const FVector& InNormal, float InDistance) : Normal(InNormal), Distance(InDistance) {}
	FPlane(const FVector& InNormal, const FVector& InPoint)
	{
		Normal = InNormal;
		Normal.Normalize();
		Distance = -Normal.Dot(InPoint);
	}

	/**
	 * @brief signed distance from point to plane
	 * > 0 → in front of plane
	 * < 0 → behind plane
	 */
	float GetDistanceToPoint(const FVector& Point) const
	{
		return Normal.Dot(Point) + Distance;
	}

	/** @brief normalize plane equation */
	void Normalize()
	{
		float Length = Normal.Length();
		if (Length > MATH_EPSILON)
		{
			Normal.X /= Length;
			Normal.Y /= Length;
			Normal.Z /= Length;
			Distance /= Length;
		}
	}
};

/**
 * @brief View frustum defined by 6 planes
 * Support: LH (DirectX)
 */
struct FFrustum
{
	enum EFrustumPlane
	{
		Near = 0,
		Far = 1,
		Left = 2,
		Right = 3,
		Top = 4,
		Bottom = 5,
		PlaneCount = 6
	};

	FPlane Planes[PlaneCount];

	void ConstructFromViewProjectionMatrix(const FMatrix& ViewProjMatrix);

	bool IsBoxInFrustum(const FAABB& Box) const;
	bool IsPointInFrustum(const FVector& Point) const;
	bool IsSphereInFrustum(const FVector& Center, float Radius) const;
};
