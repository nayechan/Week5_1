#include "pch.h"
#include "Factory/Public/Factory.h"

IMPLEMENT_CLASS(UFactory, UObject)

// 정적 팩토리 목록을 안전하게 초기화하는 함수
static TArray<TObjectPtr<UFactory>>& GetFactoryListImpl()
{
	static TArray<TObjectPtr<UFactory>> FactoryList;
	return FactoryList;
}

/**
 * @brief 생성자
 */
UFactory::UFactory()
	: SupportedClass(nullptr)
	  , Description("Generic Factory")
{
}

/**
 * @brief 새 객체를 생성하는 함수
 * @param InClass 생성할 객체의 클래스
 * @param InParent 생성될 객체의 부모 (Outer)
 * @param InName 객체 이름
 * @param InFlags 객체 플래그
 * @param InContext 생성 컨텍스트 (현재는 미사용)
 * @param InWarning 경고 출력용 (현재는 미사용)
 * @return 생성된 객체 포인터
 */
TObjectPtr<UObject> UFactory::FactoryCreateNew(TObjectPtr<UClass> InClass, TObjectPtr<UObject> InParent, const FName& InName,
                                    uint32 InFlags, TObjectPtr<UObject> InContext, void* InWarning)
{
	if (!DoesSupportClass(InClass))
	{
		UE_LOG("Factory: 이 Factory에서 %s 타입의 클래스 객체를 생성할 수 없습니다",
		       InClass ? InClass->GetClassTypeName().ToString().data() : "nullptr");
		return nullptr;
	}

	// 새 객체 생성
	TObjectPtr<UObject> NewObject = CreateNew();

	if (NewObject)
	{
		// 이름 설정
		if (InName != FName::GetNone())
		{
			NewObject->SetName(InName);
		}

		// Outer 설정
		if (InParent)
		{
			NewObject->SetOuter(InParent);
		}

		UE_LOG("Factory: %s를 통해 %s를 생성했습니다",
		       Description.data(),
		       InClass ? InClass->GetClassTypeName().ToString().data() : "Unknown");
	}

	return NewObject;
}

/**
 * @brief 이 팩토리가 지정된 클래스를 지원하는지 확인하는 함수
 * @param InClass 확인할 클래스
 * @return 지원하면 true
 */
bool UFactory::DoesSupportClass(UClass* InClass)
{
	if (!InClass || !SupportedClass)
	{
		return false;
	}

	return InClass->IsChildOf(SupportedClass);
}

/**
 * @brief 이 팩토리가 생성하는 클래스를 반환
 * @return 지원하는 클래스
 */
TObjectPtr<UClass> UFactory::GetSupportedClass() const
{
	return SupportedClass;
}

/**
 * @brief 정적 팩토리 목록 반환
 */
TArray<TObjectPtr<UFactory>>& UFactory::GetFactoryList()
{
	return GetFactoryListImpl();
}

/**
 * @brief 팩토리 등록 함수
 */
void UFactory::RegisterFactory(TObjectPtr<UFactory> InFactory)
{
	if (InFactory)
	{
		GetFactoryListImpl().push_back(InFactory);
		UE_LOG("Factory: %s 등록 완료", InFactory->GetDescription().data());
	}
}

/**
 * @brief 지정된 클래스를 생성할 수 있는 팩토리를 찾는 함수
 */
TObjectPtr<UFactory> UFactory::FindFactory(TObjectPtr<UClass> InClass)
{
	TArray<TObjectPtr<UFactory>>& FactoryList = GetFactoryListImpl();
	for (TObjectPtr<UFactory> Factory : FactoryList)
	{
		if (Factory && Factory->DoesSupportClass(InClass))
		{
			return Factory;
		}
	}

	return nullptr;
}
