#include "pch.h"
#include "Actor/Public/Actor.h"
#include "Component/Public/SceneComponent.h"
#include "Component/Public/BillBoardComponent.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"

IMPLEMENT_CLASS(AActor, UObject)

AActor::AActor()
{
	// to do: primitive factory로 빌보드 생성
	BillBoardComponent = new UBillBoardComponent(this, 5.0f);
	OwnedComponents.push_back(TObjectPtr<UBillBoardComponent>(BillBoardComponent));
}

AActor::AActor(UObject* InOuter)
{
	SetOuter(InOuter);
}

AActor::~AActor()
{
	for (UActorComponent* Component : OwnedComponents)
	{
		SafeDelete(Component);
	}
	SetOuter(nullptr);
	OwnedComponents.clear();
}

void AActor::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (RootComponent)
	{
		RootComponent->Serialize(bInIsLoading, InOutHandle);
	}
}

void AActor::SetActorLocation(const FVector& InLocation) const
{
	if (RootComponent)
	{
		RootComponent->SetRelativeLocation(InLocation);
	}
}

void AActor::SetActorRotation(const FVector& InRotation) const
{
	if (RootComponent)
	{
		RootComponent->SetRelativeRotation(InRotation);
	}
}

void AActor::SetActorScale3D(const FVector& InScale) const
{
	if (RootComponent)
	{
		RootComponent->SetRelativeScale3D(InScale);
	}
}

void AActor::SetUniformScale(bool IsUniform)
{
	if (RootComponent)
	{
		RootComponent->SetUniformScale(IsUniform);
	}
}

bool AActor::IsUniformScale() const
{
	if (RootComponent)
	{
		return RootComponent->IsUniformScale();
	}
	return false;
}

const FVector& AActor::GetActorLocation() const
{
	assert(RootComponent);
	return RootComponent->GetRelativeLocation();
}

const FVector& AActor::GetActorRotation() const
{
	assert(RootComponent);
	return RootComponent->GetRelativeRotation();
}

const FVector& AActor::GetActorScale3D() const
{
	assert(RootComponent);
	return RootComponent->GetRelativeScale3D();
}

void AActor::Tick(float DeltaTime)
{
	// 소유한 모든 컴포넌트의 Tick 처리
	for (UActorComponent* Component : OwnedComponents)
	{
		if (Component && Component->IsComponentTickEnabled())
		{
			Component->TickComponent(DeltaTime);
		}
	}
}

void AActor::BeginPlay()
{
	// 모든 컴포넌트의 BeginPlay 호출
	for (UActorComponent* Component : OwnedComponents)
	{
		if (Component)
		{
			Component->BeginPlay();
		}
	}
}

void AActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 모든 컴포넌트의 EndPlay 호출
	for (UActorComponent* Component : OwnedComponents)
	{
		if (Component)
		{
			Component->EndPlay(EndPlayReason);
		}
	}
}

void AActor::DuplicateSubObjects()
{
	Super::DuplicateSubObjects();
	
	// 복제 후 모든 컴포넌트의 Owner를 이 Actor로 재설정
	if (RootComponent)
	{
		RootComponent->SetOwner(this);
	}
	
	if (BillBoardComponent)
	{
		BillBoardComponent->SetOwner(this);
	}
	
	for (auto& Component : OwnedComponents)
	{
		if (Component)
		{
			Component->SetOwner(this);
		}
	}
	
	UE_LOG("AActor::DuplicateSubObjects: Fixed component owners for %s", GetName().ToString().data());
}

