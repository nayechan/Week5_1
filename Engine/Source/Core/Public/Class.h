#pragma once
#include "Name.h"
#include "ObjectPtr.h"

class UObject;
class UClass;

/**
 * @brief UClass Metadata System
 * Runtime에 컴파일 시에 제거되는 다양한 클래스 정보를 제공하기 위해 만들어진 클래스
 * @param ClassName 클래스 이름
 * @param SuperClass 부모 클래스
 * @param ClassSize 클래스 크기
 * @param Constructor 생성자 함수 포인터
 */
class UClass
{
public:
	// 생성자 함수 포인터 타입 정의
	typedef TObjectPtr<UObject>(*ClassConstructorType)();

	UClass(const FName& InName, TObjectPtr<UClass> InSuperClass, size_t InClassSize, ClassConstructorType InConstructor);

	static void SignUpClass(TObjectPtr<UClass> InClass);
	static TObjectPtr<UClass> FindClass(const FName& InClassName);
	static void PrintAllClasses();
	static void Shutdown();

	bool IsChildOf(TObjectPtr<UClass> InClass) const;
	TObjectPtr<UObject> CreateDefaultObject() const;

	// Getter
	const FName& GetClassTypeName() const { return ClassName; }
	TObjectPtr<UClass> GetSuperClass() const { return SuperClass; }
	size_t GetClassSize() const { return ClassSize; }

private:
	// '최초 사용 시 생성' 기법을 위해 접근자 함수를 제공합니다.
	static TArray<TObjectPtr<UClass>>& GetAllClasses();

	FName ClassName;
	TObjectPtr<UClass> SuperClass;
	size_t ClassSize;
	ClassConstructorType Constructor;
};

/**
 * @brief RTTI 매크로 시스템
 *
 * 언리얼엔진의 UCLASS(), DECLARE_CLASS, IMPLEMENT_CLASS와 유사한 매크로들 정의
 */
 // UCLASS 매크로, 기능은 존재하지 않으며 외관 상의 유사성을 목표로 세팅
#define UCLASS()

// Friend Class 처리를 위한 Generated Body 매크로
#define GENERATED_BODY() \
public: \
    friend class UClass; \
    friend class UObject;

// 클래스 선언부에 사용하는 매크로
#define DECLARE_CLASS(ClassName, SuperClassName) \
public: \
    typedef ClassName ThisClass; \
    typedef SuperClassName Super; \
    static TObjectPtr<UClass> StaticClass(); \
    virtual TObjectPtr<UClass> GetClass() const; \
    static TObjectPtr<UObject> CreateDefaultObject##ClassName(); \
private: \
    static TObjectPtr<UClass> ClassPrivate;

