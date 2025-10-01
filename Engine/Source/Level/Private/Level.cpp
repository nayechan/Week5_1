#include "pch.h"
#include "Level/Public/Level.h"

#include "Actor/Public/Actor.h"
#include "Component/Public/BillBoardComponent.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Manager/Level/Public/LevelManager.h"
#include "Manager/UI/Public/UIManager.h"
#include "Utility/Public/JsonSerializer.h"
#include "Actor/Public/CubeActor.h"
#include "Actor/Public/SphereActor.h"
#include "Actor/Public/TriangleActor.h"
#include "Actor/Public/SquareActor.h"
#include "Factory/Public/NewObject.h"
#include "Core/Public/Object.h"
#include "Factory/Public/FactorySystem.h"
#include "Manager/Config/Public/ConfigManager.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Editor/Public/Viewport.h"
#include "Utility/Public/ActorTypeMapper.h"

#include <json.hpp>

IMPLEMENT_CLASS(ULevel, UObject)

ULevel::ULevel() = default;

ULevel::ULevel(const FName& InName)
	: UObject(InName)
{
}

ULevel::~ULevel()
{
	// 소멸자는 Cleanup 함수를 호출하여 모든 리소스를 정리하도록 합니다.
	Cleanup();
}

void ULevel::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	// 불러오기
	if (bInIsLoading)
	{
		// NOTE: Version 사용하지 않음
		//uint32 Version = 0;
		//FJsonSerializer::ReadUint32(InOutHandle, "Version", Version);

		// NOTE: 레벨 로드 시 NextUUID를 변경하면 UUID 충돌이 발생하므로 관련 기능 구현을 보류합니다.
		uint32 NextUUID = 0;
		FJsonSerializer::ReadUint32(InOutHandle, "NextUUID", NextUUID);

		JSON PerspectiveCameraData;
		if (FJsonSerializer::ReadObject(InOutHandle, "PerspectiveCamera", PerspectiveCameraData))
		{
			UConfigManager::GetInstance().SetCameraSettingsFromJson(PerspectiveCameraData);
			URenderer::GetInstance().GetViewportClient()->ApplyAllCameraDataToViewportClients();
		}

		JSON PrimitivesJson;
		if (FJsonSerializer::ReadObject(InOutHandle, "Primitives", PrimitivesJson))
		{
			// ObjectRange()를 사용하여 Primitives 객체의 모든 키-값 쌍을 순회
			for (auto& Pair : PrimitivesJson.ObjectRange())
			{
				// Pair.first는 ID 문자열, Pair.second는 단일 프리미티브의 JSON 데이터입니다.
				const FString& IdString = Pair.first;
				JSON& PrimitiveDataJson = Pair.second;

				FString TypeString;
				FJsonSerializer::ReadString(PrimitiveDataJson, "Type", TypeString);

				UClass* NewClass = FActorTypeMapper::TypeToActor(TypeString);

				AActor* NewActor = SpawnActorToLevel(NewClass, IdString);
				if (NewActor)
				{
					NewActor->Serialize(bInIsLoading, PrimitiveDataJson);
				}
			}
		}
	}

	// 저장
	else
	{
		// NOTE: Version 사용하지 않음
		//InOutHandle["Version"] = 1;

		// NOTE: 레벨 로드 시 NextUUID를 변경하면 UUID 충돌이 발생하므로 관련 기능 구현을 보류합니다.
		InOutHandle["NextUUID"] = 0;

		// GetCameraSetting 호출 전에 뷰포트 클라이언트의 최신 데이터를 ConfigManager로 동기화합니다.
		URenderer::GetInstance().GetViewportClient()->UpdateCameraSettingsToConfig();
		InOutHandle["PerspectiveCamera"] = UConfigManager::GetInstance().GetCameraSettingsAsJson();

		JSON PrimitivesJson = json::Object();
		for (const TObjectPtr<AActor>& Actor : LevelActors)
		{
			JSON PrimitiveJson;
			PrimitiveJson["Type"] = FActorTypeMapper::ActorToType(Actor->GetClass());;
			Actor->Serialize(bInIsLoading, PrimitiveJson);

			PrimitivesJson[std::to_string(Actor->GetUUID())] = PrimitiveJson;
		}
		InOutHandle["Primitives"] = PrimitivesJson;
	}
}

