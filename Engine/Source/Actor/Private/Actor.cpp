#include "pch.h"
#include "Actor/Public/Actor.h"
#include "Component/Public/SceneComponent.h"
#include "Component/Public/BillBoardComponent.h"

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
