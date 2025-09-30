#pragma once
#include "Core/Public/Object.h"
#include "Render/Spatial/Public/Frustum.h"

class UConfigManager;

enum class ECameraType
{
	ECT_Orthographic,
	ECT_Perspective
};

class UCamera : public UObject
{
public:
	// Camera Speed Constants
	static constexpr float MIN_SPEED = 10.0f;
	static constexpr float MAX_SPEED = 70.0f;
	static constexpr float DEFAULT_SPEED = 20.0f;
	static constexpr float SPEED_ADJUST_STEP = 1.0f;

	UCamera();
	~UCamera() override;

	/* *
	* @brief Update 관련 함수
	* UpdateInput 함수는 사용자 입력으로 비롯된 변화의 갱신를 담당합니다.
	* Update, UpdateMatrix 함수들은 카메라의 변환 행렬의 갱신을 담당합니다.
	*/
	FVector UpdateInput();
	void Update(const D3D11_VIEWPORT& InViewport);
	void UpdateMatrixByPers();
	void UpdateMatrixByOrth();

	/**
	 * @brief Setter
	 */
	void SetLocation(const FVector& InOtherPosition) { RelativeLocation = InOtherPosition; MarkFrustumDirty(); }
	void SetRotation(const FVector& InOtherRotation) { RelativeRotation = InOtherRotation; MarkFrustumDirty(); }
	void SetFovY(const float InOtherFovY) { FovY = InOtherFovY; MarkFrustumDirty(); }
	void SetAspect(const float InOtherAspect) { Aspect = InOtherAspect; MarkFrustumDirty(); }
	void SetNearZ(const float InOtherNearZ) { NearZ = InOtherNearZ; MarkFrustumDirty(); }
	void SetFarZ(const float InOtherFarZ) { FarZ = InOtherFarZ; MarkFrustumDirty(); }
	void SetOrthoWidth(const float InOrthoWidth) { OrthoWidth = InOrthoWidth; MarkFrustumDirty(); }
	void SetCameraType(const ECameraType InCameraType) { CameraType = InCameraType; MarkFrustumDirty(); }

	/**
	 * @brief Getter
	 */
	const FViewProjConstants& GetFViewProjConstants() const { return ViewProjConstants; }
	const FViewProjConstants GetFViewProjConstantsInverse() const;

	FRay ConvertToWorldRay(float NdcX, float NdcY) const;

	// View Frustum Culling
	const FFrustum& GetViewFrustum() const;

	FVector CalculatePlaneNormal(const FVector4& Axis);
	FVector CalculatePlaneNormal(const FVector& Axis);
	FVector& GetLocation() { return RelativeLocation; }
	FVector& GetRotation() { return RelativeRotation; }
	const FVector& GetLocation() const { return RelativeLocation; }
	const FVector& GetRotation() const { return RelativeRotation; }
	const FVector& GetForward() const { return Forward; }
	const FVector& GetUp() const { return Up; }
	const FVector& GetRight() const { return Right; }
	float GetFovY() const { return FovY; }
	float GetAspect() const { return Aspect; }
	float GetNearZ() const { return NearZ; }
	float GetFarZ() const { return FarZ; }
	float GetOrthoWidth() const { return OrthoWidth; }
	ECameraType GetCameraType() const { return CameraType; }

	// Camera Movement Speed Control
	float GetMoveSpeed() const { return CurrentMoveSpeed; }
	void SetMoveSpeed(float InSpeed)
	{
		CurrentMoveSpeed = clamp(InSpeed, MIN_SPEED, MAX_SPEED);
		// CurrentMoveSpeed = min(InSpeed, MAX_SPEED);
	}
	void AdjustMoveSpeed(float InDelta) { SetMoveSpeed(CurrentMoveSpeed + InDelta); }

	/* *
	 * @brief 행렬 형태로 저장된 좌표와 변환 행렬과의 연산한 결과를 반환합니다.
	 */
	inline FVector4 MultiplyPointWithMatrix(const FVector4& Point, const FMatrix& Matrix) const
	{
		FVector4 Result = Point * Matrix;
		/* *
		 * @brief 좌표가 왜곡된 공간에 남는 것을 방지합니다.
		 */
		if (Result.W != 0.f) { Result *= (1.f / Result.W); }

		return Result;
	}

private:
	FViewProjConstants ViewProjConstants = {};
	FVector RelativeLocation = {};
	FVector RelativeRotation = {};
	FVector Forward = { 1,0,0 };
	FVector Up = {0,0,1};
	FVector Right = {0,1,0};
	float FovY = {};
	float Aspect = {};
	float NearZ = {};
	float FarZ = {};
	float OrthoWidth = {};
	ECameraType CameraType = {};

	// Dynamic Movement Speed
	float CurrentMoveSpeed = DEFAULT_SPEED;

	// Frustum Culling
	mutable FFrustum CachedFrustum;
	mutable bool bFrustumDirty = true;
	void MarkFrustumDirty() { bFrustumDirty = true; }
};