void ULevel::Init()
{
	// 월드 전체 범위 지정 (씬 크기에 맞게 조정 가능)
	FAABB WorldBounds(FVector(-10000, -10000, -10000), FVector(10000, 10000, 10000));
	StaticOctree.Initialize(WorldBounds);

	// LevelActors를 Actors 배열과 동기화 (PIE 지원을 위함)
	Actors.clear();
	for (const auto& Actor : LevelActors)
	{
		if (Actor)
		{
			Actors.push_back(Actor.Get());
		}
	}

	// 레벨 안의 모든 액터 → PrimitiveComponent 순회해서 Octree에 삽입 및 LevelPrimitiveComponents에 추가
	UE_LOG("ULevel::Init: Processing %zu LevelActors and %zu Actors", LevelActors.size(), Actors.size());
	
	// LevelActors 배열 처리
	for (auto& Actor : LevelActors)
	{
		if (!Actor) continue;
		ProcessActorForInit(Actor.Get());
	}
	
	// PIE를 위해 Actors 배열도 처리 (중복 방지)
	for (AActor* Actor : Actors)
	{
		if (!Actor) continue;

		// LevelActors에 이미 있는 Actor는 스킵
		bool bAlreadyProcessed = false;
		for (const auto& LevelActor : LevelActors)
		{
			if (LevelActor.Get() == Actor)
			{
				bAlreadyProcessed = true;
				break;
			}
		}

		if (!bAlreadyProcessed)
		{
			ProcessActorForInit(Actor);
		}
	}

	// Initialize all world transforms after loading
	for (auto& Actor : LevelActors)
	{
		if (Actor && Actor->GetRootComponent())
		{
			Actor->GetRootComponent()->UpdateWorldTransform();
		}
	}

	UE_LOG("ULevel::Init: Final LevelPrimitiveComponents count: %zu", LevelPrimitiveComponents.size());
}

void ULevel::Update()
{
	// Process Delayed Task
	ProcessPendingDeletions();

	// Update all actor transforms before ticking and rendering
	// Only root components need to be updated - they will recursively update children
	static int frameCount = 0;
	int updateCount = 0;
	for (auto& Actor : LevelActors)
	{
		if (Actor && Actor->GetRootComponent())
		{
			Actor->GetRootComponent()->UpdateWorldTransform();
			updateCount++;
		}
	}

	// Log every 60 frames to check performance
	if (++frameCount % 60 == 0)
	{
		UE_LOG("Level::Update: Updated %d root transforms", updateCount);
	}

	for (auto& Actor : LevelActors)
	{
		if (Actor)
		{
			// Tick 전에 월드 변환 업데이트
			if (USceneComponent* RootComponent = Actor->GetRootComponent())
			{
				RootComponent->UpdateWorldTransform();
			}
			Actor->Tick(0.0f); // TODO: DeltaTime 매개변수 추가 필요
		}
	}
}

void ULevel::Render()
{
}

void ULevel::Cleanup()
{
	SetSelectedActor(nullptr);

	// 1. 지연 삭제 목록에 남아있는 액터들을 먼저 처리합니다.
	ProcessPendingDeletions();

	// 2. LevelActors 배열에 남아있는 모든 액터의 메모리를 해제합니다.
	for (const auto& Actor : LevelActors)
	{
		delete Actor;
	}
	LevelActors.clear();

	// 3. 모든 액터 객체가 삭제되었으므로, 포인터를 담고 있던 컸테이너들을 비웁니다.
	ActorsToDelete.clear();
	Actors.clear(); // PIE 지원을 위한 Actors 배열도 정리
	LevelPrimitiveComponents.clear();

	// 4. 선택된 액터 참조를 안전하게 해제합니다.
	SelectedActor = nullptr;

	// 5. Octree 정리
	StaticOctree.Clear();
}

