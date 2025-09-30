#pragma once
#include "Core/Public/Object.h"
#include "Editor/Public/Gizmo.h"
#include "Editor/Public/Grid.h"
#include "Editor/public/Axis.h"
#include "Editor/Public/ObjectPicker.h"
#include "Editor/Public/BatchLines.h"
#include "Editor/Public/SplitterWindow.h"
#include "Utility/Public/SceneBVH.h"

class UPrimitiveComponent;
class FViewportClient;
class UCamera;
class ULevel;
class USplitterWidget;
struct FRay;

enum class EViewModeIndex : uint32
{
	VMI_Lit,
	VMI_Unlit,
	VMI_Wireframe,
};

enum class EViewportLayoutState
{
	Multi,
	Single,
	Animating,
};

class UEditor : public UObject
{
public:
	UEditor();
	~UEditor();

	void Update();
	void RenderEditor(UCamera* InCamera);

	void SetViewMode(EViewModeIndex InNewViewMode) { CurrentViewMode = InNewViewMode; }
	EViewModeIndex GetViewMode() const { return CurrentViewMode; }

	void SetSingleViewportLayout(int InActiveIndex);
	void RestoreMultiViewportLayout();
	// 새로운 레벨로 전환할 때 BVH 리셋
	void ResetBVH();

private:
	void InitializeLayout();
	void UpdateLayout();

	// 선택 액터의 프리미티브를 더티로 등록
	void MarkActorPrimitivesDirty(AActor* InActor);

	void ProcessMouseInput(ULevel* InLevel);
	TArray<UPrimitiveComponent*> FindCandidatePrimitives(ULevel* InLevel);

	// BVH 선행 갱신(씬 로딩/변경 시 Build, 드래그 종료 더티 전달 시 부분 Refit, 주기적 전체 Refit)
	void EnsureBVHUpToDate(ULevel* InLevel);
	void PrewarmPicking(ULevel* InLevel, UCamera* InCamera);
	// BVH를 사용해 피킹 후보를 모으고 최종 Primitive를 반환
	UPrimitiveComponent* PickPrimitiveUsingBVH(UCamera* InCamera, const FRay& WorldRay, ULevel* InLevel, float* OutDistance);
	// Precise test helper extracted from PickPrimitiveUsingBVH to improve readability.
	// New signature: accepts the primitive index and the AABB ray TMin computed by BVH traversal.
	// Returns true if Prim is hit and updates InOutHitDist with hit distance.
	bool PreciseTestForPick(UCamera* InCamera, const FRay& WorldRay, UPrimitiveComponent* Prim, int64 PrimIndex, float PrimPTMin,
		float& InOutHitDist, const FVector& RayO, const FVector& RayD);

	// 모든 기즈모 드래그 함수가 ActiveCamera를 받도록 통일
	FVector GetGizmoDragLocation(UCamera* InActiveCamera, FRay& WorldRay);
	FVector GetGizmoDragRotation(UCamera* InActiveCamera, FRay& WorldRay);
	FVector GetGizmoDragScale(UCamera* InActiveCamera, FRay& WorldRay);

	inline float Lerp(const float A, const float B, const float Alpha)
	{
		return A * (1 - Alpha) + B * Alpha;
	}

	UObjectPicker ObjectPicker;

	const float MinScale = 0.01f;
	float SavedRootRatio = 0.5f;
	float SavedLeftRatio = 0.5f;
	float SavedRightRatio = 0.5f;
	UGizmo Gizmo;
	UAxis Axis;
	UBatchLines BatchLines;

	SSplitterV RootSplitter;
	SSplitterH LeftSplitter;
	SSplitterH RightSplitter;
	SWindow ViewportWindows[4]; // 최종 뷰포트 영역의 정보, 쉽게 참조하도록 선언했습니다.
	SSplitter* DraggedSplitter = nullptr; // 드래그 상태를 추적하는 포인터
	FViewportClient* InteractionViewport = nullptr; // 뷰포트의 상호작용을 고정하는 포인터

	EViewModeIndex CurrentViewMode = EViewModeIndex::VMI_Lit;

	// Animation
	EViewportLayoutState ViewportLayoutState = EViewportLayoutState::Multi;
	EViewportLayoutState TargetViewportLayoutState = EViewportLayoutState::Multi;
	float AnimationStartTime = 0.0f;
	float AnimationDuration = 0.2f; 
	float SourceRootRatio = 0.5f;
	float SourceLeftRatio = 0.5f;
	float SourceRightRatio = 0.5f;
	float TargetRootRatio = 0.5f;
	float TargetLeftRatio = 0.5f;
	float TargetRightRatio = 0.5f;

	// Scene BVH 캐시
	FSceneBVH SceneBVH{};
	int64 SceneBVHLastCount = 0;
	int64 SceneBVHRefitTick = 0;
	bool bPickingWarmed = false;

	// 드래그 동안 누적되는 더티 프리미티브 큐
	TArray<UPrimitiveComponent*> PendingDirtyPrims;
};
