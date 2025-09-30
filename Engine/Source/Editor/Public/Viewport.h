#pragma once
#include "Editor/Public/ViewportClient.h"

class URenderer;

class FViewport
{
public:
	FViewport() = default;
	~FViewport();

	/**
	 * @brief 현재 모든 뷰포트의 카메라 상태를 UConfigManager에 동기화하는 새로운 함수
	 */
	void UpdateCameraSettingsToConfig();

	/**
	* @brief ViewportClient가 보유한 Viewport들의 정보를 갱신합니다.
	* 이 함수는 현재 2x2 기준으로 작성되어 있습니다.
	*/
	void InitializeLayout(const D3D11_VIEWPORT& InViewport);

	/**
	* @brief 모든 뷰포트의 카메라 데이터를 ConfigManager로부터 가져와 적용합니다.
	*/
	void ApplyAllCameraDataToViewportClients();

	/**
	* @brief 현재 활성화된 FViewport를 갱신합니다.
	*/
	void UpdateActiveViewportClient(const FVector& InMousePosition);

	/**
	* @brief 모든 뷰포트의 카메라를 업데이트합니다.
	*/
	void UpdateAllViewportClientCameras();

	/**
	 * @brief 직교 카메라의 이동량을 받아 포커스 포인트를 갱신하고,
	 * 활성 뷰포트를 제외한 나머지 직교 카메라들을 업데이트합니다.
	 */
	void UpdateOrthoFocusPointByDelta(const FVector& InDelta);

	bool IsSingleViewportMode() const;
	void SetSingleViewportMode(int InActiveIndex);
	void SetMultiViewportMode();

	/**
	* @brief 현재 활성화된 뷰포트를 반환합니다.
	* @return 활성 뷰포트의 포인터. 없으면 nullptr.
	*/
	FViewportClient* GetActiveViewportClient() const { return ActiveViewportClient; }

	/**
	* @brief 활성 뷰포트의 카메라를 반환하는 헬퍼 함수.
	* @return 활성 카메라의 포인터. 없으면 nullptr.
	*/
	UCamera* GetActiveCamera() const { return ActiveViewportClient ? &ActiveViewportClient->Camera : nullptr; }

	TArray<FViewportClient>& GetViewports() { return ViewportClients; }

	void SetFocusPoint(const FVector& NewFocusPoint);

private:
	TArray<FViewportClient> ViewportClients = {};
	FViewportClient* ActiveViewportClient = nullptr;
	FVector FocusPoint = { 0.0f, 0.0f, 0.0f }; 	// 직교 투영 카메라가 공유하는 좌표
	bool bIsSingleMode = false;  // 현재 싱글 뷰포트 모드인지
	int SingleModeActiveIndex = 0;  // 싱글 모드시 활성 뷰포트 인덱스
};

