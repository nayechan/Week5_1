#include "pch.h"
#include "Factory/Actor/Public/ActorFactory.h"
#include "Actor/Public/Actor.h"
#include "Level/Public/Level.h"

IMPLEMENT_CLASS(UActorFactory, UFactory)

/**
 * @brief Factory Constructor
 */
UActorFactory::UActorFactory()
{
	SupportedClass = AActor::StaticClass();
	Description = "Actor Factory";
}

/**
 * @brief Actor를 생성합니다
 * @param InWorld 액터가 생성될 월드 (현재는 미사용)
 * @param InLevel 액터가 생성될 레벨
 * @param InTransform 액터의 초기 Transform
 * @param InObjectFlags 객체 플래그
 * @return 생성된 액터 포인터
 */
TObjectPtr<AActor> UActorFactory::CreateActor(TObjectPtr<UObject> InWorld, TObjectPtr<ULevel> InLevel,
                                   const FTransform& InTransform, uint32 InObjectFlags)
{
	if (!InLevel)
	{
		UE_LOG_WARNING("ActorFactory: 유효한 레벨 없이 Actor를 생성할 수 없습니다");
		return nullptr;
	}

	// UFactory의 FactoryCreateNew를 사용하여 생성
	TObjectPtr<AActor> NewActor = Cast<AActor>(
		FactoryCreateNew(GetSupportedClass(), InLevel, FName::GetNone(), InObjectFlags));

	if (NewActor)
	{
		// Transform 적용
		PostCreateActor(NewActor, InTransform);

		UE_LOG_SUCCESS("ActorFactory: %s에서 Actor를 성공적으로 생성했습니다", GetDescription().data());
	}

	return NewActor;
}

/**
 * @brief UFactory::CreateNew 구현
 * @return 생성된 객체
 */
TObjectPtr<UObject> UActorFactory::CreateNew()
{
	return CreateNewActor();
}

/**
 * @brief Actor 생성 후 추가 설정
 * @param InActor 생성된 액터
 * @param InTransform 초기 변환
 */
void UActorFactory::PostCreateActor(AActor* InActor, const FTransform& InTransform)
{
	if (!InActor)
	{
		return;
	}

	// Transform 적용
	InActor->SetActorLocation(InTransform.Location);
	InActor->SetActorRotation(InTransform.Rotation);
	InActor->SetActorScale3D(InTransform.Scale);

	UE_LOG_SUCCESS("ActorFactory: %s에 대한 PostCreateActor 작업 완료",
	               InActor->GetClass()->GetClassTypeName().ToString().data());
}
