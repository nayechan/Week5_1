#include "pch.h"
#include "Editor/Public/Viewport.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Manager/Config/Public/ConfigManager.h"

FViewport::~FViewport()
{
	// 소멸 시에도 최신 상태를 저장하도록 함수 호출
	UpdateCameraSettingsToConfig();
}

// brief 현재 모든 뷰포트의 카메라 상태를 UConfigManager에 동기화하는 새로운 함수
void FViewport::UpdateCameraSettingsToConfig()
{
	auto& ConfigManager = UConfigManager::GetInstance();

	// 현재 이 코드는 Week04에 작성된 코드입니다.
	//for (int32 Index = 0; Index < ViewportClients.size(); ++Index)
	//{
	//	FViewportCameraData Data;
	//	FViewportClient& Viewport = ViewportClients[Index];
	//	UCamera& Camera = Viewport.Camera;

	//	// 현재 뷰포트와 카메라의 상태를 FViewportCameraData 구조체에 담습니다.
	//	Data.ViewportCameraType = Viewport.GetCameraType();
	//	Data.Location = Camera.GetLocation();
	//	Data.Rotation = Camera.GetRotation();
	//	Data.FovY = Camera.GetFovY();
	//	Data.NearClip = Camera.GetNearZ();
	//	Data.FarClip = Camera.GetFarZ();
	//	Data.OrthoWidth = Camera.GetOrthoWidth();
	//	Data.FocusLocation = this->FocusPoint;

	//	// 완성된 데이터를 ConfigManager에 전달합니다.
	//	ConfigManager.SetViewportCameraData(Index, Data);
	//}


	for (int32 Index = 0; Index < ViewportClients.size(); ++Index)
	{		FViewportCameraData Data;
		FViewportClient& Viewport = ViewportClients[Index];
		UCamera& Camera = Viewport.Camera;

		// 현재 뷰포트와 카메라의 상태를 FViewportCameraData 구조체에 담습니다.
		Data.ViewportCameraType = Viewport.GetCameraType();
		Data.Location = Camera.GetLocation();
		Data.Rotation = Camera.GetRotation();
		Data.FovY = Camera.GetFovY();
		Data.NearClip = Camera.GetNearZ();
		Data.FarClip = Camera.GetFarZ();

		// 완성된 데이터를 ConfigManager에 전달합니다.
		ConfigManager.SetViewportCameraData(Index, Data);
	}
}

void FViewport::InitializeLayout(const D3D11_VIEWPORT& InViewport)
{
	if (ViewportClients.size() < 4) { ViewportClients.resize(4); }
	const float BaseX = InViewport.TopLeftX;
	const float BaseY = InViewport.TopLeftY;
	const float HalfW = InViewport.Width * 0.5f;
	const float HalfH = InViewport.Height * 0.5f;

	ViewportClients[0].SetViewportInfo({ BaseX + 0.0f,      BaseY + 0.0f,      HalfW, HalfH, 0.0f, 1.0f });
	ViewportClients[1].SetViewportInfo({ BaseX + HalfW,     BaseY + 0.0f,      HalfW, HalfH, 0.0f, 1.0f });
	ViewportClients[2].SetViewportInfo({ BaseX + 0.0f,      BaseY + HalfH,     HalfW, HalfH, 0.0f, 1.0f });
	ViewportClients[3].SetViewportInfo({ BaseX + HalfW,     BaseY + HalfH,     HalfW, HalfH, 0.0f, 1.0f });

	// 모든 직교 카메라가 새로운 FocusPoint를 바라보도록 위치를 즉시 갱신합니다.
	UpdateAllViewportClientCameras();
}

