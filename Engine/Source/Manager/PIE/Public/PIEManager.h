#pragma once
#include "Core/Public/Object.h"
#include "Core/Public/ObjectPtr.h"

class UWorld;
class FViewportClient;

/**
 * @brief PIE (Play In Editor) 세션을 관리하는 Manager
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
	 * @brief PIE World Update (PIE가 실행 중일 때만)
	 */
	void Update(float DeltaTime);

	/**
	 * @brief PIE 일시정지/재개
	 */
	void TogglePausePIE();

	/**
	 * @brief PIE가 실행 중인지 확인
	 */
	bool IsPIERunning() const { return PIEWorld != nullptr; }

	/**
	 * @brief PIE가 일시정지 상태인지 확인
	 */
	bool IsPIEPaused() const { return bIsPaused; }

	/**
	 * @brief PIE World 가져오기
	 */
	UWorld* GetPIEWorld() const { return PIEWorld.Get(); }

	/**
	 * @brief PIE가 실행 중인 Viewport 가져오기
	 */
	FViewportClient* GetPIEViewport() const { return PIEViewport; }

private:
	TObjectPtr<UWorld> PIEWorld = nullptr;          // PIE World (PIEManager가 소유)
	FViewportClient* PIEViewport = nullptr;         // PIE가 실행 중인 Viewport
	bool bIsPaused = false;                         // PIE 일시정지 상태
};
