#pragma once
#include "Core/Public/Object.h"

using std::chrono::high_resolution_clock;

UCLASS()
class UTimeManager :
	public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(UTimeManager, UObject)

public:
	void Update();

	// Getter & Setter
	float GetFPS() const { return FPS; }
	float GetDeltaTime() const { return DeltaTime; }
	float GetGameTime() const { return GameTime; }
	bool IsPaused() const { return bIsPaused; }

	void PauseGame() { bIsPaused = true; }
	void ResumeGame() { bIsPaused = false; }

private:
	high_resolution_clock::time_point PrevTime;
	high_resolution_clock::time_point CurrentTime;
	float GameTime;
	float DeltaTime;

	float FPS;
	int FrameCount;
	float FrameSpeedSamples[Time::FPS_SAMPLE_COUNT];
	int FrameSpeedSampleIndex;

	bool bIsPaused;

	void Initialize();
	void CalculateFPS();
};
