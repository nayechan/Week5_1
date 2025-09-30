#include "pch.h"
#include "Editor/Public/ViewportClient.h"

void FViewportClient::Apply(ID3D11DeviceContext* InContext) const
{
	InContext->RSSetViewports(1, &ViewportInfo);
}

void FViewportClient::ClearDepth(ID3D11DeviceContext* InContext, ID3D11DepthStencilView* InStencilView) const
{
	InContext->ClearDepthStencilView(InStencilView, D3D11_CLEAR_DEPTH, 1.f, 0);
}

void FViewportClient::SetCameraType(EViewportCameraType InViewportCameraType)
{
	CameraType = InViewportCameraType;

	if (CameraType == EViewportCameraType::Perspective)
	{
		Camera.SetCameraType(ECameraType::ECT_Perspective);
	}
	else
	{
		Camera.SetCameraType(ECameraType::ECT_Orthographic);
	}
}

void FViewportClient::SnapCameraToView(const FVector& InFocusPoint)
{
	const float Distance = 50.0f; // 초점으로부터의 기본 거리

	switch (CameraType)
	{
	case EViewportCameraType::Perspective:
		// 원근 카메라는 자유롭게 움직여야 하므로 위치를 고정하지 않습니다.
		break;
	case EViewportCameraType::Ortho_Top:
		Camera.SetLocation(InFocusPoint + FVector::UpVector() * Distance);
		Camera.SetRotation(FVector::YAxisVector() * 90.f); // 정수리에서 아래를 보도록 회전
		break;
	case EViewportCameraType::Ortho_Bottom:
		Camera.SetLocation(InFocusPoint + FVector::DownVector() * Distance);
		Camera.SetRotation(FVector::YAxisVector() * -90.f); // 발밑에서 위를 보도록 회전
		break;
	case EViewportCameraType::Ortho_Front:
		Camera.SetLocation(InFocusPoint + FVector::ForwardVector() * Distance);
		Camera.SetRotation(FVector::ZAxisVector() * 180.0f); // 정면에서 뒤를 보도록 회전
		break;
	case EViewportCameraType::Ortho_Back:
		Camera.SetLocation(InFocusPoint + FVector::BackwardVector() * Distance);
		Camera.SetRotation(FVector::ZAxisVector() * 0.f); // 뒤에서 앞을 보도록 회전
		break;
	case EViewportCameraType::Ortho_Left:
		Camera.SetLocation(InFocusPoint + FVector::LeftVector() * Distance);
		Camera.SetRotation(FVector::ZAxisVector() * 90.0f); // 왼쪽에서 오른쪽을 보도록 회전
		break;
	case EViewportCameraType::Ortho_Right:
		Camera.SetLocation(InFocusPoint + FVector::RightVector() * Distance);
		Camera.SetRotation(FVector::ZAxisVector() * -90.0f); // 오른쪽에서 왼쪽을 보도록 회전
	}
}
