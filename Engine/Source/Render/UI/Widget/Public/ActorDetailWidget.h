#pragma once
#include "Widget.h"

class AActor;
class UActorComponent;

/**
 * @brief 선택된 Actor의 이름과 컴포넌트 트리를 표시하는 Widget
 * Rename 기능이 추가되어 있음
 */
class UActorDetailWidget :
	public UWidget
{
public:
	void Initialize() override;
	void Update() override;
	void RenderWidget() override;

	// Special Member Function
	UActorDetailWidget();
	~UActorDetailWidget() override;

private:
	bool bIsRenamingActor = false;
	char ActorNameBuffer[256] = {};

	// Helper functions
	void RenderActorHeader(TObjectPtr<AActor> InSelectedActor);
	void RenderComponentTree(AActor* InSelectedActor);
	void RenderComponentNode(USceneComponent* InComponent);
	void RenderNonSceneComponentNode(UActorComponent* InComponent);

	// 이름 변경 함수
	void StartRenamingActor(TObjectPtr<AActor> InActor);
	void FinishRenamingActor(TObjectPtr<AActor> InActor);
	void CancelRenamingActor();

	TObjectPtr<UActorComponent> SelectedComponent;
	TObjectPtr<USceneComponent> NodeToOpenNextFrame = nullptr;

	// 컴포넌트 클래스 목록을 캐싱
	static TArray<TObjectPtr<UClass>> CreatableComponentClasses;
};
