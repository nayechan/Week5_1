#pragma once
#include "Widget.h"

class UPrimitiveSpawnWidget
	:public UWidget
{
public:
	void Initialize() override;
	void Update() override;
	void RenderWidget() override;
	void SpawnActors() const;

	// Special Member Function
	UPrimitiveSpawnWidget();
	~UPrimitiveSpawnWidget() override;

private:
	EPrimitiveType SelectedPrimitiveType = EPrimitiveType::Sphere;
	int32 NumberOfSpawn = 1;
	float SpawnRangeMin = -10.0f;
	float SpawnRangeMax = 10.0f;
};
