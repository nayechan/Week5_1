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
        return nullptr;

    // 새로운 PIE 월드 생성
    UWorld* PIEWorld = NewObject<UWorld>(nullptr, UWorld::StaticClass(), FName("PIEWorld"));
    if (!PIEWorld)
        return nullptr;

    PIEWorld->SetWorldType(EWorldType::PIE);

    // 에디터 월드의 레벨을 복제
    if (EditorWorld->GetLevel())
    {
        ULevel* EditorLevel = EditorWorld->GetLevel();
        
        // 새로운 PIE 레벨 생성
        ULevel* PIELevel = NewObject<ULevel>(TObjectPtr<UObject>(PIEWorld), ULevel::StaticClass(), FName("PIELevel"));
        
        if (PIELevel)
        {
            // 에디터 레벨의 모든 액터들을 PIE 레벨로 복제
            // GetActors()는 const reference이므로 SpawnActorToLevel 사용
            for (AActor* EditorActor : EditorLevel->GetActors())
            {
                if (EditorActor)
                {
                    // 액터 복제 (얕은 복사 + 서브오브젝트 깊은 복사)
                    AActor* PIEActor = Cast<AActor>(EditorActor->Duplicate());
                    if (PIEActor)
                    {
                        // SpawnActorToLevel은 BeginPlay를 호출하므로, 직접 추가
                        PIELevel->AddActorDirect(PIEActor);
                    }
                }
            }

            PIEWorld->SetLevel(PIELevel);
        }
    }

    return PIEWorld;
}

void UWorld::InitializeActorsForPlay()
{
    if (!Level)
        return;

    // Level 초기화 (Octree 구축 등)
    Level->Init();

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
