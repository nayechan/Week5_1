#pragma once
#include "Core/Public/Object.h"
#include "Editor/Public/Editor.h"

/**
 * @brief UI의 기능 단위인 위젯 클래스의 Interface Class
 * 위젯이라면 필수적인 공통 인터페이스와 기능을 제공
 */
UCLASS()
class UWidget : public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UWidget, UObject)

public:
	// Essential Role
	// 필요하지 않은 기능이 있을 수 있으나 구현 시 반드시 고려하라는 의미의 순수 가상 함수 처리
	virtual void Initialize() {};
	virtual void Update() {};
	virtual void RenderWidget() {};

	// 후처리는 취사 선택
	virtual void PostProcess() {}

	// Singleton 판별 함수 (default는 false)
	virtual bool IsSingleton() const { return false; }

	// Special Member Function
	UWidget() = default;
	UWidget(const FString& InName);
	~UWidget() override = default;
};
