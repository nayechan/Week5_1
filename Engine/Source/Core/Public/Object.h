#pragma once
#include "Class.h"
#include "Name.h"
#include "ObjectPtr.h"

namespace json { class JSON; }
using JSON = json::JSON;

UCLASS()
class UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UObject, UObject)

public:
	// 생성자 및 소멸자
	UObject();
	explicit UObject(const FName& InName);
	virtual ~UObject();

	// 2. 가상 함수 (인터페이스)
	virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle);

	// 3. Public 멤버 함수
	bool IsA(TObjectPtr<UClass> InClass) const;
	void AddMemoryUsage(uint64 InBytes, uint32 InCount);
	void RemoveMemoryUsage(uint64 InBytes, uint32 InCount);

	// Getter & Setter
	const FName& GetName() const { return Name; }
	UObject* GetOuter() const { return Outer.Get(); }
	uint64 GetAllocatedBytes() const { return AllocatedBytes; }
	uint32 GetAllocatedCount() const { return AllocatedCounts; }
	uint32 GetUUID() const { return UUID; }

	void SetName(const FName& InName) { Name = InName; }
	void SetOuter(UObject* InObject);
	void SetDisplayName(const FString& InName) const { Name.SetDisplayName(InName); }

private:
	// 4. Private 멤버 함수
	void PropagateMemoryChange(uint64 InBytesDelta, uint32 InCountDelta);

private:
	// 5. Private 멤버 변수
	uint32 UUID;
	uint32 InternalIndex;
	FName Name;
	TObjectPtr<UObject> Outer;
	uint64 AllocatedBytes = 0;
	uint32 AllocatedCounts = 0;
};


/**
 * @brief 안전한 타입 캐스팅 함수 (원시 포인터용)
 * UClass::IsChildOf를 사용한 런타임 타입 체크
 * @tparam T 캐스팅할 대상 타입
 * @param InObject 캐스팅할 원시 포인터
 * @return 캐스팅 성공시 T*, 실패시 nullptr
 */
template <typename T>
T* Cast(UObject* InObject)
{
	static_assert(std::is_base_of_v<UObject, T>, "Cast<T>: T는 UObject를 상속받아야 합니다");

	if (!InObject)
	{
		return nullptr;
	}

	// 런타임 타입 체크: InObject가 T 타입인가?
	if (InObject->IsA(T::StaticClass()))
	{
		return static_cast<T*>(InObject);
	}

	return nullptr;
}

/**
 * @brief 안전한 타입 캐스팅 함수 (const 원시 포인터용)
 * @tparam T 캐스팅할 대상 타입
 * @param InObject 캐스팅할 const 원시 포인터
 * @return 캐스팅 성공시 const T*, 실패시 nullptr
 */
template <typename T>
const T* Cast(const UObject* InObject)
{
	static_assert(std::is_base_of_v<UObject, T>, "Cast<T>: T는 UObject를 상속받아야 합니다");

	if (!InObject)
	{
		return nullptr;
	}

	// 런타임 타입 체크: InObject가 T 타입인가?
	if (InObject->IsA(T::StaticClass()))
	{
		return static_cast<const T*>(InObject);
	}

	return nullptr;
}

/**
 * @brief 안전한 타입 캐스팅 함수 (TObjectPtr용)
 * @tparam T 캐스팅할 대상 타입
 * @tparam U 소스 타입
 * @param InObjectPtr 캐스팅할 TObjectPtr
 * @return 캐스팅 성공시 TObjectPtr<T>, 실패시 nullptr을 담은 TObjectPtr<T>
 */
template <typename T, typename U>
TObjectPtr<T> Cast(const TObjectPtr<U>& InObjectPtr)
{
	static_assert(std::is_base_of_v<UObject, T>, "Cast<T>: T는 UObject를 상속받아야 합니다");
	static_assert(std::is_base_of_v<UObject, U>, "Cast<T>: U는 UObject를 상속받아야 합니다");

	U* RawPtr = InObjectPtr.Get();
	T* CastedPtr = Cast<T>(RawPtr);

	return TObjectPtr<T>(CastedPtr);
}

