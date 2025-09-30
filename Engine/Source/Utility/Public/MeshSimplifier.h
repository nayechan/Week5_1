#pragma once
#include "Core/Public/Object.h"
#include "Global/Matrix.h"
#include "Global/Vector.h"

struct FStaticMesh;

UCLASS()
class UMeshSimplifier : public UObject
{
	GENERATED_BODY()

public:
	// Internal struct to hold vertex data during simplification
	struct FSimplifyVertex
	{
		FVector Position;
		FVector Normal;
		FVector4 Color;
		FVector2 TexCoord;
		FMatrix Quadric;
		bool bIsActive = true;
	};

	// Internal struct to hold face data
	struct FSimplifyFace
	{
		int V[3];	// Indices into the vertex array
		bool bIsActive = true;
	};

	// Internal struct for an edge collapse candidate
	struct FCollapseCandidate
	{
		int V1;
		int V2;
		float Cost;
		FVector OptimalPosition;

		bool operator>(const FCollapseCandidate& Other) const
		{
			return Cost > Other.Cost;
		}
	};

public:
	/**
	 * @brief Simplifies a static mesh using the Quadric Error Metrics (QEM) algorithm.
	 * @param InOriginalMesh The original mesh to simplify.
	 * @param InTargetRatio The target simplification ratio (0.0 to 1.0). 0.5 means reducing to 50% of the original triangle count.
	 * @return A new, simplified FStaticMesh. The caller is responsible for managing the memory of the returned object.
	 */
	static FStaticMesh* Simplify(const FStaticMesh* InOriginalMesh, float InTargetRatio);
};