UObject* AActor::Duplicate()
{
	UE_LOG("AActor::Duplicate: Starting duplication of %s (UUID: %u)", GetName().ToString().data(), GetUUID());
	
	// NewObject를 사용하여 새로운 Actor 생성 (올바른 Factory 사용)
	AActor* NewActor = NewObject<AActor>(nullptr, GetClass());
	if (!NewActor)
	{
		UE_LOG("AActor::Duplicate: Failed to create new actor!");
		return nullptr;
	}
	
	UE_LOG("AActor::Duplicate: New actor created with UUID: %u", NewActor->GetUUID());
	
	// AActor 고유 속성들 복사
	NewActor->bActorTickEnabled = bActorTickEnabled;
	NewActor->bTickInEditor = bTickInEditor;
	
	// 서브 오브젝트들을 깊은 복사로 복제 (먼저 RootComponent 생성)
	NewActor->DuplicateSubObjects();
	
	// 컴포넌트별 추가 복제 작업 (메시 정보 등)
	CopyComponentData(NewActor);
	
	// Transform 정보 복사 (RootComponent가 복제된 후)
	if (RootComponent && NewActor->GetRootComponent())
	{
		UE_LOG("AActor::Duplicate: Copying transform from %s to %s", 
		       GetName().ToString().data(), NewActor->GetName().ToString().data());
		       
		FVector OriginalLocation = RootComponent->GetRelativeLocation();
		FVector OriginalRotation = RootComponent->GetRelativeRotation();
		FVector OriginalScale = RootComponent->GetRelativeScale3D();
		
		NewActor->GetRootComponent()->SetRelativeLocation(OriginalLocation);
		NewActor->GetRootComponent()->SetRelativeRotation(OriginalRotation);
		NewActor->GetRootComponent()->SetRelativeScale3D(OriginalScale);
		
		UE_LOG("AActor::Duplicate: Transform copied - Pos(%.2f,%.2f,%.2f) Rot(%.2f,%.2f,%.2f) Scale(%.2f,%.2f,%.2f)",
		       OriginalLocation.X, OriginalLocation.Y, OriginalLocation.Z,
		       OriginalRotation.X, OriginalRotation.Y, OriginalRotation.Z,
		       OriginalScale.X, OriginalScale.Y, OriginalScale.Z);
	}
	else
	{
		UE_LOG("AActor::Duplicate: Warning - Could not copy transform (RootComponent: orig=%p, new=%p)", 
		       RootComponent.Get(), NewActor->GetRootComponent());
	}
	
	UE_LOG("AActor::Duplicate: Duplication completed for %s (UUID: %u) -> %s (UUID: %u)", 
	       GetName().ToString().data(), GetUUID(), 
	       NewActor->GetName().ToString().data(), NewActor->GetUUID());
	
	return NewActor;
}

void AActor::CopyComponentData(AActor* TargetActor)
{
	if (!TargetActor)
	{
		return;
	}
	
	UE_LOG("AActor::CopyComponentData: Copying component data from %s to %s", 
	       GetName().ToString().data(), TargetActor->GetName().ToString().data());
	
	// 원본과 대상의 컴포넌트 개수가 다르면 경고
	if (OwnedComponents.size() != TargetActor->OwnedComponents.size())
	{
		UE_LOG("AActor::CopyComponentData: Warning - Component count mismatch (Source: %zu, Target: %zu)",
		       OwnedComponents.size(), TargetActor->OwnedComponents.size());
	}
	
	// 컴포넌트 데이터 복사
	size_t ComponentCount = std::min(OwnedComponents.size(), TargetActor->OwnedComponents.size());
	for (size_t i = 0; i < ComponentCount; ++i)
	{
		UActorComponent* SourceComponent = OwnedComponents[i];
		UActorComponent* TargetComponent = TargetActor->OwnedComponents[i];
		
		if (!SourceComponent || !TargetComponent)
		{
			continue;
		}
		
		// StaticMeshComponent 인 경우 메시 데이터 복사
		UStaticMeshComponent* SourceMeshComp = Cast<UStaticMeshComponent>(SourceComponent);
		UStaticMeshComponent* TargetMeshComp = Cast<UStaticMeshComponent>(TargetComponent);
		
		if (SourceMeshComp && TargetMeshComp && SourceMeshComp->GetStaticMesh())
		{
			// 메시 에셋 경로를 사용하여 대상 컴포넌트에 설정
			FName AssetPath = SourceMeshComp->GetStaticMesh()->GetAssetPathFileName();
			TargetMeshComp->SetStaticMesh(AssetPath);
			
			UE_LOG("AActor::CopyComponentData: Copied StaticMesh data: %s", 
			       AssetPath.ToString().data());
		}
	}
	
	UE_LOG("AActor::CopyComponentData: Component data copying completed");
}
