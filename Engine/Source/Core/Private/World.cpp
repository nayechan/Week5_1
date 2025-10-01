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

    // Level의 모든 액터들을 업데이트
    for (AActor* Actor : Level->GetActors())
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
		UE_LOG("DuplicateWorldForPIE: Editor Level found with %zu actors", EditorLevel->GetActors().size());
		
		// 새로운 PIE 레벨 생성
		ULevel* PIELevel = NewObject<ULevel>(TObjectPtr<UObject>(PIEWorld), ULevel::StaticClass(), FName("PIELevel"));
		
		if (PIELevel)
		{
			UE_LOG("DuplicateWorldForPIE: PIE Level created, starting actor duplication...");
			// PIE Level의 내부 배열에 직접 액터들 추가
			size_t duplicatedCount = 0;
			TArray<AActor*>& PIEActorsArray = PIELevel->GetActors();  // 참조로 가져오기
			
			for (AActor* EditorActor : EditorLevel->GetActors())
			{
				if (EditorActor)
				{
					UE_LOG("DuplicateWorldForPIE: Duplicating actor: %s", EditorActor->GetName().ToString().data());
					// 액터 복제 (얕은 복사 + 서브오브젝트 깊은 복사)
					AActor* PIEActor = Cast<AActor>(EditorActor->Duplicate());
					if (PIEActor)
					{
						UE_LOG("DuplicateWorldForPIE: Actor duplicated successfully: %s", PIEActor->GetName().ToString().data());
						// PIE 레벨의 Actors 배열에 직접 추가
						PIEActorsArray.push_back(PIEActor);
						// CRITICAL FIX: LevelActors 배열에도 추가 (Init()에서 Actors 배열을 LevelActors로 동기화하기 때문)
						PIELevel->GetLevelActors().push_back(TObjectPtr<AActor>(PIEActor));
						duplicatedCount++;
						UE_LOG("DuplicateWorldForPIE: Added to PIE Level Actors (%zu) and LevelActors (%zu)", PIEActorsArray.size(), PIELevel->GetLevelActors().size());
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
    for (AActor* Actor : Level->GetActors())
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
    for (AActor* Actor : Level->GetActors())
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
