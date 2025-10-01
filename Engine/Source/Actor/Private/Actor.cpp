#include "pch.h"
#include "Actor/Public/Actor.h"
#include "Component/Public/SceneComponent.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Manager/Level/Public/LevelManager.h"
#include "Level/Public/Level.h"

IMPLEMENT_CLASS(AActor, UObject)

AActor::AActor()
{}

AActor::AActor(UObject* InOuter)
{
	SetOuter(InOuter);
}

AActor::~AActor()
{
	// 직접 소유한 컴포넌트만 삭제 (자식 컴포넌트는 부모가 삭제할 때 자동으로 처리됨)
	UE_LOG("AActor::~AActor(): Destroying %s with %d owned components", 
	       GetName().ToString().c_str(), OwnedComponents.size());
	
	// 역순으로 삭제 (나중에 생성된 것부터 먼저 삭제)
	for (int32 i = OwnedComponents.size() - 1; i >= 0; --i)
	{
		UActorComponent* Component = OwnedComponents[i].Get();
		if (Component)
		{
			UE_LOG("  Deleting component: %s (Type: %d)", 
			       Component->GetName().ToString().c_str(),
			       static_cast<int>(Component->GetComponentType()));
			
			// SceneComponent인 경우 부모 관계만 정리 (자식들은 소멸자에서 처리)
			if (USceneComponent* SceneComp = Cast<USceneComponent>(Component))
			{
				// 부모 관계 해제 - 자식들은 소멸자에서 자동으로 처리됨
				SceneComp->SetParentAttachment(nullptr);
			}
			
			SafeDelete(Component);
		}
	}
	
	SetOuter(nullptr);
	OwnedComponents.clear();
	
	UE_LOG("AActor::~AActor(): Destruction completed for %s", GetName().ToString().c_str());
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

UActorComponent* AActor::AddComponentByClass(UClass* ComponentClass, const FName& ComponentName)
{
	if (!ComponentClass)
	{
		return nullptr;
	}

	TObjectPtr<UClass> ComponentClassPtr(ComponentClass);
	TObjectPtr<UActorComponent> NewComponent = NewObject<UActorComponent>(TObjectPtr<UObject>(this),
		ComponentClassPtr, ComponentName);

	if (NewComponent)
	{
		// 액터를 컴포넌트의 소유자로 설정하고, 소유 목록에 추가
		NewComponent->SetOwner(this);
		OwnedComponents.push_back(NewComponent);

		// PrimitiveComponent 타입이라면, 레벨의 렌더링 목록에 등록
		if (UPrimitiveComponent* NewPrimitive = Cast<UPrimitiveComponent>(NewComponent.Get()))
		{
			ULevel* CurrentLevel = ULevelManager::GetInstance().GetCurrentLevel();
			if (CurrentLevel)
			{
				CurrentLevel->RegisterPrimitiveComponent(NewPrimitive);
			}
		}
	}

	return NewComponent.Get();
}

void AActor::RegisterComponent(UActorComponent* Component)
{
	if (!Component)
	{
		return;
	}

	// 중복 방지
	for (const auto& ExistingComp : OwnedComponents)
	{
		if (ExistingComp.Get() == Component)
		{
			UE_LOG("AActor::RegisterComponent: Component %s already registered", Component->GetName().ToString().c_str());
			return;
		}
	}

	// 컴포넌트 등록
	OwnedComponents.push_back(TObjectPtr<UActorComponent>(Component));
	Component->SetOwner(this);

	// PrimitiveComponent인 경우 Level에 등록
	if (UPrimitiveComponent* PrimitiveComp = Cast<UPrimitiveComponent>(Component))
	{
		ULevel* CurrentLevel = ULevelManager::GetInstance().GetCurrentLevel();
		if (CurrentLevel)
		{
			CurrentLevel->RegisterPrimitiveComponent(PrimitiveComp);
		}
	}

	UE_LOG("AActor::RegisterComponent: Component %s registered to %s", 
	       Component->GetName().ToString().c_str(), GetName().ToString().c_str());
}

void AActor::UnregisterComponent(UActorComponent* Component)
{
	if (!Component)
	{
		return;
	}

	// OwnedComponents에서 제거
	for (auto it = OwnedComponents.begin(); it != OwnedComponents.end(); ++it)
	{
		if (it->Get() == Component)
		{
			OwnedComponents.erase(it);
			break;
		}
	}

	// PrimitiveComponent인 경우 Level에서 제거
	if (UPrimitiveComponent* PrimitiveComp = Cast<UPrimitiveComponent>(Component))
	{
		ULevel* CurrentLevel = ULevelManager::GetInstance().GetCurrentLevel();
		if (CurrentLevel)
		{
			// Level에서 Primitive 제거 로직 호출 필요
			// 현재 Level 클래스에 이러한 메서드가 없으므로 필요 시 추가
		}
	}

	UE_LOG("AActor::UnregisterComponent: Component %s unregistered from %s", 
	       Component->GetName().ToString().c_str(), GetName().ToString().c_str());
}

// Helper function to recursively collect scene component children
static void CollectSceneComponentChildren(USceneComponent* SceneComp, TArray<UActorComponent*>& OutComponents, int depth = 0)
{
	if (!SceneComp) return;

	// Add all direct children
	for (USceneComponent* Child : SceneComp->GetAttachChildren())
	{
		if (Child)
		{
			OutComponents.push_back(Child);

			// Recursively collect children of children
			CollectSceneComponentChildren(Child, OutComponents, depth + 1);
		}
	}
}

TArray<UActorComponent*> AActor::GetAllComponents() const
{
	TArray<UActorComponent*> AllComponents;

	// Collect all directly owned components
	for (const auto& Component : OwnedComponents)
	{
		if (Component)
		{
			AllComponents.push_back(Component.Get());

			// If this is a SceneComponent, recursively collect its entire hierarchy
			if (USceneComponent* SceneComp = Cast<USceneComponent>(Component.Get()))
			{
				CollectSceneComponentChildren(SceneComp, AllComponents, 1);
			}
		}
	}

	return AllComponents;
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
	
	// 모든 컴포넌트 (child component 포함) Owner 재설정
	TArray<UActorComponent*> AllComponents = GetAllComponents();
	UE_LOG("AActor::DuplicateSubObjects: Processing %d total components (including children) for %s", 
	       AllComponents.size(), GetName().ToString().data());
	       
	for (UActorComponent* Component : AllComponents)
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
	
	// CRITICAL: Manually duplicate child components that weren't copied by DuplicateSubObjects
	DuplicateChildComponents(NewActor);
	
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

		//fix buf Force update world Transform immediately after copying
		if (USceneComponent* NewRootScene = Cast<USceneComponent>(NewActor->GetRootComponent()))
		{
			NewRootScene->UpdateWorldTransform();
		}
		
		
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

// Helper function to recursively duplicate scene component hierarchy
static USceneComponent* DuplicateSceneComponentRecursive(USceneComponent* SourceComponent, AActor* TargetActor, USceneComponent* TargetParent, int depth = 0)
{
	if (!SourceComponent || !TargetActor)
	{
		return nullptr;
	}
	
	FString indent(depth * 2, ' ');
	UE_LOG("%sRecursively duplicating component: %s (depth=%d)", 
		   indent.c_str(), SourceComponent->GetName().ToString().c_str(), depth);
	
	// Duplicate the component
	USceneComponent* NewComponent = static_cast<USceneComponent*>(SourceComponent->Duplicate());
	if (!NewComponent)
	{
		UE_LOG("%s  Failed to duplicate component: %s", 
			   indent.c_str(), SourceComponent->GetName().ToString().c_str());
		return nullptr;
	}
	
	// Set owner and register
	NewComponent->SetOwner(TargetActor);
	TargetActor->RegisterComponent(NewComponent);
	
	// Attach to parent if provided
	if (TargetParent)
	{
		NewComponent->SetParentAttachment(TargetParent);
		UE_LOG("%s  Attached %s to parent %s", 
			   indent.c_str(), NewComponent->GetName().ToString().c_str(),
			   TargetParent->GetName().ToString().c_str());
	}
	
	// Recursively duplicate all children
	for (USceneComponent* ChildComponent : SourceComponent->GetAttachChildren())
	{
		if (ChildComponent)
		{
			DuplicateSceneComponentRecursive(ChildComponent, TargetActor, NewComponent, depth + 1);
		}
	}
	
	UE_LOG("%s  Successfully duplicated component: %s (UUID: %u)", 
		   indent.c_str(), NewComponent->GetName().ToString().c_str(), NewComponent->GetUUID());
		   
	return NewComponent;
}

void AActor::DuplicateChildComponents(AActor* TargetActor)
{
	if (!TargetActor)
	{
		return;
	}
	
	UE_LOG("AActor::DuplicateChildComponents: Starting recursive child component duplication from %s to %s", 
	       GetName().ToString().data(), TargetActor->GetName().ToString().data());
	
	// Get all components from source (original) actor
	TArray<UActorComponent*> SourceAllComponents = GetAllComponents();
	TArray<UActorComponent*> TargetAllComponents = TargetActor->GetAllComponents();
	
	UE_LOG("  Source has %d total components, Target has %d total components", 
	       SourceAllComponents.size(), TargetAllComponents.size());
	
	// Find components that exist in source but not in target and duplicate them recursively
	for (UActorComponent* SourceComponent : SourceAllComponents)
	{
		if (!SourceComponent) continue;
		
		// Skip components that are already duplicated (check by name and type)
		bool bFoundInTarget = false;
		for (UActorComponent* TargetComponent : TargetAllComponents)
		{
			if (TargetComponent && 
			    TargetComponent->GetName() == SourceComponent->GetName() &&
			    TargetComponent->GetClass() == SourceComponent->GetClass())
			{
				bFoundInTarget = true;
				break;
			}
		}
		
		if (!bFoundInTarget)
		{
			UE_LOG("    Missing component in target: %s (Type: %d)", 
			       SourceComponent->GetName().ToString().c_str(),
			       static_cast<int>(SourceComponent->GetComponentType()));
			
			// Handle SceneComponent hierarchies recursively
			if (USceneComponent* SourceSceneComp = Cast<USceneComponent>(SourceComponent))
			{
				// Only duplicate if this component doesn't have a parent that we will duplicate
				// (to avoid duplicating the same subtree multiple times)
				USceneComponent* SourceParent = SourceSceneComp->GetAttachParent();
				bool bParentWillBeDuplicated = false;
				
				if (SourceParent)
				{
					// Check if the parent is also missing and will be duplicated
					for (UActorComponent* CheckComponent : SourceAllComponents)
					{
						if (CheckComponent == SourceParent)
						{
							// Check if parent is also missing in target
							bool bParentFoundInTarget = false;
							for (UActorComponent* TargetComp : TargetAllComponents)
							{
								if (TargetComp && 
								    TargetComp->GetName() == SourceParent->GetName() &&
								    TargetComp->GetClass() == SourceParent->GetClass())
								{
									bParentFoundInTarget = true;
									break;
								}
							}
							if (!bParentFoundInTarget)
							{
								bParentWillBeDuplicated = true;
							}
							break;
						}
					}
				}
				
				// Only duplicate root components of missing subtrees
				if (!bParentWillBeDuplicated)
				{
					// Find the target parent to attach to
					USceneComponent* TargetParent = nullptr;
					if (SourceParent)
					{
						// Find corresponding parent in target by name and type
						for (UActorComponent* TargetComp : TargetActor->GetAllComponents())
						{
							if (USceneComponent* TargetSceneComp = Cast<USceneComponent>(TargetComp))
							{
								if (TargetSceneComp->GetName() == SourceParent->GetName() &&
								    TargetSceneComp->GetClass() == SourceParent->GetClass())
								{
									TargetParent = TargetSceneComp;
									break;
								}
							}
						}
					}
					
					// Recursively duplicate this component and all its children
					DuplicateSceneComponentRecursive(SourceSceneComp, TargetActor, TargetParent);
				}
			}
			else
			{
				// For non-scene components, duplicate normally
				UActorComponent* NewComponent = static_cast<UActorComponent*>(SourceComponent->Duplicate());
				if (NewComponent)
				{
					NewComponent->SetOwner(TargetActor);
					TargetActor->RegisterComponent(NewComponent);
					
					UE_LOG("      Successfully duplicated non-scene component: %s (UUID: %u)", 
					       NewComponent->GetName().ToString().c_str(), NewComponent->GetUUID());
				}
				else
				{
					UE_LOG("      Failed to duplicate non-scene component: %s", 
					       SourceComponent->GetName().ToString().c_str());
				}
			}
		}
	}
	
	UE_LOG("AActor::DuplicateChildComponents: Recursive child component duplication completed");
}
