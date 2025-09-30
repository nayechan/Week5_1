#pragma once
#include "UIWindow.h"

class AActor;

/**
 * @brief Scene 관리를 위한 Window
 * SceneHierarchyWidget을 포함하여 현재 Level의 Actor들을 관리할 수 있는 UI 제공
 */
class UDetailWindow : public UUIWindow
{
public:
	UDetailWindow();
	virtual ~UDetailWindow() override {}

	void Initialize() override;

	void OnSelectedActorChanged(AActor* InActor) override;
};