AActor* ULevel::SpawnActorToLevel(UClass* InActorClass, const FName& InName)
{
	if (!InActorClass)
	{
		return nullptr;
	}

	AActor* NewActor = NewObject<AActor>(nullptr, TObjectPtr(InActorClass), InName);
	if (NewActor)
	{
		if (InName != FName::GetNone())
		{
			NewActor->SetName(InName);
		}
		// LevelActors와 Actors 모두 업데이트 (PIE 지원)
		LevelActors.push_back(TObjectPtr(NewActor));
		Actors.push_back(NewActor);
		NewActor->BeginPlay();

		for (const auto& Comp : NewActor->GetOwnedComponents())
		{
			if (auto PrimitiveComp = Cast<UPrimitiveComponent>(Comp))
			{
				LevelPrimitiveComponents.push_back(PrimitiveComp);

				// 빌보드 컴포넌트가 아니면 DynamicPrimitives에도 추가 (렌더링용)
				if (PrimitiveComp->GetPrimitiveType() != EPrimitiveType::BillBoard)
				{
					DynamicPrimitives.push_back(TObjectPtr(PrimitiveComp));
				}
			}
		}

		return NewActor;
	}

	return nullptr;
}

void ULevel::AddLevelPrimitiveComponent(AActor* Actor)
{
	if (!Actor) return;

	for (auto& Component : Actor->GetOwnedComponents())
	{
		if (Component->GetComponentType() >= EComponentType::Primitive)
		{
			TObjectPtr<UPrimitiveComponent> PrimitiveComponent = Cast<UPrimitiveComponent>(Component);

			if (!PrimitiveComponent)
			{
				continue;
			}

			/* 3가지 경우 존재.
			1: primitive show flag가 꺼져 있으면, 도형, 빌보드 모두 렌더링 안함.
			2: primitive show flag가 켜져 있고, billboard show flag가 켜져 있으면, 도형, 빌보드 모두 렌더링
			3: primitive show flag가 켜져 있고, billboard show flag가 꺼져 있으면, 도형은 렌더링 하지만, 빌보드는 렌더링 안함. */
			// 빌보드는 무조건 피킹이 된 actor의 빌보드여야 렌더링 가능
			if (PrimitiveComponent->IsVisible() && (ShowFlags & EEngineShowFlags::SF_Primitives))
			{
				if (PrimitiveComponent->GetPrimitiveType() != EPrimitiveType::BillBoard)
				{
					LevelPrimitiveComponents.push_back(PrimitiveComponent);
				}
				else if (PrimitiveComponent->GetPrimitiveType() == EPrimitiveType::BillBoard && (ShowFlags & EEngineShowFlags::SF_BillboardText) && (ULevelManager::GetInstance().GetCurrentLevel()->GetSelectedActor() == Actor))
				{
					//TObjectPtr<UBillBoardComponent> BillBoard = Cast<UBillBoardComponent>(PrimitiveComponent);
					//BillBoard->UpdateRotationMatrix();
					LevelPrimitiveComponents.push_back(PrimitiveComponent);
				}
			}
		}
	}
}


void ULevel::SetSelectedActor(AActor* InActor)
{
	if (InActor != SelectedActor)
	{
		UUIManager::GetInstance().OnSelectedActorChanged(InActor);
	}
	SelectedActor = InActor;
}

