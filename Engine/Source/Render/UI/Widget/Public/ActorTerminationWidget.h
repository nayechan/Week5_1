#pragma once
#include "Widget.h"

/**
 * @brief 해당 Widget은 ImGui 기능을 제공하지 않지만 Termination 유틸을 제공한다
 */
class UActorTerminationWidget
	: public UWidget
{
public:
	void Initialize() override;
	void Update() override;
	void RenderWidget() override;
	void DeleteSelectedActor();

	UActorTerminationWidget();
	~UActorTerminationWidget() override;

private:
	TObjectPtr<AActor> SelectedActor;
};