/**
 * @brief 빠른 캐스팅 함수 (런타임 체크 없음, 디버그에서만 체크)
 * 성능이 중요한 상황에서 사용, 타입 안전성을 개발자가 보장해야 함
 * @tparam T 캐스팅할 대상 타입
 * @param InObject 캐스팅할 원시 포인터
 * @return T* (런타임 체크 없이 static_cast)
 */
template <typename T>
T* CastChecked(UObject* InObject)
{
	static_assert(std::is_base_of_v<UObject, T>, "CastChecked<T>: T는 UObject를 상속받아야 합니다");

#ifdef _DEBUG
	// 디버그 빌드에서는 안전성 체크
	if (InObject && !InObject->IsA(T::StaticClass()))
	{
		// 로그나 assert 추가 가능
		return nullptr;
	}
#endif

	return static_cast<T*>(InObject);
}

/**
 * @brief 빠른 캐스팅 함수 (TObjectPtr용)
 * @tparam T 캐스팅할 대상 타입
 * @tparam U 소스 타입
 * @param InObjectPtr 캐스팅할 TObjectPtr
 * @return TObjectPtr<T>
 */
template <typename T, typename U>
TObjectPtr<T> CastChecked(const TObjectPtr<U>& InObjectPtr)
{
	static_assert(std::is_base_of_v<UObject, T>, "CastChecked<T>: T는 UObject를 상속받아야 합니다");
	static_assert(std::is_base_of_v<UObject, U>, "CastChecked<T>: U는 UObject를 상속받아야 합니다");

	U* RawPtr = InObjectPtr.Get();
	T* CastedPtr = CastChecked<T>(RawPtr);

	return TObjectPtr<T>(CastedPtr);
}

/**
 * @brief 타입 체크 함수 (캐스팅하지 않고 체크만)
 * @tparam T 체크할 타입
 * @param InObject 체크할 객체
 * @return InObject가 T 타입이면 true
 */
template <typename T>
bool IsA(const UObject* InObject)
{
	static_assert(std::is_base_of_v<UObject, T>, "IsA<T>: T는 UObject를 상속받아야 합니다");

	if (!InObject)
	{
		return false;
	}

	return InObject->IsA(T::StaticClass());
}

/**
 * @brief 타입 체크 함수 (TObjectPtr용)
 * @tparam T 체크할 타입
 * @tparam U 소스 타입
 * @param InObjectPtr 체크할 TObjectPtr
 * @return InObjectPtr가 T 타입이면 true
 */
template <typename T, typename U>
bool IsA(const TObjectPtr<U>& InObjectPtr)
{
	static_assert(std::is_base_of_v<UObject, T>, "IsA<T>: T는 UObject를 상속받아야 합니다");
	static_assert(std::is_base_of_v<UObject, U>, "IsA<T>: U는 UObject를 상속받아야 합니다");

	return IsA<T>(InObjectPtr.Get());
}

/**
 * @brief 유효성과 타입을 동시에 체크하는 함수
 * @tparam T 체크할 타입
 * @param InObject 체크할 객체
 * @return 객체가 유효하고 T 타입이면 true
 */
template <typename T>
bool IsValid(const UObject* InObject)
{
	return InObject && IsA<T>(InObject);
}

/**
 * @brief 유효성과 타입을 동시에 체크하는 함수 (TObjectPtr용)
 * @tparam T 체크할 타입
 * @tparam U 소스 타입
 * @param InObjectPtr 체크할 TObjectPtr
 * @return 객체가 유효하고 T 타입이면 true
 */
template <typename T, typename U>
bool IsValid(const TObjectPtr<U>& InObjectPtr)
{
	return InObjectPtr && IsA<T>(InObjectPtr);
}

TArray<TObjectPtr<UObject>>& GetUObjectArray();
