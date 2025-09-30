#include "pch.h"
#include "Factory/Component/Public/ComponentFactory.h"

#include "Component/Mesh/Public/CubeComponent.h"
#include "Component/Mesh/Public/SphereComponent.h"
#include "Component/Mesh/Public/TriangleComponent.h"
#include "Component/Mesh/Public/SquareComponent.h"
#include "Component/Public/LineComponent.h"
#include "Component/Public/SceneComponent.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"

// NOTE: Factory는 현시점에서 필요하지 않다고 생각되어 제외함

/*
static TObjectPtr<TComponentFactory<UCubeComponent>> GCubeComponentFactory;
static TObjectPtr<TComponentFactory<USphereComponent>> GSphereComponentFactory;
static TObjectPtr<TComponentFactory<UTriangleComponent>> GTriangleComponentFactory;
static TObjectPtr<TComponentFactory<USquareComponent>> GSquareComponentFactory;
static TObjectPtr<TComponentFactory<ULineComponent>> GLineComponentFactory;
static TObjectPtr<TComponentFactory<USceneComponent>> GSceneComponentFactory;
static TObjectPtr<TComponentFactory<UPrimitiveComponent>> GPrimitiveComponentFactory;
static TObjectPtr<TComponentFactory<UStaticMeshComponent>> GStaticMeshComponentFactory;
*/

// 생성자에서 정적 인스턴스에 대한 Register 함수 호출
struct FComponentFactoryRegistrar
{
	FComponentFactoryRegistrar()
	{
		// NOTE: Factory는 현시점에서 필요하지 않다고 생각되어 제외함

		/*
		UFactory::RegisterFactory(GCubeComponentFactory);
		UFactory::RegisterFactory(GSphereComponentFactory);
		UFactory::RegisterFactory(GTriangleComponentFactory);
		UFactory::RegisterFactory(GSquareComponentFactory);
		UFactory::RegisterFactory(GLineComponentFactory);
		UFactory::RegisterFactory(GSceneComponentFactory);
		UFactory::RegisterFactory(GPrimitiveComponentFactory);
		UFactory::RegisterFactory(GStaticMeshComponentFactory);
		*/
		UE_LOG_SUCCESS("ComponentFactories: 모든 Component Factory 클래스 등록이 완료되었습니다");
	}
};

// 정적 인스턴스로 자동 등록
static FComponentFactoryRegistrar GComponentFactoryRegistrar;
