#pragma once
#include "Core/Public/Object.h"

class ULevel;
class FViewportClient;

/**
 * @brief PIE (Play In Editor) 세션을 관리하는 Manager
 * TODO: World 구조로 변경 시 World 기반으로 리팩토링
 */
UCLASS()
class UPIEManager : public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(UPIEManager, UObject)

public:
	/**
	 * @brief PIE 세션 시작
	 * @param InPIEViewport PIE를 실행할 Viewport
	 */
	void StartPIE(FViewportClient* InPIEViewport);

	/**
	 * @brief PIE 세션 종료
	 */
	void StopPIE();

	/**
	 * @brief PIE 일시정지/재개
	 */
	void TogglePausePIE();

	/**
	 * @brief PIE가 실행 중인지 확인
	 */
	bool IsPIERunning() const { return PIELevel != nullptr; }

	/**
	 * @brief PIE가 일시정지 상태인지 확인
	 */
	bool IsPIEPaused() const { return bIsPaused; }

	/**
	 * @brief PIE Level 가져오기
	 */
	ULevel* GetPIELevel() const { return PIELevel.get(); }

	/**
	 * @brief PIE가 실행 중인 Viewport 가져오기
	 */
	FViewportClient* GetPIEViewport() const { return PIEViewport; }

private:
	TUniquePtr<ULevel> PIELevel = nullptr;          // PIE Level (PIEManager가 소유)
	FViewportClient* PIEViewport = nullptr;         // PIE가 실행 중인 Viewport
	bool bIsPaused = false;                         // PIE 일시정지 상태
};
