#include "pch.h"
#include "Factory/Actor/Public/SphereActorFactory.h"
#include "Actor/Public/SphereActor.h"

IMPLEMENT_CLASS(USphereActorFactory, UActorFactory)

/**
 * @brief Factory Constructor
 * Factory 등록도 함께 수행
 */
USphereActorFactory::USphereActorFactory()
{
	SupportedClass = ASphereActor::StaticClass();
	Description = "SphereActor Factory";

	RegisterFactory(TObjectPtr<UFactory>(this));
}

/**
 * @brief SphereActor 인스턴스를 생성합니다
 * @return 생성된 SphereActor
 */
TObjectPtr<AActor> USphereActorFactory::CreateNewActor()
{
	UE_LOG_SUCCESS("SphereActorFactory: Creating new SphereActor instance");
	return TObjectPtr<AActor>(new ASphereActor);
}
