#include "pch.h"
#include "Core/Public/Class.h"

#include "Core/Public/Object.h"

using std::stringstream;

/**
 * @brief '최초 사용 시 생성' 기법을 적용한 클래스 레지스트리 접근자
 * @return 모든 UClass 정보를 담고 있는 정적 TArray의 참조
 */
TArray<TObjectPtr<UClass>>& UClass::GetAllClasses()
{
	// 이 함수가 최초로 호출될 때 단 한 번만 안전하게 초기화됩니다.
	static TArray<TObjectPtr<UClass>> AllClasses;
	return AllClasses;
}
/**
 * @brief UClass Constructor
 * @param InName Class 이름
 * @param InSuperClass Parent Class
 * @param InClassSize Class Size
 * @param InConstructor 생성자 함수 포인터
 */
UClass::UClass(const FName& InName, TObjectPtr<UClass> InSuperClass, size_t InClassSize,
	ClassConstructorType InConstructor)
	: ClassName(InName), SuperClass(InSuperClass), ClassSize(InClassSize), Constructor(InConstructor)
{
	UE_LOG("UClass: 클래스 등록: %s", ClassName.ToString().data());
}

/**
 * @brief 이 클래스가 지정된 클래스의 하위 클래스인지 확인
 * @param InClass 확인할 클래스
 * @return 하위 클래스이거나 같은 클래스면 true
 */
bool UClass::IsChildOf(const TObjectPtr<UClass> InClass) const
{
	if (!InClass)
	{
		return false;
	}

	// 자기 자신과 같은 클래스인 경우
	if (this == InClass)
	{
		return true;
	}

	// 부모 클래스들을 거슬러 올라가면서 확인
	const UClass* CurrentClass = this;
	while (CurrentClass)
	{
		if (CurrentClass->ClassName == InClass->ClassName)
		{
			return true;
		}

		CurrentClass = CurrentClass->SuperClass.Get();
	}

	return false;
}

/**
 * @brief 새로운 인스턴스 생성
 * @return 생성된 객체 포인터
 */
TObjectPtr<UObject> UClass::CreateDefaultObject() const
{
	if (Constructor)
	{
		return Constructor();
	}

	return nullptr;
}

/**
 * @brief 클래스 이름으로 UClass 찾기
 * @param InClassName 찾을 클래스 이름
 * @return 찾은 UClass 포인터 (없으면 nullptr)
 */
TObjectPtr<UClass> UClass::FindClass(const FName& InClassName)
{
	for (TObjectPtr<UClass> Class : GetAllClasses())
	{
		if (Class && Class->GetClassTypeName() == InClassName)
		{
			return Class;
		}
	}

	return nullptr;
}

/**
 * @brief 모든 UClass 등록
 * @param InClass 등록할 UClass
 */
void UClass::SignUpClass(TObjectPtr<UClass> InClass)
{
	if (InClass)
	{
		GetAllClasses().emplace_back(InClass);
		UE_LOG("UClass: Class registered: %s (Total: %llu)", InClass->GetClassTypeName().ToString().data(), GetAllClasses().size());
	}
}

/**
 * @brief 등록된 모든 클래스 출력
 * For Debugging
 */
void UClass::PrintAllClasses()
{
	UE_LOG("=== Registered Classes (%llu) ===", GetAllClasses().size());

	for (size_t i = 0; i < GetAllClasses().size(); ++i)
	{
		UClass* Class = GetAllClasses()[i];

		stringstream ss;
		ss << Class->GetClassTypeName().ToString();

		if (Class)
		{
			ss << "[" << i << "] " << Class->GetClassTypeName().ToString()
				<< " (Size: " << Class->GetClassSize() << " bytes)";

			if (Class->GetSuperClass())
			{
				ss << " -> " << Class->GetSuperClass()->GetClassTypeName().ToString();
			}
			else
			{
				ss << " (Base Class)";
			}
			UE_LOG("%s", ss.str().c_str());
		}
	}

	UE_LOG("================================");
}

/**
 * @brief 안전하게 종료 전 ClassObject에 할당한 메모리를 free하는 함수
 */
void UClass::Shutdown()
{
	for (TObjectPtr<UClass>& ClassObject : GetAllClasses())
	{
		if (ClassObject.Get())
		{
			FString ClassName = ClassObject->GetClassTypeName().ToString();
			UE_LOG_WARNING("System: GC: %s에 해제되지 않은 메모리가 있습니다", ClassName.data());
			delete ClassObject;
			UE_LOG_SUCCESS("System: GC: %s에 할당한 메모리를 해제했습니다", ClassName.data());
		}
	}

	(void)GetAllClasses().empty();
}

TArray<TObjectPtr<UClass>> UClass::GetSubclassesOf(TObjectPtr<UClass> InParentClass)
{
	TArray<TObjectPtr<UClass>> SubClasses;
	if (!InParentClass)
	{
		return SubClasses;
	}

	// private static 함수인 GetAllClasses()를 클래스 내부에서 안전하게 호출
	const auto& AllClasses = GetAllClasses();
	for (const auto& Class : AllClasses)
	{
		if (Class && Class->IsChildOf(InParentClass) && Class != InParentClass)
		{
			SubClasses.push_back(Class);
		}
	}
	return SubClasses;
}
