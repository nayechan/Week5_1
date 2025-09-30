#pragma once

#include <type_traits>

class UObject;
class UClass;

template <typename T>
struct TObjectPtr;

/**
 * @brief 기본적인 스마트 포인터 클래스
 * 언리얼 엔진 코드의 ObjectPtr을 프로젝트 상황에 맞게 로컬라이징한 버전
 */
class ObjectPtr
{
public:
	// 기본 생성자
	ObjectPtr() : Pointer(nullptr)
	{
	}

	// nullptr 생성자
	ObjectPtr(nullptr_t) : Pointer(nullptr)
	{
	}

	// 객체 포인터 생성자
	template <typename T>
	explicit ObjectPtr(T* InObject) : Pointer(static_cast<void*>(InObject))
	{
	}

	// 복사 생성자
	ObjectPtr(const ObjectPtr& InOther) = default;

	// 이동 생성자
	ObjectPtr(ObjectPtr&& InOther) noexcept = default;

	// 복사 대입 연산자
	ObjectPtr& operator=(const ObjectPtr& InOther) = default;

	// 이동 대입 연산자
	ObjectPtr& operator=(ObjectPtr&& InOther) noexcept = default;

	// nullptr 대입 연산자
	ObjectPtr& operator=(nullptr_t)
	{
		Pointer = nullptr;
		return *this;
	}

	// 객체 포인터 대입 연산자
	template <typename T>
	ObjectPtr& operator=(T* InObject)
	{
		Pointer = static_cast<void*>(InObject);
		return *this;
	}

	// 원시 포인터 반환
	template <typename T>
	T* Get() const
	{
		return static_cast<T*>(Pointer);
	}

	// 유효성 검사 연산자
	explicit operator bool() const
	{
		return Pointer != nullptr;
	}

	// 부정 연산자
	bool operator!() const
	{
		return Pointer == nullptr;
	}

	// 동등 비교 연산자
	bool operator==(const ObjectPtr& InOther) const
	{
		return Pointer == InOther.Pointer;
	}

	// 불일치 비교 연산자
	bool operator!=(const ObjectPtr& InOther) const
	{
		return Pointer != InOther.Pointer;
	}

	// nullptr과의 동등 비교
	bool operator==(nullptr_t) const
	{
		return Pointer == nullptr;
	}

	// nullptr과의 불일치 비교
	bool operator!=(nullptr_t) const
	{
		return Pointer != nullptr;
	}

	// 해시 함수를 위한 포인터 반환
	void* GetRawPtr() const { return Pointer; }

private:
	void* Pointer;
};

/**
 * @brief 타입 안정성을 제공하는 template 기반 스마트 포인터
 * 마찬가지로 언리얼 엔진의 TObjectPtr을 로컬라이징한 버전
 */
template <typename T>
struct TObjectPtr
{
	static_assert(!std::is_void_v<T>, "TObjectPtr<T>는 void 타입으로 사용할 수 없습니다");

public:
	using ElementType = T;

	// 기본 생성자
	TObjectPtr() : ObjectPtr()
	{
	}

	// nullptr 생성자
	TObjectPtr(nullptr_t) : ObjectPtr(nullptr)
	{
	}

	// 객체 포인터 생성자
	explicit TObjectPtr(T* InObject) : ObjectPtr(InObject)
	{
	}

	// 다른 타입에서의 변환 생성자 (상속 관계일 때)
	template <typename U,
	          typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
	TObjectPtr(const TObjectPtr<U>& InOther) : ObjectPtr(InOther.ObjectPtr)
	{
	}

	// 복사 생성자
	TObjectPtr(const TObjectPtr& InOther) = default;

	// 이동 생성자
	TObjectPtr(TObjectPtr&& InOther) noexcept = default;

	// 복사 대입 연산자
	TObjectPtr& operator=(const TObjectPtr& InOther) = default;

	// 이동 대입 연산자
	TObjectPtr& operator=(TObjectPtr&& InOther) noexcept = default;

