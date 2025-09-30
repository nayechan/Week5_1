#include "pch.h"
#include "Factory/Actor/Public/StaticMeshActorFactory.h"
#include "Actor/Public/StaticMeshActor.h"

IMPLEMENT_CLASS(UStaticMeshActorFactory, UActorFactory)


UStaticMeshActorFactory::UStaticMeshActorFactory()
{
	SupportedClass = AStaticMeshActor::StaticClass();
	Description = "StaticMeshActor Factory";

	RegisterFactory(TObjectPtr<UFactory>(this));
}

/**
 * @brief StaticMeshActor 인스턴스를 생성합니다
 * @return 생성된 StaticMeshActor
 */
TObjectPtr<AActor> UStaticMeshActorFactory::CreateNewActor()
{
	UE_LOG_SUCCESS("StaticMeshActorFactory: Creating new StaticMeshActor instance");
	return TObjectPtr<AActor>(new AStaticMeshActor);
}
