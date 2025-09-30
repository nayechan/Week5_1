#pragma once
#include "Core/Public/Object.h"
#include "Core/Public/ObjectPtr.h"

/**
 * @brief Base Factory Class
 * Editor, Runtime 에서 객체의 생성을 담당한다
 */
UCLASS()
class UFactory :
	public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UFactory, UObject)

public:
	// 생성 함수
	virtual TObjectPtr<UObject> CreateNew() { return nullptr; }
	virtual TObjectPtr<UObject> FactoryCreateNew(TObjectPtr<UClass> InClass, TObjectPtr<UObject> InParent, const FName& InName,
	                                  uint32 InFlags = 0, TObjectPtr<UObject> InContext = nullptr, void* InWarning = nullptr);

	// Factory가 지원하는 클래스인지 확인하기 위한 함수
	virtual TObjectPtr<UClass> GetSupportedClass() const;
	virtual bool DoesSupportClass(UClass* InClass);

	// 생성을 처리하는 과정에서 해당 객체를 생성해 줄 Factory를 찾기 위한 함수
	static TArray<TObjectPtr<UFactory>>& GetFactoryList();
	static void RegisterFactory(TObjectPtr<UFactory> InFactory);
	static TObjectPtr<UFactory> FindFactory(TObjectPtr<UClass> InClass);

	// Getter
	const FString& GetDescription() const { return Description; }
	virtual bool IsActorFactory() const { return false; }

	// Special member function
	UFactory();
	~UFactory() override = default;

protected:
	TObjectPtr<UClass> SupportedClass;
	FString Description;
};