// Level에서 Actor 제거하는 함수
bool ULevel::DestroyActor(AActor* InActor)
{
	if (!InActor)
	{
		return false;
	}

	// LevelActors 리스트에서 제거
	for (auto Iterator = LevelActors.begin(); Iterator != LevelActors.end(); ++Iterator)
	{
		if (*Iterator == InActor)
		{
			LevelActors.erase(Iterator);
			break;
		}
	}

	// Remove Actor Selection
	if (SelectedActor == InActor)
	{
		SelectedActor = nullptr;
	}

	// Remove Primitive Component
	for (const auto& Component : InActor->GetOwnedComponents())
	{
		UActorComponent* ActorComponent = Component.Get();
		const auto& Iterator = std::find(LevelPrimitiveComponents.begin(), LevelPrimitiveComponents.end(), ActorComponent);
		if(Iterator != LevelPrimitiveComponents.end())
			LevelPrimitiveComponents.erase(Iterator);
		if (ActorComponent->GetClass() == UBillBoardComponent::StaticClass())
		{
			continue;
		}
		else if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(ActorComponent))
		{
			// 1) Attempt to remove from the static octree
			StaticOctree.Remove(PrimComp);

			// 2) Remove from dynamic primitives array
			for (auto It = DynamicPrimitives.begin(); It != DynamicPrimitives.end(); ++It)
			{
				if (It->Get() == PrimComp)
				{
					DynamicPrimitives.erase(It);
					break;
				}
			}
		}
	}
	// Remove
	delete InActor;

	UE_LOG("Level: Actor Destroyed Successfully");
	return true;
}

// 지연 삭제를 위한 마킹
void ULevel::MarkActorForDeletion(AActor* InActor)
{
	if (!InActor)
	{
		UE_LOG("Level: MarkActorForDeletion: InActor Is Null");
		return;
	}

	// 이미 삭제 대기 중인지 확인
	for (AActor* PendingActor : ActorsToDelete)
	{
		if (PendingActor == InActor)
		{
			UE_LOG("Level: Actor Already Marked For Deletion");
			return;
		}
	}

	// 삭제 대기 리스트에 추가
	ActorsToDelete.push_back(InActor);
	UE_LOG("Level: 다음 Tick에 Actor를 제거하기 위한 마킹 처리: %s", InActor->GetName().ToString().data());

	// 선택 해제는 바로 처리
	if (SelectedActor == InActor)
	{
		SelectedActor = nullptr;
	}
}

void ULevel::ProcessPendingDeletions()
{
	if (ActorsToDelete.empty())
	{
		return;
	}

	UE_LOG("Level: %zu개의 객체 지연 삭제 프로세스 처리 시작", ActorsToDelete.size());

	// 원본 배열을 복사하여 사용 (DestroyActor가 원본을 수정할 가능성에 대비)
	TArray<AActor*> ActorsToProcess = ActorsToDelete;
	ActorsToDelete.clear();

	for (AActor* ActorToDelete : ActorsToProcess)
	{
		if (ActorToDelete)
		{
			DestroyActor(ActorToDelete);
		}
	}

	UE_LOG("Level: 모든 지연 삭제 프로세스 완료");
}

void ULevel::ProcessActorForInit(AActor* Actor)
{
	if (!Actor) return;
	
	for (auto& Component : Actor->GetOwnedComponents())
	{
		if (Component->GetComponentType() >= EComponentType::Primitive)
		{
			UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Component);
			if (!PrimitiveComponent) continue;
			
			// LevelPrimitiveComponents에 추가 (렌더링을 위해 필수!)
			LevelPrimitiveComponents.push_back(TObjectPtr<UPrimitiveComponent>(PrimitiveComponent));

			// 빌보드 컴포넌트는 Octree에 삽입하지 않음
			if (PrimitiveComponent->GetPrimitiveType() == EPrimitiveType::BillBoard)
				continue;

			FVector Min, Max;
			PrimitiveComponent->GetWorldAABB(Min, Max);
			FAABB WorldBounds(Min, Max);

			StaticOctree.Insert(PrimitiveComponent, WorldBounds);
		}
	}
}

void ULevel::MoveToDynamic(UPrimitiveComponent* InPrim)
{
	if (!InPrim) return;

	// 이미 동적 배열에 있는지 확인 (중복 추가 방지)
	for (const auto& DynPrim : DynamicPrimitives)
	{
		if (DynPrim == InPrim) return;
	}
	
	StaticOctree.Remove(InPrim);
	DynamicPrimitives.push_back(TObjectPtr(InPrim));
}

