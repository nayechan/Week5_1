#include "pch.h"
#include "Physics/Public/AABB.h"

/**
 * @brief Checks if this AABB completely contains another AABB.
 */
bool FAABB::Contains(const FAABB& Other) const
{
	return Min.X <= Other.Min.X && Min.Y <= Other.Min.Y && Min.Z <= Other.Min.Z &&
		Max.X >= Other.Max.X && Max.Y >= Other.Max.Y && Max.Z >= Other.Max.Z;
}

/**
 * @brief Checks if this AABB intersects with another AABB.
 */
bool FAABB::Intersects(const FAABB& Other) const
{
	return Min.X <= Other.Max.X && Max.X >= Other.Min.X &&
		Min.Y <= Other.Max.Y && Max.Y >= Other.Min.Y &&
		Min.Z <= Other.Max.Z && Max.Z >= Other.Min.Z;
}