	// nullptr 대입 연산자
	TObjectPtr& operator=(nullptr_t)
	{
		ObjectPtr = nullptr;
		return *this;
	}

	// 객체 포인터 대입 연산자
	TObjectPtr& operator=(T* InObject)
	{
		ObjectPtr = InObject;
		return *this;
	}

	// 다른 타입으로부터의 대입 연산자 (상속 관계일 때)
	template <typename U,
	          typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
	TObjectPtr& operator=(const TObjectPtr<U>& InOther)
	{
		ObjectPtr = InOther.ObjectPtr;
		return *this;
	}

	// 원시 포인터 반환
	T* Get() const
	{
		return ObjectPtr.template Get<T>();
	}

	// 암시적 포인터 변환
	operator T*() const
	{
		return Get();
	}

	// 역참조 연산자
	T* operator->() const
	{
		return Get();
	}

	// 참조 연산자
	T& operator*() const
	{
		return *Get();
	}

	// 유효성 검사 연산자
	explicit operator bool() const
	{
		return ObjectPtr.operator bool();
	}

	// 부정 연산자
	bool operator!() const
	{
		return ObjectPtr.operator!();
	}

	// 동등 비교 연산자
	template <typename U>
	bool operator==(const TObjectPtr<U>& InOther) const
	{
		return ObjectPtr == InOther.ObjectPtr;
	}

	// 불일치 비교 연산자
	template <typename U>
	bool operator!=(const TObjectPtr<U>& InOther) const
	{
		return ObjectPtr != InOther.ObjectPtr;
	}

	// 원시 포인터와의 동등 비교
	bool operator==(T* InOther) const
	{
		return Get() == InOther;
	}

	// 원시 포인터와의 불일치 비교
	bool operator!=(T* InOther) const
	{
		return Get() != InOther;
	}

	// nullptr와의 동등 비교
	bool operator==(nullptr_t) const
	{
		return ObjectPtr == nullptr;
	}

	// nullptr와의 불일치 비교
	bool operator!=(nullptr_t) const
	{
		return ObjectPtr != nullptr;
	}

	// Friend class를 위한 내부 ObjectPtr 접근
	template <typename U>
	friend struct TObjectPtr;
	::ObjectPtr ObjectPtr;
};

// 유틸리티 함수들

// 원시 포인터 반환 함수
template <typename T>
T* ToRawPtr(const TObjectPtr<T>& InPtr)
{
	return InPtr.Get();
}

// 원시 포인터를 그대로 반환하는 오버로드
template <typename T>
T* ToRawPtr(T* InPtr)
{
	return InPtr;
}

// TObjectPtr 생성 함수
template <typename T>
TObjectPtr<T> MakeObjectPtr(T* InObject)
{
	return TObjectPtr<T>(InObject);
}

// 해시 함수 지원
namespace std
{
	template <typename T>
	struct hash<TObjectPtr<T>>
	{
		size_t operator()(const TObjectPtr<T>& Ptr) const
		{
			return hash<void*>{}(Ptr.ObjectPtr.GetRawPtr());
		}
	};

	template <>
	struct hash<ObjectPtr>
	{
		size_t operator()(const ObjectPtr& InPtr) const noexcept
		{
			return hash<void*>{}(InPtr.GetRawPtr());
		}
	};
}

// 전역 비교 연산자들
template <typename T>
bool operator==(T* InLeft, const TObjectPtr<T>& InRight)
{
	return InLeft == InRight.Get();
}

template <typename T>
bool operator!=(T* InLeft, const TObjectPtr<T>& InRight)
{
	return InLeft != InRight.Get();
}

template <typename T>
bool operator==(nullptr_t, const TObjectPtr<T>& InRight)
{
	return InRight == nullptr;
}

template <typename T>
bool operator!=(nullptr_t, const TObjectPtr<T>& InRight)
{
	return InRight != nullptr;
}