// 클래스 구현부에 사용하는 매크로
#define IMPLEMENT_CLASS(ClassName, SuperClassName) \
    TObjectPtr<UClass> ClassName::ClassPrivate = nullptr; \
    TObjectPtr<UClass> ClassName::StaticClass() \
    { \
        if (!ClassPrivate) \
        { \
            ClassPrivate = TObjectPtr<UClass>(new UClass( \
                FName(#ClassName), \
                SuperClassName::StaticClass(), \
                sizeof(ClassName), \
                &ClassName::CreateDefaultObject##ClassName \
            )); \
            UClass::SignUpClass(ClassPrivate); \
        } \
        return ClassPrivate; \
    } \
    TObjectPtr<UClass> ClassName::GetClass() const \
    { \
        return ClassName::StaticClass(); \
    } \
    TObjectPtr<UObject> ClassName::CreateDefaultObject##ClassName() \
    { \
        return TObjectPtr<UObject>(new ClassName()); \
    } \
    /* 즉시 실행 람다를 사용하여 정적 초기화 시점에 클래스를 자동 등록합니다. */ \
    static bool bIsRegistered_##ClassName = [](){ ClassName::StaticClass(); return true; }();

/**
 * @brief 싱글톤 클래스용 RTTI 매크로 시스템
 *
 * 싱글톤 패턴을 사용하는 클래스에 UClass 시스템을 적용하기 위한 매크로들
 * 기존 DECLARE_CLASS/IMPLEMENT_CLASS와 동일한 RTTI 기능을 제공하면서
 * 싱글톤 패턴의 GetInstance() 방식과 호환되도록 설계
 */

 // 싱글톤 클래스 선언부에 사용하는 매크로
#define DECLARE_SINGLETON_CLASS(ClassName, SuperClassName) \
public: \
    typedef ClassName ThisClass; \
    typedef SuperClassName Super; \
    static TObjectPtr<UClass> StaticClass(); \
    virtual TObjectPtr<UClass> GetClass() const; \
    static TObjectPtr<UObject> CreateDefaultObject##ClassName(); \
    static ClassName& GetInstance(); \
private: \
    static TObjectPtr<UClass> ClassPrivate; \
    ClassName(); \
    virtual ~ClassName(); \
    ClassName(const ClassName&) = delete; \
    ClassName& operator=(const ClassName&) = delete; \
    ClassName(ClassName&&) = delete; \
    ClassName& operator=(ClassName&&) = delete;

// 싱글톤 클래스 구현부에 사용하는 매크로
#define IMPLEMENT_SINGLETON_CLASS(ClassName, SuperClassName) \
    TObjectPtr<UClass> ClassName::ClassPrivate = nullptr; \
    TObjectPtr<UClass> ClassName::StaticClass() \
    { \
        if (!ClassPrivate) \
        { \
            ClassPrivate = TObjectPtr<UClass>(new UClass( \
                FName(#ClassName), \
                SuperClassName::StaticClass(), \
                sizeof(ClassName), \
                nullptr /* 싱글톤은 동적 생성을 지원하지 않으므로 생성자 포인터를 null로 전달 */ \
            )); \
            UClass::SignUpClass(ClassPrivate); \
        } \
        return ClassPrivate; \
    } \
    TObjectPtr<UClass> ClassName::GetClass() const \
    { \
        return ClassName::StaticClass(); \
    } \
    TObjectPtr<UObject> ClassName::CreateDefaultObject##ClassName() \
    { \
        return TObjectPtr<UObject>(&ClassName::GetInstance()); \
    } \
    ClassName& ClassName::GetInstance() \
    { \
        static ClassName Instance; \
        return Instance; \
    } \
    /* 즉시 실행 람다를 사용하여 정적 초기화 시점에 클래스를 자동 등록합니다. */ \
    static bool bIsRegistered_##ClassName = [](){ ClassName::StaticClass(); return true; }();

// 싱글톤 베이스 클래스용 매크로 (SuperClass가 nullptr인 경우)
#define IMPLEMENT_SINGLETON_CLASS_BASE(ClassName) \
    TObjectPtr<UClass> ClassName::ClassPrivate = nullptr; \
    TObjectPtr<UClass> ClassName::StaticClass() \
    { \
        if (!ClassPrivate) \
        { \
            ClassPrivate = TObjectPtr<UClass>(new UClass( \
                FName(#ClassName), \
                nullptr, \
                sizeof(ClassName), \
                nullptr \
            )); \
            UClass::SignUpClass(ClassPrivate); \
        } \
        return ClassPrivate; \
    } \
    TObjectPtr<UClass> ClassName::GetClass() const \
    { \
        return ClassName::StaticClass(); \
    } \
    TObjectPtr<UObject> ClassName::CreateDefaultObject##ClassName() \
    { \
        return TObjectPtr<UObject>(&ClassName::GetInstance()); \
    } \
    ClassName& ClassName::GetInstance() \
    { \
        static ClassName Instance; \
        return Instance; \
    } \
    /* 즉시 실행 람다를 사용하여 정적 초기화 시점에 클래스를 자동 등록합니다. */ \
    static bool bIsRegistered_##ClassName = [](){ ClassName::StaticClass(); return true; }();

// UObject의 기본 매크로 (Base Class)
#define IMPLEMENT_CLASS_BASE(ClassName) \
    TObjectPtr<UClass> ClassName::ClassPrivate = nullptr; \
    TObjectPtr<UClass> ClassName::StaticClass() \
    { \
        if (!ClassPrivate) \
        { \
            ClassPrivate = TObjectPtr<UClass>(new UClass( \
                FName(#ClassName), \
                nullptr, \
                sizeof(ClassName), \
                &ClassName::CreateDefaultObject##ClassName \
            )); \
            UClass::SignUpClass(ClassPrivate); \
        } \
        return ClassPrivate; \
    } \
    TObjectPtr<UClass> ClassName::GetClass() const \
    { \
        return ClassName::StaticClass(); \
    } \
    TObjectPtr<UObject> ClassName::CreateDefaultObject##ClassName() \
    { \
        return TObjectPtr<UObject>(new ClassName()); \
    } \
    /* 즉시 실행 람다를 사용하여 정적 초기화 시점에 클래스를 자동 등록합니다. */ \
    static bool bIsRegistered_##ClassName = [](){ ClassName::StaticClass(); return true; }();
