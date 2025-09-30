#include "pch.h"
#include "Factory/Actor/Public/TriangleActorFactory.h"
#include "Actor/Public/TriangleActor.h"

IMPLEMENT_CLASS(UTriangleActorFactory, UActorFactory)

/**
 * @brief Factory Constructor
 * Factory 등록도 함께 수행
 */
UTriangleActorFactory::UTriangleActorFactory()
{
	SupportedClass = ATriangleActor::StaticClass();
	Description = "TriangleActor Factory";

	RegisterFactory(TObjectPtr<UFactory>(this));
}

/**
 * @brief TriangleActor 인스턴스를 생성합니다
 * @return 생성된 TriangleActor
 */
TObjectPtr<AActor> UTriangleActorFactory::CreateNewActor()
{
	UE_LOG_SUCCESS("TriangleActorFactory: Creating new TriangleActor instance");
	return TObjectPtr<AActor>(new ATriangleActor);
}
