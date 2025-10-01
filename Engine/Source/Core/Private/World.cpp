#include "pch.h"
#include "Core/Public/World.h"
#include "Level/Public/Level.h"
#include "Actor/Public/Actor.h"
#include "Component/Public/ActorComponent.h"
#include "Factory/Public/NewObject.h"

IMPLEMENT_CLASS(UWorld, UObject)

UWorld::UWorld()
    : Super()
    , Level(nullptr)
    , WorldType(EWorldType::Editor)
{
}

UWorld::UWorld(const FName& InName)
    : Super(InName)
    , Level(nullptr)
    , WorldType(EWorldType::Editor)
{
}

UWorld::~UWorld()
{
}

void UWorld::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);
    
    if (bInIsLoading)
    {
        // 로딩 시 구현
    }
    else
    {
        // 저장 시 구현
    }
}

void UWorld::Tick(float DeltaTime)
{
    if (!Level)
        return;

    // IMPORTANT: Process Level's pending deletions and system updates FIRST
    Level->Update();

    // Level의 모든 액터들을 업데이트
    TArray<AActor*> Actors = Level->GetActorsPtrs();
    for (AActor* Actor : Actors)
    {
        if (Actor && Actor->IsActorTickEnabled())
        {
            Actor->Tick(DeltaTime);
        }
    }
}