void ULevel::DuplicateSubObjects()
{
	Super::DuplicateSubObjects();
	
	UE_LOG("ULevel::DuplicateSubObjects: Starting with %zu LevelActors and %zu Actors", LevelActors.size(), Actors.size());
	
	// LevelActors 배열이 비어있지만 Actors에 데이터가 있는 경우 (PIE)
	if (LevelActors.empty() && !Actors.empty())
	{
		UE_LOG("ULevel::DuplicateSubObjects: PIE mode detected - processing Actors array");
		// Actors 배열의 모든 Actor 복제
		for (auto& Actor : Actors)
		{
			if (Actor)
			{
				AActor* DuplicatedActor = static_cast<AActor*>(Actor->Duplicate());
				if (DuplicatedActor)
				{
					Actor = DuplicatedActor;
				}
			}
		}
	}
	else
	{
		UE_LOG("ULevel::DuplicateSubObjects: Editor mode detected - processing LevelActors array");
		// LevelActors 배열의 모든 Actor 복제
		for (auto& Actor : LevelActors)
		{
			if (Actor)
			{
				Actor = static_cast<AActor*>(Actor->Duplicate());
			}
		}
		
		// Actors 배열을 LevelActors에서 동기화
		Actors.clear();
		for (const auto& Actor : LevelActors)
		{
			if (Actor)
			{
				Actors.push_back(Actor.Get());
			}
		}
	}
	
	// LevelPrimitiveComponents 업데이트
	LevelPrimitiveComponents.clear();
	
	// Actors 배열을 기준으로 LevelPrimitiveComponents 업데이트 (PIE 지원)
	for (AActor* Actor : Actors)
	{
		if (Actor)
		{
			for (const auto& Component : Actor->GetOwnedComponents())
			{
				if (auto PrimitiveComp = Cast<UPrimitiveComponent>(Component))
				{
					LevelPrimitiveComponents.push_back(TObjectPtr<UPrimitiveComponent>(PrimitiveComp));
				}
			}
		}
	}
	
	UE_LOG("ULevel::DuplicateSubObjects: Completed with %zu LevelActors, %zu Actors, and %zu LevelPrimitiveComponents", 
	       LevelActors.size(), Actors.size(), LevelPrimitiveComponents.size());
}

UObject* ULevel::Duplicate()
{
	UE_LOG("ULevel::Duplicate: Starting duplication of %s (UUID: %u)", GetName().ToString().data(), GetUUID());
	
	// NewObject를 사용하여 새로운 Level 생성
	ULevel* NewLevel = NewObject<ULevel>(nullptr, GetClass());
	if (!NewLevel)
	{
		UE_LOG("ULevel::Duplicate: Failed to create new level!");
		return nullptr;
	}
	
	UE_LOG("ULevel::Duplicate: New level created with UUID: %u", NewLevel->GetUUID());
	
	// ULevel 고유 속성들 복사
	NewLevel->ShowFlags = ShowFlags;
	
	// 서브 오브젝트 복제
	NewLevel->DuplicateSubObjects();
	
	UE_LOG("ULevel::Duplicate: Duplication completed for %s (UUID: %u) -> %s (UUID: %u)", 
	       GetName().ToString().data(), GetUUID(), 
	       NewLevel->GetName().ToString().data(), NewLevel->GetUUID());
	
	return NewLevel;
}


void ULevel::RegisterPrimitiveComponent(UPrimitiveComponent* NewPrimitive)
{
	if (!NewPrimitive) return;

	// 빌보드 컴포넌트는 렌더링 목록에 직접 추가하지 않습니다.
	if (NewPrimitive->GetPrimitiveType() == EPrimitiveType::BillBoard)
		return;

	// 런타임에 생성된 컴포넌트는 DynamicPrimitives 목록에 추가하며 렌더링되도록 합니다.
	DynamicPrimitives.push_back(TObjectPtr(NewPrimitive));
}
