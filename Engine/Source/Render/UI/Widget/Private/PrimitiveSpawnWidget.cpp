#include "pch.h"
#include "Render/UI/Widget/Public/PrimitiveSpawnWidget.h"

#include "Level/Public/Level.h"
#include "Manager/Level/Public/LevelManager.h"
#include "Actor/Public/CubeActor.h"
#include "Actor/Public/SphereActor.h"
#include "Actor/Public/SquareActor.h"
#include "Actor/Public/TriangleActor.h"
#include "Actor/Public/StaticMeshActor.h"
#include "Actor/Public/BillboardActor.h"

UPrimitiveSpawnWidget::UPrimitiveSpawnWidget()
	: UWidget("Primitive Spawn Widget")
{
}

UPrimitiveSpawnWidget::~UPrimitiveSpawnWidget() = default;

void UPrimitiveSpawnWidget::Initialize()
{
	// Do Nothing Here
}

void UPrimitiveSpawnWidget::Update()
{
	// Do Nothing Here
}

void UPrimitiveSpawnWidget::RenderWidget()
{
	ImGui::Text("Primitive Actor 생성");
	ImGui::Spacing();

	// Primitive 타입 선택 DropDown
	const char* PrimitiveTypes[] = {
		"Sphere",
		"Cube",
		"Triangle",
		"Square",
		"StaticMesh",
		"Billboard",
	};

	const int TypeCount = static_cast<int>(sizeof(PrimitiveTypes) / sizeof(PrimitiveTypes[0]));

	// compute combo index from current SelectedPrimitiveType so selection persists
	int TypeNumber = 0;
	switch (SelectedPrimitiveType)
	{
	case EPrimitiveType::Sphere:     TypeNumber = 0; break;
	case EPrimitiveType::Cube:       TypeNumber = 1; break;
	case EPrimitiveType::Triangle:   TypeNumber = 2; break;
	case EPrimitiveType::Square:     TypeNumber = 3; break;
	case EPrimitiveType::StaticMesh: TypeNumber = 4; break;
	case EPrimitiveType::Billboard:  TypeNumber = 5; break;
	default:                        TypeNumber = 0; break;
	}

	ImGui::Text("Primitive Type:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(120);
	if (ImGui::Combo("##PrimitiveType", &TypeNumber, PrimitiveTypes, TypeCount))
	{
		// user changed selection -> update SelectedPrimitiveType
		switch (TypeNumber)
		{
		case 0: SelectedPrimitiveType = EPrimitiveType::Sphere; break;
		case 1: SelectedPrimitiveType = EPrimitiveType::Cube; break;
		case 2: SelectedPrimitiveType = EPrimitiveType::Triangle; break;
		case 3: SelectedPrimitiveType = EPrimitiveType::Square; break;
		case 4: SelectedPrimitiveType = EPrimitiveType::StaticMesh; break;
		case 5: SelectedPrimitiveType = EPrimitiveType::Billboard; break;
		default: SelectedPrimitiveType = EPrimitiveType::Sphere; break;
		}
	}

	// Spawn 버튼과 개수 입력
	ImGui::Text("Number of Spawn:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(80);
	ImGui::InputInt("##NumberOfSpawn", &NumberOfSpawn);
	NumberOfSpawn = max(1, NumberOfSpawn);
	NumberOfSpawn = min(100, NumberOfSpawn);

	ImGui::SameLine();
	if (ImGui::Button("Spawn Actors"))
	{
		SpawnActors();
	}

	// 스폰 범위 설정
	ImGui::Text("Spawn Range:");
	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("Min##SpawnRange", &SpawnRangeMin, 0.1f, -50.0f, SpawnRangeMax - 0.1f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("Max##SpawnRange", &SpawnRangeMax, 0.1f, SpawnRangeMin + 0.1f, 50.0f);

	ImGui::Separator();
}

/**
 * @brief Actor 생성 함수
 * 난수를 활용한 Range, Size, Rotion 값 생성으로 Actor Spawn
 */
void UPrimitiveSpawnWidget::SpawnActors() const
{
	ULevelManager& LevelManager = ULevelManager::GetInstance();
	ULevel* CurrentLevel = LevelManager.GetCurrentLevel();

	if (!CurrentLevel)
	{
		UE_LOG("ControlPanel: Actor를 생성할 레벨이 존재하지 않습니다");
		return;
	}

	UE_LOG("ControlPanel: %s 타입의 Actor를 %d개 생성 시도합니다",
		EnumToString(SelectedPrimitiveType), NumberOfSpawn);

	// 지정된 개수만큼 액터 생성
	for (int32 i = 0; i < NumberOfSpawn; i++)
	{
		AActor* NewActor = nullptr;

		// 타입에 따라 액터 생성
		if (SelectedPrimitiveType == EPrimitiveType::Cube)
		{
			NewActor = CurrentLevel->SpawnActorToLevel(ACubeActor::StaticClass());
		}
		else if (SelectedPrimitiveType == EPrimitiveType::Sphere)
		{
			NewActor = CurrentLevel->SpawnActorToLevel(ASphereActor::StaticClass());
		}
		else if (SelectedPrimitiveType == EPrimitiveType::Triangle)
		{
			NewActor = CurrentLevel->SpawnActorToLevel(ATriangleActor::StaticClass());
		}
		else if (SelectedPrimitiveType == EPrimitiveType::Square)
		{
			NewActor = CurrentLevel->SpawnActorToLevel(ASquareActor::StaticClass());
		}
		else if (SelectedPrimitiveType == EPrimitiveType::StaticMesh)
		{
			NewActor = CurrentLevel->SpawnActorToLevel(AStaticMeshActor::StaticClass());
		}
		else if (SelectedPrimitiveType == EPrimitiveType::Billboard)
		{
			NewActor = CurrentLevel->SpawnActorToLevel(UBillboardActor::StaticClass());
		}

		if (NewActor)
		{
			// 범위 내 랜덤 위치
			float RandomX = SpawnRangeMin + (static_cast<float>(rand()) / RAND_MAX) * (SpawnRangeMax - SpawnRangeMin);
			float RandomY = SpawnRangeMin + (static_cast<float>(rand()) / RAND_MAX) * (SpawnRangeMax - SpawnRangeMin);
			float RandomZ = SpawnRangeMin + (static_cast<float>(rand()) / RAND_MAX) * (SpawnRangeMax - SpawnRangeMin);

			NewActor->SetActorLocation(FVector(RandomX, RandomY, RandomZ));

			// 임의의 스케일 (0.5 ~ 2.0 범위)
			float RandomScale = 0.5f + (static_cast<float>(rand()) / RAND_MAX) * 1.5f;
			NewActor->SetActorScale3D(FVector(RandomScale, RandomScale, RandomScale));

			// 새로 생성된 액터를 Dynamic 목록에 추가
			CurrentLevel->AddActorToDynamic(NewActor);

			UE_LOG("ControlPanel: (%.2f, %.2f, %.2f) 지점에 Actor를 배치했습니다", RandomX, RandomY, RandomZ);
		}
		else
		{
			UE_LOG("ControlPanel: Actor 배치에 실패했습니다 %d", i);
		}
	}
}
