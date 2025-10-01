#include "pch.h"
#include "Level/Public/Level.h"

#include "Actor/Public/Actor.h"
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

TArray<AActor*> ULevel::GetActorsPtrs() const
{
	TArray<AActor*> ActorPtrs;
	for (const auto& Actor : LevelActors)
	{
		if (Actor)
		{
			ActorPtrs.push_back(Actor.Get());
		}
	}
	return ActorPtrs;
}

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

	// PIE 지원을 위한 별도 배열 동기화 제거 (통합된 배열 사용)
	UE_LOG("ULevel::Init: Processing %zu LevelActors", LevelActors.size());

	// IMPORTANT: Initialize all world transforms FIRST before processing primitives
	// 월드 변환을 먼저 업데이트해야 GetWorldAABB가 올바른 값을 반환함
	for (auto& Actor : LevelActors)
	{
		if (Actor && Actor->GetRootComponent())
		{
			Actor->GetRootComponent()->UpdateWorldTransform();
		}
	}

	// 레벨 안의 모든 액터를 Octree에 삽입 및 LevelPrimitiveComponents에 추가
	for (auto& Actor : LevelActors)
	{
		if (!Actor) continue;
		ProcessActorForInit(Actor.Get());
	}

	UE_LOG("ULevel::Init: Final LevelPrimitiveComponents count: %zu", LevelPrimitiveComponents.size());
}

