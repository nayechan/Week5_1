#pragma once
#include "Factory.h"
#include "Factory/Actor/Public/ActorFactory.h"
#include "Core/Public/ObjectPtr.h"

using std::is_base_of_v;

/**
 * @brief 클래스를 지정하여 새 객체를 생성하는 함수
 * @tparam T 생성할 객체의 타입
 * @param InOuter 생성될 객체의 부모
 * @param InClass 생성할 클래스 (nullptr이면 T::StaticClass() 사용)
 * @param InName 객체 이름
 * @param InFlags 객체 플래그
 * @return 생성된 객체
 */
template <typename T>
TObjectPtr<T> NewObject(TObjectPtr<UObject> InOuter = nullptr, TObjectPtr<UClass> InClass = nullptr,
	const FName& InName = FName::GetNone(), uint32 InFlags = 0)
{
	static_assert(is_base_of_v<UObject, T>, "생성할 클래스는 UObject를 반드시 상속 받아야 합니다");

	TObjectPtr<UClass> ClassToUse = InClass ? InClass : T::StaticClass();

	// NOTE: Factory는 현시점에서 필요하지 않다고 생각되어 제외함
	
	//// Factory를 사용하여 생성 시도
	//TObjectPtr<UFactory> Factory = UFactory::FindFactory(ClassToUse);
	//if (Factory)
	//{
	//	TObjectPtr<UObject> NewObject = Factory->FactoryCreateNew(ClassToUse, InOuter, InName, InFlags);
	//	if (NewObject)
	//	{
	//		return Cast<T>(NewObject);
	//	}
	//}

	// Factory가 없으면 기존 방식으로 폴백
	TObjectPtr<T> NewObject;
	if (InClass && InClass->IsChildOf(T::StaticClass()))
	{
		NewObject = Cast<T>(InClass->CreateDefaultObject());
	}
	else
	{
		NewObject = TObjectPtr<T>(new T());
	}

	if (NewObject)
	{
		if (InName != FName::GetNone())
		{
			NewObject->SetName(InName);
		}
		if (InOuter)
		{
			NewObject->SetOuter(InOuter);
		}
	}

	return NewObject;
}
