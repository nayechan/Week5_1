#include "pch.h"
#include "Render/UI/Widget/Public/CameraControlWidget.h"

#include "Editor/Public/Camera.h"

// Camera Mode
static const char* CameraMode[] = {
	"Perspective",
	"Orthographic"
};

UCameraControlWidget::UCameraControlWidget()
	: UWidget("Camera Control Widget")
{
	Cameras.resize(4);
}

// 카메라를 소유가 아닌 참조만 하기에 따로 소멸 해제를 책임지지 않습니다.
UCameraControlWidget::~UCameraControlWidget() = default;

void UCameraControlWidget::Initialize()
{
}

void UCameraControlWidget::Update()
{
}

void UCameraControlWidget::RenderWidget()
{
	for (int Index = 0; Index < Cameras.size(); ++Index)
	{
		UCamera* Camera = Cameras[Index];
		if (!Camera) continue;

		// ImGui의 ID 스택을 사용하여 각 카메라 컨트롤을 고유하게 만듭니다.
		ImGui::PushID(Index);

		// 뷰포트 이름을 표시 (예: "Viewport 0: Perspective")
		FString ViewportTitle = FString("Camera ") + std::to_string(Index).c_str();
		if (ImGui::CollapsingHeader(ViewportTitle.c_str()))
		{
			// --- UI를 그리기 직전에 항상 카메라로부터 값을 가져옵니다 ---
			auto& Location = Camera->GetLocation();
			auto& Rotation = Camera->GetRotation();
			float FovY = Camera->GetFovY();
			float NearZ = Camera->GetNearZ();
			float FarZ = Camera->GetFarZ();
			float OrthoWidth = Camera->GetOrthoWidth();
			float MoveSpeed = Camera->GetMoveSpeed();
			int ModeIndex = (Camera->GetCameraType() == ECameraType::ECT_Perspective) ? 0 : 1;

			// --- UI 렌더링 및 상호작용 ---
			if (ImGui::SliderFloat("Move Speed", &MoveSpeed, UCamera::MIN_SPEED, UCamera::MAX_SPEED, "%.1f"))
			{
				Camera->SetMoveSpeed(MoveSpeed); // 변경 시 즉시 적용
			}

			if (ImGui::Combo("Mode", &ModeIndex, CameraMode, IM_ARRAYSIZE(CameraMode)))
			{
				Camera->SetCameraType(ModeIndex == 0 ? ECameraType::ECT_Perspective : ECameraType::ECT_Orthographic);
			}

			ImGui::DragFloat3("Location", &Location.X, 0.05f);
			ImGui::DragFloat3("Rotation", &Rotation.X, 0.1f);

			bool bOpticsChanged = false;
			if (ModeIndex == 0) // 원근 투영 
			{
				bOpticsChanged |= ImGui::SliderFloat("FOV", &FovY, 1.0f, 170.0f, "%.1f");
				bOpticsChanged |= ImGui::DragFloat("Z Near", &NearZ, 0.01f, 0.0001f, 1e6f, "%.4f");
				bOpticsChanged |= ImGui::DragFloat("Z Far", &FarZ, 0.1f, 0.001f, 1e7f, "%.3f");
			}
			else if (ModeIndex == 1) // 직교 투영
			{
				bOpticsChanged |= ImGui::SliderFloat("OrthoWidth", &OrthoWidth, 1.0f, 150.0f, "%.1f");
			}

			// 변경된 값을 카메라에 다시 적용
			if (bOpticsChanged)
			{
				Camera->SetFovY(FovY);
				Camera->SetNearZ(NearZ);
				Camera->SetFarZ(FarZ);
				Camera->SetOrthoWidth(OrthoWidth);
			}
		}

		ImGui::PopID();
	}
}

void UCameraControlWidget::SyncFromCamera(UCamera* InCamera)
{
	if (!InCamera) { return; }

	CameraModeIndex = (InCamera->GetCameraType() == ECameraType::ECT_Perspective) ? 0 : 1;
	UiFovY = InCamera->GetFovY();
	UiNearZ = InCamera->GetNearZ();
	UiFarZ = InCamera->GetFarZ();
}

void UCameraControlWidget::PushToCamera(UCamera* InCamera)
{
	if (!InCamera) { return; }

	// 카메라 모드 설정
	InCamera->SetCameraType(CameraModeIndex == 0 ? ECameraType::ECT_Perspective : ECameraType::ECT_Orthographic);

	// 카메라 파라미터 설정
	UiNearZ = max(0.0001f, UiNearZ);
	UiFarZ = max(UiNearZ + 0.0001f, UiFarZ);
	UiFovY = min(170.0f, max(1.0f, UiFovY));

	InCamera->SetNearZ(UiNearZ);
	InCamera->SetFarZ(UiFarZ);
	InCamera->SetFovY(UiFovY);

	// 카메라 업데이트
	if (CameraModeIndex == 0)
	{
		InCamera->UpdateMatrixByPers();
	}
	else
	{
		InCamera->UpdateMatrixByOrth();
	}
}

void UCameraControlWidget::SetCamera(UCamera* InCamera, const int Index)
{
	// 적정 범위가 아니라면 취소
	if (Index < 0 || Index >= Cameras.size()) { return; }

	Cameras[Index] = InCamera;
}