UWorld* UWorld::DuplicateWorldForPIE(UWorld* EditorWorld)
{
	if (!EditorWorld)
	{
		UE_LOG("DuplicateWorldForPIE: EditorWorld is null!");
		return nullptr;
	}

	UE_LOG("DuplicateWorldForPIE: Starting duplication of %s", EditorWorld->GetName().ToString().data());

	// 새로운 PIE 월드 생성
	UWorld* PIEWorld = NewObject<UWorld>(nullptr, UWorld::StaticClass(), FName("PIEWorld"));
	if (!PIEWorld)
	{
		UE_LOG("DuplicateWorldForPIE: Failed to create PIE World!");
		return nullptr;
	}

	UE_LOG("DuplicateWorldForPIE: PIE World created successfully");
	PIEWorld->SetWorldType(EWorldType::PIE);

	// 에디터 월드의 레벨을 복제
	if (EditorWorld->GetLevel())
	{
		ULevel* EditorLevel = EditorWorld->GetLevel();
		TArray<AActor*> EditorActors = EditorLevel->GetActorsPtrs();
		UE_LOG("DuplicateWorldForPIE: Editor Level found with %zu actors", EditorActors.size());
		
		// 새로운 PIE 레벨 생성
		ULevel* PIELevel = NewObject<ULevel>(TObjectPtr<UObject>(PIEWorld), ULevel::StaticClass(), FName("PIELevel"));
		
		if (PIELevel)
		{
			UE_LOG("DuplicateWorldForPIE: PIE Level created, starting actor duplication...");
			// PIE Level의 LevelActors 배열에 직접 액터들 추가
			size_t duplicatedCount = 0;
			TArray<TObjectPtr<AActor>>& PIEActorsArray = PIELevel->GetActors();  // 통합된 배열 참조
			
			for (AActor* EditorActor : EditorActors)
			{
				if (EditorActor)
				{
					// 복제 전 원본 액터의 컴포넌트 상태 확인
					TArray<UActorComponent*> OriginalComponents = EditorActor->GetAllComponents();
					UE_LOG("DuplicateWorldForPIE: Duplicating actor: %s (has %d total components)", 
					       EditorActor->GetName().ToString().data(), OriginalComponents.size());
					       
					// 자식 컴포넌트들 리스트 출력
					for (UActorComponent* OrigComp : OriginalComponents)
					{
						if (OrigComp)
						{
							UE_LOG("  - Original Component: %s (Type: %d, Owner: %s)", 
							       OrigComp->GetName().ToString().c_str(),
							       static_cast<int>(OrigComp->GetComponentType()),
							       OrigComp->GetOwner() ? OrigComp->GetOwner()->GetName().ToString().c_str() : "None");
						}
					}
					
					// 액터 복제 (얘은 복사 + 서브오브젝트 깊은 복사)
					AActor* PIEActor = Cast<AActor>(EditorActor->Duplicate());
					if (PIEActor)
					{
						// 복제 후 PIE 액터의 컴포넌트 상태 확인
						TArray<UActorComponent*> PIEComponents = PIEActor->GetAllComponents();
						UE_LOG("DuplicateWorldForPIE: Actor duplicated successfully: %s (has %d total components)", 
						       PIEActor->GetName().ToString().data(), PIEComponents.size());
						       
						// 복제된 컴포넌트들 리스트 출력
						for (UActorComponent* PIEComp : PIEComponents)
						{
							if (PIEComp)
							{
								UE_LOG("  - PIE Component: %s (Type: %d, Owner: %s)", 
								       PIEComp->GetName().ToString().c_str(),
								       static_cast<int>(PIEComp->GetComponentType()),
								       PIEComp->GetOwner() ? PIEComp->GetOwner()->GetName().ToString().c_str() : "None");
							}
						}
						
						// 통합된 PIE 배열에 추가
						PIEActorsArray.push_back(TObjectPtr<AActor>(PIEActor));
						duplicatedCount++;
						UE_LOG("DuplicateWorldForPIE: Added to PIE Level Actors (%zu)", PIEActorsArray.size());
					}
					else
					{
						UE_LOG("DuplicateWorldForPIE: Failed to duplicate actor: %s", EditorActor->GetName().ToString().data());
					}
				}
			}
			
			UE_LOG("DuplicateWorldForPIE: Duplicated %zu actors successfully", duplicatedCount);
			PIEWorld->SetLevel(PIELevel);
			UE_LOG("DuplicateWorldForPIE: PIE Level set to PIE World");
			
			// PIE Level 초기화 (렌더링 시스템에 등록)
			UE_LOG("DuplicateWorldForPIE: Initializing PIE Level...");
			PIELevel->Init();
			UE_LOG("DuplicateWorldForPIE: PIE Level initialized successfully");
		}
		else
		{
			UE_LOG("DuplicateWorldForPIE: Failed to create PIE Level!");
		}
	}
	else
	{
		UE_LOG("DuplicateWorldForPIE: Editor World has no level!");
	}

	UE_LOG("DuplicateWorldForPIE: Returning PIE World: %s", PIEWorld ? PIEWorld->GetName().ToString().data() : "null");
	return PIEWorld;
}

void UWorld::InitializeActorsForPlay()
{
    if (!Level)
        return;

    // 모든 액터의 BeginPlay 호출
    TArray<AActor*> Actors = Level->GetActorsPtrs();
    for (AActor* Actor : Actors)
    {
        if (Actor)
        {
            Actor->BeginPlay();
        }
    }
}

void UWorld::CleanupWorld()
{
    if (!Level)
        return;

    // 모든 액터의 EndPlay 호출
    TArray<AActor*> Actors = Level->GetActorsPtrs();
    for (AActor* Actor : Actors)
    {
        if (Actor)
        {
            Actor->EndPlay(EEndPlayReason::RemovedFromWorld);
        }
    }

    // 레벨 정리
    Level->Cleanup();
}

void UWorld::DuplicateSubObjects()
{
	Super::DuplicateSubObjects();
	if (Level)
	{
		// Duplicate() 반환형이 UObject* 라면 캐스팅 명시
		Level = static_cast<ULevel*>(Level->Duplicate());
	}
}

UObject* UWorld::Duplicate()
{
	// 얕은 복사 + 서브오브젝트 깊은 복사
	UWorld* NewWorld = new UWorld(*this);
	NewWorld->DuplicateSubObjects();
	return NewWorld;
}
