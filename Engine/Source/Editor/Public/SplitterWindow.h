#pragma once
#include "Global/CoreTypes.h"

/* *
* @brief 스플리터의 접힘 상태를 나타내는 Enum 타입입니다.
*/
enum class ECollapseState
{
	Normal,	// 기본 상태
	SideLT, // 좌측 혹은 위에서 접힌 상태
	SideRB  // 우측 혹은 아래에서 접힌 상태
};

/**
 * @brief 레이아웃 시스템의 가장 기본적인 단위. 화면의 사각 영역을 나타냅니다.
 */
class SWindow
{
public:
	SWindow() = default;
	virtual ~SWindow() {}

	virtual void Resize(const FRect& InRect) { Rect = InRect; }

	// 마우스의 좌표가 현재 윈도우 영역 내에 있는지 판별합니다.
	bool IsHovered(const FPoint& InMousePosition) const;

	FRect Rect{};
};

/**
 * @brief 두 개의 자식 SWindow를 관리하는 스플리터의 기본 클래스입니다.
 */
class SSplitter : public SWindow
{
public:
	SSplitter() = default;
	virtual ~SSplitter() = default;

	// 자식 윈도우를 설정합니다.
	void SetChildren(SWindow* InSideLT, SWindow* InSideRB);

	// Getter
	float GetRatio() const { return Ratio; }

	// Setter
	void SetRatio(const float InRatio);

public:
	SWindow* SideLT = nullptr; // Left 혹은 Top을 맡는 윈도우
	SWindow* SideRB = nullptr; // Right 혹은 Bottom을 맡는 윈도우
	float Ratio = 0.5f;		   // 부모 윈도우 영역 대비 위치 비율
	const float Thickness = 10.0f;    // 드래그 판정이 가능한 범위
	const float CollapseThreshold = 0.03f; // 스플리터가 화면 비율을 감지하는 기준
	ECollapseState CollapseState = ECollapseState::Normal; // 스플리터 상태
};

/**
 * @brief 화면을 수평(위, 아래)으로 분할하는 스플리터입니다.
 */
class SSplitterH : public SSplitter
{
public:
	void Resize(const FRect& ParentRect) override;
};

/**
 * @brief 화면을 수직(좌, 우)으로 분할하는 스플리터입니다.
 */
class SSplitterV : public SSplitter
{
public:
	void Resize(const FRect& ParentRect) override;
};
