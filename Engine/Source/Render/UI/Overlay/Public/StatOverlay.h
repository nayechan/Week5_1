#pragma once
#include "Core/Public/Object.h"
#include <d2d1.h>
#include <dwrite.h>

enum class EStatType : uint8
{
	None = 0,
	FPS = 1 << 0,       // 1
	Memory = 1 << 1,    // 2
	Picking = 1 << 2,   // 3
	BVH = 1 << 3,       // 4
	Culling = 1 << 4,	// 5
	All = FPS | Memory | Picking | BVH | Culling
};

UCLASS()
class UStatOverlay : public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(UStatOverlay, UObject)

public:
	void Initialize();
	void Release();
	void Render();
	void CreateRenderTarget();

	void PreResize();
	void OnResize();

	// Stat control methods
	void ShowFPS(bool bShow) { bShow ? EnableStat(EStatType::FPS) : DisableStat(EStatType::FPS); }
	void ShowMemory(bool bShow) { bShow ? EnableStat(EStatType::Memory) : DisableStat(EStatType::Memory); }
	void ShowPicking(bool bShow) { bShow ? EnableStat(EStatType::Picking) : DisableStat(EStatType::Picking); }
	void ShowBVH(bool bShow) { bShow ? EnableStat(EStatType::BVH) : DisableStat(EStatType::BVH); }
	void ShowAll(bool bShow) { SetStatType(bShow ? EStatType::All : EStatType::None); }
	void ShowCulling(bool bShow) { bShow ? EnableStat(EStatType::Culling) : DisableStat(EStatType::Culling); }

private:
	void RenderFPS();
	void RenderMemory();
	void RenderPicking();
	void RenderBVH();
	void RenderText(const FString& Text, float X, float Y, float R, float G, float B);
	void RenderCulling();

	// FPS Stats
	float CurrentFPS = 0.0f;
	float FrameTime = 0.0f;

	// Rendering position
	float OverlayX = 18.0f;
	float OverlayY = 55.0f;

	uint8 StatMask = static_cast<uint8>(EStatType::None);

	// Helper methods
	std::wstring ToWString(const FString& InStr);
	void EnableStat(EStatType InStatType);
	void DisableStat(EStatType InStatType);
	void SetStatType(EStatType InStatType);
	bool IsStatEnabled(EStatType InStatType) const;

	ID2D1RenderTarget* D2DRenderTarget = nullptr;
	ID2D1SolidColorBrush* TextBrush = nullptr;
	IDWriteTextFormat* TextFormat = nullptr;
	
	ID2D1Factory* D2DFactory = nullptr;
	IDWriteFactory* DWriteFactory = nullptr;
};
