#include "pch.h"
#include "Manager/Time/Public/TimeManager.h"

IMPLEMENT_SINGLETON_CLASS_BASE(UTimeManager)

UTimeManager::UTimeManager()
{
	Initialize();
}

UTimeManager::~UTimeManager() = default;

void UTimeManager::Initialize()
{
	CurrentTime = high_resolution_clock::now();
	PrevTime = CurrentTime;
	GameTime = 0.0f;

	DeltaTime = 0.0f;
	FPS = 0.0f;
	FrameSpeedSampleIndex = 0;

	bIsPaused = false;

	for (int i = 0; i < Time::FPS_SAMPLE_COUNT; ++i)
	{
		FrameSpeedSamples[i] = 0.0f;
	}
}

void UTimeManager::Update()
{
	// 현재 시간 업데이트
	PrevTime = CurrentTime;
	CurrentTime = high_resolution_clock::now();

	// DeltaTime 계산 (초 단위)
	auto Duration = std::chrono::duration_cast<std::chrono::microseconds>(CurrentTime - PrevTime);
	DeltaTime = Duration.count() / 1000000.0f;

	if (!bIsPaused)
	{
		GameTime += DeltaTime;
	}

	CalculateFPS();
	++FrameCount;
}

void UTimeManager::CalculateFPS()
{
	if (DeltaTime > 0.0f)
	{
		// Frame time을 millisecond 단위로 저장
		FrameSpeedSamples[FrameSpeedSampleIndex] = DeltaTime * 1000.0f;
		FrameSpeedSampleIndex = (FrameSpeedSampleIndex + 1) % Time::FPS_SAMPLE_COUNT;
	}

	float FrameTimeSum = 0.0f;
	float WeightSum = 0.0f;
	int ValidSampleCount = 0;

	for (int i = 0; i < Time::FPS_SAMPLE_COUNT; ++i)
	{
		if (FrameSpeedSamples[i] > 0.0f)
		{
			// 가벼운 가중치: 최근 샘플일수록 높은 가중치 (1.0 ~ 0.5)
			int SampleAge = (FrameSpeedSampleIndex - i + Time::FPS_SAMPLE_COUNT) % Time::FPS_SAMPLE_COUNT;
			float Weight = 1.0f - (SampleAge * 0.5f / Time::FPS_SAMPLE_COUNT);

			FrameTimeSum += FrameSpeedSamples[i] * Weight;
			WeightSum += Weight;
			++ValidSampleCount;
		}
	}

	if (ValidSampleCount > 0 && FrameTimeSum > 0.0f)
	{
		float AverageFrameTime = FrameTimeSum / WeightSum;
		FPS = 1000.0f / AverageFrameTime;  // ms → FPS 변환
	}
}
