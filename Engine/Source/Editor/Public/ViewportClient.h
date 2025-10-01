#pragma once
#include "Editor/Public/Camera.h"

class ULevel;

// 뷰포트의 카메라 모드를 정의하는 열거형
enum class EViewportCameraType : uint8_t
{
	Perspective,
	Ortho_Top,
	Ortho_Bottom,
	Ortho_Front,
	Ortho_Back,
	Ortho_Left,
	Ortho_Right
};

// int → EViewportCameraType 변환
inline EViewportCameraType ToClientCameraType(int InValue)
{
	switch (InValue)
	{
	case static_cast<int>(EViewportCameraType::Perspective):   return EViewportCameraType::Perspective;
	case static_cast<int>(EViewportCameraType::Ortho_Top):     return EViewportCameraType::Ortho_Top;
	case static_cast<int>(EViewportCameraType::Ortho_Bottom):  return EViewportCameraType::Ortho_Bottom;
	case static_cast<int>(EViewportCameraType::Ortho_Front):   return EViewportCameraType::Ortho_Front;
	case static_cast<int>(EViewportCameraType::Ortho_Back):    return EViewportCameraType::Ortho_Back;
	case static_cast<int>(EViewportCameraType::Ortho_Left):    return EViewportCameraType::Ortho_Left;
	case static_cast<int>(EViewportCameraType::Ortho_Right):   return EViewportCameraType::Ortho_Right;

	default:
		UE_LOG_ERROR("[EViewportCameraType] Enum 파싱에 실패했습니다 (기본값 사용)", InValue);
		return EViewportCameraType::Perspective;
	}
}

// EViewportType을 문자열로 변환하는 헬퍼 함수
inline const char* ClientCameraTypeToString(EViewportCameraType InType)
{
	switch (InType)
	{
	case EViewportCameraType::Perspective:   return "Perspective";
	case EViewportCameraType::Ortho_Top:     return "Top";
	case EViewportCameraType::Ortho_Bottom:  return "Bottom";
	case EViewportCameraType::Ortho_Front:   return "Front";
	case EViewportCameraType::Ortho_Back:    return "Back";
	case EViewportCameraType::Ortho_Left:    return "Left";
	case EViewportCameraType::Ortho_Right:   return "Right";
	default:                                 return "Unknown";
	}
}

class FViewportClient
{
public:
	FViewportClient() = default;
	~FViewportClient() = default;

	/* *
	* @brief 출력될 화면의 너비, 높이, 깊이 등을 적용합니다.
	*/
	void Apply(ID3D11DeviceContext* InContext) const;

	/* *
	* @brief 현재는 사용하지 않지만, 추후 사용될 여지가 있음
	*/
	void ClearDepth(ID3D11DeviceContext* InContext, ID3D11DepthStencilView* InStencilView) const;

	/* *
	* @brief 카메라 상태를 업데이트합니다.
	*/
	void SnapCameraToView(const FVector& InFocusPoint);

	bool IsOrthographic() const { return CameraType != EViewportCameraType::Perspective; }

	// Getter
	D3D11_VIEWPORT GetViewportInfo() const { return ViewportInfo; }
	EViewportCameraType GetCameraType() const { return CameraType; }

	// Setter
	void SetViewportInfo(const D3D11_VIEWPORT& InViewport) { ViewportInfo = InViewport; }
	void SetCameraType(EViewportCameraType InViewportCameraType);

	D3D11_VIEWPORT ViewportInfo = {};
	UCamera Camera;
	bool bIsActive = false;
	bool bIsVisible = true;  // 뷰포트 표시 여부 (기본값: 표시)
	EViewportCameraType CameraType = EViewportCameraType::Perspective;

	// PIE System: 이 뷰포트가 렌더링할 World (nullptr이면 Editor World 사용)
	class UWorld* RenderTargetWorld = nullptr;
};

