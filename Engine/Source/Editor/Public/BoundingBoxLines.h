#pragma once
#include "Core/Public/Object.h"
#include "Physics/Public/AABB.h"

class UBoundingBoxLines : UObject
{
public:
	UBoundingBoxLines();
	~UBoundingBoxLines() = default;

	void MergeVerticesAt(TArray<FVector>& destVertices, size_t insertStartIndex);
	void UpdateVertices(FAABB boundingBoxInfo);

	uint32 GetNumVertices() const
	{
		return NumVertices;
	}

	FAABB GetRenderedBoxInfo() const
	{
		return RenderedBoxInfo;
	}

private:
	TArray<FVector> Vertices;
	uint32 NumVertices = 8;
	FAABB RenderedBoxInfo;
};