void ULevel::Update()
{
	// Process Delayed Task
	static int updateCallCount = 0;
	updateCallCount++;
	if (!ActorsToDelete.empty())
	{
		UE_LOG("Level::Update (frame %d): ActorsToDelete에 %zu개 대기 중, ProcessPendingDeletions 호출 예정",
		       updateCallCount, ActorsToDelete.size());
	}
	ProcessPendingDeletions();

	// 최적화: Transform 업데이트를 루트 컴포넌트만 수행 (자식들은 재귀적으로 업데이트)
	static int frameCount = 0;
	int updateCount = 0;
	int tickCount = 0;
	
	for (auto& Actor : LevelActors)
	{
		if (Actor && Actor->GetRootComponent())
		{
			// 루트 컴포넌트만 업데이트 (자식들은 재귀적으로 처리됨)
			Actor->GetRootComponent()->UpdateWorldTransform();
			updateCount++;
		}
		
		// Tick 처리
		if (Actor && Actor->IsActorTickEnabled())
		{
			Actor->Tick(0.0f); // TODO: DeltaTime 매개변수 추가 필요
			tickCount++;
		}
	}

	// Log every 60 frames to check performance
	if (++frameCount % 60 == 0)
	{
		UE_LOG("Level::Update: Updated %d transforms, Ticked %d actors", updateCount, tickCount);
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

	// 2. CRITICAL: Octree를 먼저 정리 (Actor 삭제 전에 포인터 참조 제거)
	StaticOctree.Clear();

	// 3. LevelActors 배열에 남아있는 모든 액터의 메모리를 해제합니다.
	for (const auto& Actor : LevelActors)
	{
		delete Actor;
	}
	LevelActors.clear();

	// 4. 모든 액터 객체가 삭제되었으므로, 포인터를 담고 있던 컴테이너들을 비웁니다.
	ActorsToDelete.clear();
	LevelPrimitiveComponents.clear();
	DynamicPrimitives.clear();

	// 5. 선택된 액터 참조를 안전하게 해제합니다.
	SelectedActor = nullptr;
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
		// 통합된 Actor 배열에 추가
		LevelActors.push_back(TObjectPtr(NewActor));
		NewActor->BeginPlay();

		// Use GetAllComponents() to include nested children
		TArray<UActorComponent*> AllComponents = NewActor->GetAllComponents();
		for (UActorComponent* Comp : AllComponents)
		{
			if (UPrimitiveComponent* PrimitiveComp = Cast<UPrimitiveComponent>(Comp))
			{
				LevelPrimitiveComponents.push_back(TObjectPtr(PrimitiveComp));

				// 빌보드 컴포넌트가 아니면 DynamicPrimitives에도 추가 (렌더링용)
				if (PrimitiveComp->GetPrimitiveType() != EPrimitiveType::Billboard)
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

	UE_LOG("Level::AddLevelPrimitiveComponent for Actor: %s", Actor->GetName().ToString().c_str());

	// Use GetAllComponents() instead of GetOwnedComponents() to include nested children
	TArray<UActorComponent*> AllComponents = Actor->GetAllComponents();

	UE_LOG("  -> Got %d components from GetAllComponents()", AllComponents.size());

	for (UActorComponent* Component : AllComponents)
	{
		if (!Component || Component->GetComponentType() < EComponentType::Primitive)
		{
			if (Component)
			{
				UE_LOG("  -> Skipping non-primitive component: %s (Type: %d)",
					   Component->GetName().ToString().c_str(),
					   static_cast<int>(Component->GetComponentType()));
			}
			continue;
		}

		UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Component);
		if (!PrimitiveComponent)
		{
			UE_LOG("  -> Cast to PrimitiveComponent failed for: %s", Component->GetName().ToString().c_str());
			continue;
		}

		/* 3가지 경우 존재.
		1: primitive show flag가 꺼져 있으면, 도형, 빌보드 모두 렌더링 안함.
		2: primitive show flag가 켜져 있고, billboard show flag가 켜져 있으면, 도형, 빌보드 모두 렌더링
		3: primitive show flag가 켜져 있고, billboard show flag가 꺼져 있으면, 도형은 렌더링 하지만, 빌보드는 렌더링 안함. */
		// 빌보드는 무조건 피킹이 된 actor의 빌보드여야 렌더링 가능
		if (PrimitiveComponent->IsVisible() && (ShowFlags & EEngineShowFlags::SF_Primitives))
		{
				LevelPrimitiveComponents.push_back(TObjectPtr(PrimitiveComponent));			
		}
	}
}
void ULevel::AddActorToDynamic(AActor* Actor)
{
	if (!Actor) return;

	for (auto& Component : Actor->GetOwnedComponents())
	{
		if (Component->GetComponentType() >= EComponentType::Primitive)
		{
			UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Component);
			if (!PrimitiveComponent) continue;

			// 빌보드 컴포넌트는 추가하지 않음
			/*if (PrimitiveComponent->GetPrimitiveType() == EPrimitiveType::Billboard)
				continue;*/

				// 런타임에 생성된 오브젝트는 DynamicPrimitives에 추가
				// 나중에 필요시 MoveToDynamic/MoveToStatic으로 이동 가능
			DynamicPrimitives.push_back(TObjectPtr(PrimitiveComponent));
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

	// CRITICAL FIX: Remove all references to components BEFORE deleting the actor
	// to avoid dangling pointer access and heap corruption

	// Step 1: Get all components while actor is still valid
	TArray<UActorComponent*> AllComponents = InActor->GetAllComponents();
	UE_LOG("Level: DestroyActor - Removing %d components from %s", AllComponents.size(), InActor->GetName().ToString().data());

	// Step 2: Remove components from all collections BEFORE deleting actor memory
	for (UActorComponent* ActorComponent : AllComponents)
	{
		if (!ActorComponent) continue;

		// If it's a PrimitiveComponent, remove from spatial structures first
		if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(ActorComponent))
		{
			// 1) Remove from the static octree
			StaticOctree.Remove(PrimComp);

			// 2) Remove from LevelPrimitiveComponents using remove-erase idiom
			// This is safe because we compare pointers before deletion
			LevelPrimitiveComponents.erase(
				std::remove_if(LevelPrimitiveComponents.begin(), LevelPrimitiveComponents.end(),
					[PrimComp](const TObjectPtr<UPrimitiveComponent>& Ptr) {
						return Ptr.Get() == PrimComp;
					}),
				LevelPrimitiveComponents.end()
			);

			// 3) Remove from dynamic primitives array using remove-erase idiom
			DynamicPrimitives.erase(
				std::remove_if(DynamicPrimitives.begin(), DynamicPrimitives.end(),
					[PrimComp](const TObjectPtr<UPrimitiveComponent>& Ptr) {
						return Ptr.Get() == PrimComp;
					}),
				DynamicPrimitives.end()
			);

			UE_LOG("  - Removed component %s from all collections", PrimComp->GetName().ToString().data());
		}
	}

	UE_LOG("Level: After component removal, LevelPrimitiveComponents size: %zu", LevelPrimitiveComponents.size());

	// Step 3: NOW it's safe to delete the actor (all references have been cleared)
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
	if (InActor->IsPendingKill())
	{
		UE_LOG("Level: Actor Already Marked For Deletion (via PendingKill flag)");
		return;
	}

	for (AActor* PendingActor : ActorsToDelete)
	{
		if (PendingActor == InActor)
		{
			UE_LOG("Level: Actor Already Marked For Deletion");
			return;
		}
	}

	// CRITICAL: Mark actor and all its components as pending kill IMMEDIATELY
	// This prevents any further use of this object before actual deletion
	InActor->MarkPendingKill();
	TArray<UActorComponent*> AllComponents = InActor->GetAllComponents();
	for (UActorComponent* Component : AllComponents)
	{
		if (Component)
		{
			Component->MarkPendingKill();
		}
	}

	// 삭제 대기 리스트에 추가
	ActorsToDelete.push_back(InActor);
	UE_LOG("Level: 다음 Tick에 Actor를 제거하기 위한 마킹 처리: %s (PendingKill set)", InActor->GetName().ToString().data());

	// 선택 해제는 바로 처리
	if (SelectedActor == InActor)
	{
		SelectedActor = nullptr;
	}

	// UIManager의 SelectedObject도 해제
	// 삭제될 액터와 관련된 모든 선택을 무조건 해제 (안전을 위해)
	UUIManager& UIManager = UUIManager::GetInstance();
	UIManager.SetSelectedObject(nullptr);
}

void ULevel::ProcessPendingDeletions()
{
	if (ActorsToDelete.empty())
	{
		return;
	}

	UE_LOG("===============================================");
	UE_LOG("Level::ProcessPendingDeletions: %zu개의 객체 지연 삭제 프로세스 처리 시작", ActorsToDelete.size());

	// 원본 배열을 복사하여 사용 (DestroyActor가 원본을 수정할 가능성에 대비)
	TArray<AActor*> ActorsToProcess = ActorsToDelete;
	ActorsToDelete.clear();

	UE_LOG("Level::ProcessPendingDeletions: ActorsToDelete 배열 클리어 완료");

	for (AActor* ActorToDelete : ActorsToProcess)
	{
		if (ActorToDelete)
		{
			UE_LOG("Level::ProcessPendingDeletions: DestroyActor 호출 - %s (ptr: %p)",
			       ActorToDelete->GetName().ToString().data(), ActorToDelete);
			DestroyActor(ActorToDelete);
		}
		else
		{
			UE_LOG_WARNING("Level::ProcessPendingDeletions: nullptr 액터 발견, 스킵");
		}
	}

	UE_LOG("Level::ProcessPendingDeletions: 모든 지연 삭제 프로세스 완료");
	UE_LOG("===============================================");
}

void ULevel::ProcessActorForInit(AActor* Actor)
{
	if (!Actor) return;

	UE_LOG("ProcessActorForInit: Processing actor %s", Actor->GetName().ToString().c_str());

	// Use GetAllComponents() to include child components
	TArray<UActorComponent*> AllComponents = Actor->GetAllComponents();
	UE_LOG("  -> Got %d total components (including children)", AllComponents.size());

	// Force update all scene components' world transforms first
	for (UActorComponent* Component : AllComponents)
	{
		if (USceneComponent* SceneComp = Cast<USceneComponent>(Component))
		{
			SceneComp->UpdateWorldTransform();
		}
	}

	// Process all components for rendering system registration
	for (UActorComponent* Component : AllComponents)
	{
		if (Component->GetComponentType() >= EComponentType::Primitive)
		{
			UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Component);
			if (!PrimitiveComponent) continue;
			
			UE_LOG("  -> Processing primitive component: %s (Owner: %s)",
				   PrimitiveComponent->GetName().ToString().c_str(),
				   PrimitiveComponent->GetOwner() ? PrimitiveComponent->GetOwner()->GetName().ToString().c_str() : "None");
			
			// LevelPrimitiveComponents에 추가 (렌더링을 위해 필수!)
			LevelPrimitiveComponents.push_back(TObjectPtr<UPrimitiveComponent>(PrimitiveComponent));

			// 빌보드 컴포넌트는 Octree에 삽입하지 않음
			if (PrimitiveComponent->GetPrimitiveType() == EPrimitiveType::Billboard)
			{
				UE_LOG("    -> Skipping Billboard component for Octree");
				continue;
			}

			// CRITICAL: Initialize Min/Max to safe defaults before GetWorldAABB
			FVector Min(0.0f, 0.0f, 0.0f), Max(0.0f, 0.0f, 0.0f);
			PrimitiveComponent->GetWorldAABB(Min, Max);

			// Skip if bounding box is invalid (GetWorldAABB returns without setting values)
			if (Min.X == 0.0f && Min.Y == 0.0f && Min.Z == 0.0f &&
			    Max.X == 0.0f && Max.Y == 0.0f && Max.Z == 0.0f)
			{
				UE_LOG("    -> Warning: Invalid bounding box, skipping Octree insertion");
				continue;
			}

			FAABB WorldBounds(Min, Max);
			StaticOctree.Insert(PrimitiveComponent, WorldBounds);
			UE_LOG("    -> Added to Octree");
		}
	}

	UE_LOG("ProcessActorForInit: Completed for %s", Actor->GetName().ToString().c_str());
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
	
	UE_LOG("ULevel::DuplicateSubObjects: Starting with %zu LevelActors", LevelActors.size());
	
	// 통합된 LevelActors 배열의 모든 Actor 복제
	for (auto& Actor : LevelActors)
	{
		if (Actor)
		{
			Actor = static_cast<AActor*>(Actor->Duplicate());
		}
	}
	
	// LevelPrimitiveComponents 업데이트
	LevelPrimitiveComponents.clear();
	
	// LevelActors 배열을 기준으로 LevelPrimitiveComponents 업데이트 (child component 포함)
	for (const auto& Actor : LevelActors)
	{
		if (Actor)
		{
			// Use GetAllComponents() to include child components
			TArray<UActorComponent*> AllComponents = Actor->GetAllComponents();
			UE_LOG("  DuplicateSubObjects: Actor %s has %d total components", 
			       Actor->GetName().ToString().c_str(), AllComponents.size());
			       
			for (UActorComponent* Component : AllComponents)
			{
				if (auto PrimitiveComp = Cast<UPrimitiveComponent>(Component))
				{
					LevelPrimitiveComponents.push_back(TObjectPtr<UPrimitiveComponent>(PrimitiveComp));
					UE_LOG("    -> Added primitive: %s (Owner: %s)", 
					       PrimitiveComp->GetName().ToString().c_str(),
					       PrimitiveComp->GetOwner() ? PrimitiveComp->GetOwner()->GetName().ToString().c_str() : "None");
				}
			}
		}
	}
	
	// DynamicPrimitives도 복제된 Actor들로부터 업데이트
	DynamicPrimitives.clear();
	UE_LOG("  DuplicateSubObjects: Rebuilding DynamicPrimitives from duplicated actors...");
	
	// 복제된 Actor들에서 DynamicPrimitives 재구성
	for (const auto& Actor : LevelActors)
	{
		if (Actor)
		{
			// 모든 component (자식 포함)를 검사하여 DynamicPrimitives에 추가
			TArray<UActorComponent*> AllComponents = Actor->GetAllComponents();
			UE_LOG("  PIE DuplicateSubObjects: Actor %s has %d components after duplication:", 
			       Actor->GetName().ToString().c_str(), AllComponents.size());
			       
			// Debug: 각 컴포넌트의 타입과 상태 확인
			for (int32 i = 0; i < AllComponents.size(); i++)
			{
				UActorComponent* Component = AllComponents[i];
				if (Component)
				{
					bool bIsOwnedComponent = false;
					for (const auto& OwnedComp : Actor->GetOwnedComponents())
					{
						if (OwnedComp.Get() == Component)
						{
							bIsOwnedComponent = true;
							break;
						}
					}
					
					UE_LOG("    [%d] %s: %s (Type: %d, IsOwned: %s, Owner: %s)",
					       i, bIsOwnedComponent ? "OWNED" : "CHILD",
					       Component->GetName().ToString().c_str(),
					       static_cast<int>(Component->GetComponentType()),
					       bIsOwnedComponent ? "Yes" : "No",
					       Component->GetOwner() ? Component->GetOwner()->GetName().ToString().c_str() : "None");
				}
			}
			       
			for (UActorComponent* Component : AllComponents)
			{
				if (UPrimitiveComponent* PrimitiveComp = Cast<UPrimitiveComponent>(Component))
				{
					// Billboard가 아닌 모든 primitive component를 DynamicPrimitives에 추가
					if (PrimitiveComp->GetPrimitiveType() != EPrimitiveType::Billboard)
					{
						DynamicPrimitives.push_back(TObjectPtr<UPrimitiveComponent>(PrimitiveComp));
						UE_LOG("    -> Added to DynamicPrimitives: %s (Owner: %s, Type: %d, Visible: %s)", 
						       PrimitiveComp->GetName().ToString().c_str(),
						       PrimitiveComp->GetOwner() ? PrimitiveComp->GetOwner()->GetName().ToString().c_str() : "None",
						       static_cast<int>(PrimitiveComp->GetPrimitiveType()),
						       PrimitiveComp->IsVisible() ? "Yes" : "No");
					}
					else
					{
						UE_LOG("    -> Skipping Billboard component: %s", PrimitiveComp->GetName().ToString().c_str());
					}
				}
			}
		}
	}
	
	UE_LOG("ULevel::DuplicateSubObjects: Completed with %zu LevelActors, %zu LevelPrimitiveComponents, and %zu DynamicPrimitives", 
	       LevelActors.size(), LevelPrimitiveComponents.size(), DynamicPrimitives.size());
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

	UE_LOG("Level::RegisterPrimitiveComponent: %s (Owner: %s)",
		   NewPrimitive->GetName().ToString().c_str(),
		   NewPrimitive->GetOwner() ? NewPrimitive->GetOwner()->GetName().ToString().c_str() : "None");

	// 빌보드 컴포넌트는 렌더링 목록에 직접 추가하지 않습니다.
	if (NewPrimitive->GetPrimitiveType() == EPrimitiveType::Billboard)
	{
		UE_LOG("  -> Skipping Billboard component");
		return;
	}

	// 런타임에 생성된 컴포넌트는 DynamicPrimitives 목록에 추가하며 렌더링되도록 합니다.
	DynamicPrimitives.push_back(TObjectPtr(NewPrimitive));

	// CRITICAL: Also add to LevelPrimitiveComponents for BVH/picking
	LevelPrimitiveComponents.push_back(TObjectPtr(NewPrimitive));

	UE_LOG("  -> Added to DynamicPrimitives and LevelPrimitiveComponents (Total: %d)",
		   LevelPrimitiveComponents.size());
}
