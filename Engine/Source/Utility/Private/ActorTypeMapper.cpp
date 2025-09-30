#include "pch.h"
#include "Utility/Public/ActorTypeMapper.h"

#include "Global/Types.h"
#include "Component/Mesh/Public/CubeComponent.h"
#include "Component/Mesh/Public/SphereComponent.h"
#include "Component/Mesh/Public/TriangleComponent.h"
#include "Component/Mesh/Public/SquareComponent.h"
#include "Component/Public/LineComponent.h"
#include "Component/Public/SceneComponent.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Actor/Public/CubeActor.h"
#include "Actor/Public/SphereActor.h"
#include "Actor/Public/SquareActor.h"
#include "Actor/Public/StaticMeshActor.h"
#include "Actor/Public/TriangleActor.h"

// 하드 코딩으로 구현
FString FActorTypeMapper::ActorToType(UClass* InClass)
{
	FName TypeName = InClass->GetClassTypeName();

	if (TypeName == ACubeActor::StaticClass()->GetClassTypeName())
	{
		return "Cube";
	}
	else if (TypeName == ASphereActor::StaticClass()->GetClassTypeName())
	{
		return "Sphere";
	}
	else if (TypeName == ASquareActor::StaticClass()->GetClassTypeName())
	{
		return "Square";
	}
	else if (TypeName == ATriangleActor::StaticClass()->GetClassTypeName())
	{
		return "Triangle";
	}
	else if (TypeName == AStaticMeshActor::StaticClass()->GetClassTypeName())
	{
		return "StaticMeshComp";
	}
}

// 하드 코딩으로 구현
UClass* FActorTypeMapper::TypeToActor(const FString& InTypeString)
{
	UClass* NewActorClass = nullptr;

	// 타입에 따라 액터 생성
	if (InTypeString == "Cube")
	{
		NewActorClass = ACubeActor::StaticClass();
	}
	else if (InTypeString == "Sphere")
	{
		NewActorClass = ASphereActor::StaticClass();
	}
	else if (InTypeString == "Triangle")
	{
		NewActorClass = ATriangleActor::StaticClass();
	}
	else if (InTypeString == "Square")
	{
		NewActorClass = ASquareActor::StaticClass();
	}
	else if (InTypeString == "StaticMeshComp")
	{
		NewActorClass = AStaticMeshActor::StaticClass();
	}

	return NewActorClass;
}