// 모든 뷰포트 설정을 순회하며 적용하는 함수
void FViewport::ApplyAllCameraDataToViewportClients()
{
	auto& ConfigManager = UConfigManager::GetInstance();

	// 현재 주석처리된 해당 코드는 Week04에 사용되는 코드입니다.
	//for (int32 Index = 0; Index < ViewportClients.size(); ++Index)
	//{
	//	FViewportClient& TargetViewport = ViewportClients[Index];
	//	UCamera& TargetCamera = TargetViewport.Camera;

	//	// ConfigManager로부터 저장된 뷰포트 데이터를 가져옵니다.
	//	const FViewportCameraData& CamData = ConfigManager.GetViewportCameraData(Index);

	//	// 뷰포트의 카메라 타입을 먼저 설정합니다.
	//	TargetViewport.SetCameraType(CamData.ViewportCameraType);

	//	if (CamData.ViewportCameraType == EViewportCameraType::Perspective)
	//	{
	//		TargetCamera.SetLocation(CamData.Location);
	//		TargetCamera.SetRotation(CamData.Rotation);
	//		TargetCamera.SetFarZ(CamData.FarClip);
	//		TargetCamera.SetNearZ(CamData.NearClip);
	//		TargetCamera.SetFovY(CamData.FovY);
	//	}
	//	else // Orthographic
	//	{
	//		TargetCamera.SetLocation(CamData.Location);
	//		TargetCamera.SetRotation(CamData.Rotation);
	//		TargetCamera.SetFarZ(CamData.FarClip);
	//		TargetCamera.SetNearZ(CamData.NearClip);
	//		TargetCamera.SetOrthoWidth(CamData.OrthoWidth);
	//		FocusPoint = CamData.FocusLocation;
	//	}
	//}

	// 해당 코드는 Week05 전용 코드입니다.
	for (int32 Index = 0; Index < ViewportClients.size(); ++Index)
	{
		FViewportClient& TargetViewport = ViewportClients[Index];
		UCamera& TargetCamera = TargetViewport.Camera;

		// ConfigManager로부터 저장된 뷰포트 데이터를 가져옵니다.
		const FViewportCameraData& CamData = ConfigManager.GetViewportCameraData();

		// 뷰포트의 카메라 타입을 먼저 설정합니다.
		TargetViewport.SetCameraType(CamData.ViewportCameraType);

		if (CamData.ViewportCameraType == EViewportCameraType::Perspective)
		{
			TargetCamera.SetLocation(CamData.Location);
			TargetCamera.SetRotation(CamData.Rotation);
			TargetCamera.SetFarZ(CamData.FarClip);
			TargetCamera.SetNearZ(CamData.NearClip);
			TargetCamera.SetFovY(CamData.FovY);
		}
		else // Orthographic
		{
			TargetCamera.SetLocation(CamData.Location);
			TargetCamera.SetRotation(CamData.Rotation);
			TargetCamera.SetFarZ(CamData.FarClip);
			TargetCamera.SetNearZ(CamData.NearClip);
		}
	}
}

void FViewport::UpdateActiveViewportClient(const FVector& InMousePosition)
{
	ActiveViewportClient = nullptr;
	for (auto& Viewport : ViewportClients)
	{
		Viewport.bIsActive = false;
		const D3D11_VIEWPORT& ViewportInfo = Viewport.GetViewportInfo();

		// 마우스가 현재 뷰포트의 사각 영역 내에 있는지 확인합니다.
		if (InMousePosition.X >= ViewportInfo.TopLeftX && InMousePosition.X <= (ViewportInfo.TopLeftX + ViewportInfo.Width) &&
			InMousePosition.Y >= ViewportInfo.TopLeftY && InMousePosition.Y <= (ViewportInfo.TopLeftY + ViewportInfo.Height))
		{
			Viewport.bIsActive = true;
			ActiveViewportClient = &Viewport;
		}
	}
}

void FViewport::UpdateAllViewportClientCameras()
{
	for (FViewportClient& Viewport : ViewportClients)
	{
		Viewport.SnapCameraToView(FocusPoint);
	}
}

void FViewport::UpdateOrthoFocusPointByDelta(const FVector& InDelta)
{
	FocusPoint += InDelta;
	UpdateAllViewportClientCameras();
}

void FViewport::SetFocusPoint(const FVector& NewFocusPoint)
{
	FocusPoint = NewFocusPoint;
	UpdateAllViewportClientCameras();
}

bool FViewport::IsSingleViewportMode() const
{
	return bIsSingleMode;
}

void FViewport::SetSingleViewportMode(int InActiveIndex)
{
	if (InActiveIndex < 0 || InActiveIndex >= ViewportClients.size()) return;

	bIsSingleMode = true;
	SingleModeActiveIndex = InActiveIndex;

	// 지정된 뷰포트만 표시, 나머지는 숨김
	for (int i = 0; i < ViewportClients.size(); ++i)
	{
		ViewportClients[i].bIsVisible = (i == InActiveIndex);
	}
}

void FViewport::SetMultiViewportMode()
{
	bIsSingleMode = false;

	// 모든 뷰포트 표시
	for (auto& ViewportClient : ViewportClients)
	{
		ViewportClient.bIsVisible = true;
	}
}
