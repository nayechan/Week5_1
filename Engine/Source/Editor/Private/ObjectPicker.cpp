#include "pch.h"
#include "Editor/Public/ObjectPicker.h"
#include "Editor/Public/Camera.h"
#include "Editor/Public/Gizmo.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Manager/Input/Public/InputManager.h"
#include "Core/Public/AppWindow.h"
#include "ImGui/imgui.h"
#include "Level/Public/Level.h"
#include "Global/Quaternion.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Component/Mesh/Public/StaticMesh.h"
#include "Utility/Public/StaticMeshBVH.h"
#include "Utility/Public/ScopeCycleCounter.h"

FRay UObjectPicker::GetModelRay(const FRay& Ray, UPrimitiveComponent* Primitive)
{
	FMatrix ModelInverse = Primitive->GetWorldTransformInverse();

	FRay ModelRay;
	ModelRay.Origin = Ray.Origin * ModelInverse;
	ModelRay.Direction = Ray.Direction * ModelInverse;
	ModelRay.Direction.Normalize();
	return ModelRay;
}

UPrimitiveComponent* UObjectPicker::PickPrimitive(UCamera* InActiveCamera, const FRay& WorldRay, TArray<UPrimitiveComponent*>& Candidate, float* Distance)
{
	UPrimitiveComponent* ShortestPrimitive = nullptr;
	float ShortestDistance = D3D11_FLOAT32_MAX;
	float PrimitiveDistance = D3D11_FLOAT32_MAX;

	for (UPrimitiveComponent* Primitive : Candidate)
	{
		if (Primitive->GetPrimitiveType() == EPrimitiveType::BillBoard)
		{
			continue;
		}
		FMatrix ModelMat = Primitive->GetWorldTransform();
		FRay ModelRay = GetModelRay(WorldRay, Primitive);
		if (IsRayPrimitiveCollided(InActiveCamera, ModelRay, Primitive, ModelMat, &PrimitiveDistance))
			//Ray와 Primitive가 충돌했다면 거리 테스트 후 가까운 Actor Picking
		{
			if (PrimitiveDistance < ShortestDistance)
			{
				ShortestPrimitive = Primitive;
				ShortestDistance = PrimitiveDistance;
			}
		}
	}
	*Distance = ShortestDistance;

	return ShortestPrimitive;
}

bool UObjectPicker::PickPrimitive(UCamera* InActiveCamera, const FRay& WorldRay, UPrimitiveComponent* Primitive, float* OutDistance)
{
	if (!Primitive || Primitive->GetPrimitiveType() == EPrimitiveType::BillBoard)
	{
		if (OutDistance)
		{
			*OutDistance = D3D11_FLOAT32_MAX;
		}

		return false;
	}

	// Transform 계산
	FMatrix ModelMat = Primitive->GetWorldTransform();
	FRay ModelRay = GetModelRay(WorldRay, Primitive);

	float Dist = D3D11_FLOAT32_MAX;
	const bool bHit = IsRayPrimitiveCollided(InActiveCamera, ModelRay, Primitive, ModelMat, &Dist);
	if (OutDistance)
	{
		*OutDistance = bHit ? Dist : D3D11_FLOAT32_MAX;
	}

	return bHit;
}

void UObjectPicker::PickGizmo(UCamera* InActiveCamera, const FRay& WorldRay, UGizmo& Gizmo, FVector& CollisionPoint)
{
	//Forward, Right, Up순으로 테스트할거임.
	//원기둥 위의 한 점 P, 축 위의 임의의 점 A에(기즈모 포지션) 대해, AP벡터와 축 벡터 V와 피타고라스 정리를 적용해서 점 P의 축부터의 거리 r을 구할 수 있음.
	//r이 원기둥의 반지름과 같다고 방정식을 세운 후 근의공식을 적용해서 충돌여부 파악하고 distance를 구할 수 있음.

	//FVector4 PointOnCylinder = WorldRay.Origin + WorldRay.Direction * X;
	//dot(PointOnCylinder - GizmoLocation)*Dot(PointOnCylinder - GizmoLocation) - Dot(PointOnCylinder - GizmoLocation, GizmoAxis)^2 = r^2 = radiusOfGizmo
	//이 t에 대한 방정식을 풀어서 근의공식 적용하면 됨.

	FVector GizmoLocation = Gizmo.GetGizmoLocation();
	FVector GizmoAxises[3] = { FVector{1, 0, 0}, FVector{0, 1, 0}, FVector{0, 0, 1} };

	if (Gizmo.GetGizmoMode() == EGizmoMode::Scale || !Gizmo.IsWorldMode())
	{
		FVector Rad = FVector::GetDegreeToRadian(Gizmo.GetActorRotation());
		FMatrix R = FMatrix::RotationMatrix(Rad);
		//FQuaternion q = FQuaternion::FromEuler(Rad);

		for (int i = 0; i < 3; i++)
		{
			//GizmoAxises[a] = FQuaternion::RotateVector(q, GizmoAxises[a]); // 쿼터니언으로 축 회전
			//GizmoAxises[a].Normalize();
			const FVector4 a4(GizmoAxises[i].X, GizmoAxises[i].Y, GizmoAxises[i].Z, 0.0f);
			FVector4 rotated4 = a4 * R;
			FVector V(rotated4.X, rotated4.Y, rotated4.Z);
			V.Normalize();
			GizmoAxises[i] = V;
		}
	}

	FVector WorldRayOrigin{ WorldRay.Origin.X,WorldRay.Origin.Y ,WorldRay.Origin.Z };
	FVector WorldRayDirection(WorldRay.Direction.X, WorldRay.Direction.Y, WorldRay.Direction.Z);
	WorldRayDirection.Normalize();

	switch (Gizmo.GetGizmoMode())
	{
	case EGizmoMode::Translate:
	case EGizmoMode::Scale:
	{
		FVector GizmoDistanceVector = WorldRayOrigin - GizmoLocation;
		bool bIsCollide = false;

		float GizmoRadius = Gizmo.GetTranslateRadius();
		float GizmoHeight = Gizmo.GetTranslateHeight();
		float A, B, C; //Ax^2 + Bx + C의 ABC
		float X; //해
		float Det; //판별식
		//0 = forward 1 = Right 2 = UP

		for (int a = 0; a < 3; a++)
		{
			FVector GizmoAxis = GizmoAxises[a];
			A = 1 - static_cast<float>(pow(WorldRay.Direction.Dot3(GizmoAxis), 2));
			B = WorldRay.Direction.Dot3(GizmoDistanceVector) - WorldRay.Direction.Dot3(GizmoAxis) * GizmoDistanceVector.
				Dot(GizmoAxis); //B가 2의 배수이므로 미리 약분
			C = static_cast<float>(GizmoDistanceVector.Dot(GizmoDistanceVector) -
				pow(GizmoDistanceVector.Dot(GizmoAxis), 2)) - GizmoRadius * GizmoRadius;

			Det = B * B - A * C;
			if (Det >= 0) //판별식 0이상 => 근 존재. 높이테스트만 통과하면 충돌
			{
				X = (-B + sqrtf(Det)) / A;
				FVector PointOnCylinder = WorldRayOrigin + WorldRayDirection * X;
				float Height = (PointOnCylinder - GizmoLocation).Dot(GizmoAxis);
				if (Height <= GizmoHeight && Height >= 0) //충돌
				{
					CollisionPoint = PointOnCylinder;
					bIsCollide = true;

				}
				X = (-B - sqrtf(Det)) / A;
				PointOnCylinder = WorldRayOrigin + WorldRayDirection * X;
				Height = (PointOnCylinder - GizmoLocation).Dot(GizmoAxis);
				if (Height <= GizmoHeight && Height >= 0)
				{
					CollisionPoint = PointOnCylinder;
					bIsCollide = true;
				}
				if (bIsCollide)
				{
					switch (a)
					{
					case 0:	Gizmo.SetGizmoDirection(EGizmoDirection::Forward);	return;
					case 1:	Gizmo.SetGizmoDirection(EGizmoDirection::Right);	return;
					case 2:	Gizmo.SetGizmoDirection(EGizmoDirection::Up);		return;
					}
				}
			}
		}
	} break;
	case EGizmoMode::Rotate:
	{
		for (int a = 0; a < 3; a++)
		{
			if (IsRayCollideWithPlane(WorldRay, GizmoLocation, GizmoAxises[a], CollisionPoint))
			{
				FVector RadiusVector = CollisionPoint - GizmoLocation;
				if (Gizmo.IsInRadius(RadiusVector.Length()))
				{
					switch (a)
					{
					case 0:	Gizmo.SetGizmoDirection(EGizmoDirection::Forward);	return;
					case 1:	Gizmo.SetGizmoDirection(EGizmoDirection::Right);	return;
					case 2:	Gizmo.SetGizmoDirection(EGizmoDirection::Up);		return;
					}
				}
			}
		}
	} break;
	default: break;
	}

	Gizmo.SetGizmoDirection(EGizmoDirection::None);
}

// 개별 primitive와 ray 충돌 검사
bool UObjectPicker::IsRayPrimitiveCollided(UCamera* InActiveCamera, const FRay& ModelRay, UPrimitiveComponent* Primitive, const FMatrix& ModelMatrix, float* ShortestDistance)
{
	const TArray<FNormalVertex>* Vertices = nullptr;
	const TArray<uint32>* Indices = nullptr;

	if (auto* SMC = Cast<UStaticMeshComponent>(Primitive))
	{
		UStaticMesh* Mesh = SMC->GetStaticMesh();
		if (!Mesh || !Mesh->IsValid()) return false;

		// For static meshes, always use LOD 0 for accurate picking
		Vertices = &Mesh->GetVertices(0);
		Indices = &Mesh->GetIndices(0);

		// Try to use the mesh's internal BVH for fast picking
		if (const FStaticMeshBVH* MBVH = UAssetManager::GetInstance().GetStaticMeshBVH(Mesh))
		{
			if (!Vertices || Vertices->empty()) return false;

			bool bHit = false;
			float LocalBest = *ShortestDistance;

				// 모델T 컷오프(있으면 front-to-back 가지치기 강화)
				float CutoffT = std::numeric_limits<float>::infinity();

				// Compute world-space ray direction once (wd = ModelRay.Direction * ModelMatrix)
				FVector4 md(ModelRay.Direction.X, ModelRay.Direction.Y, ModelRay.Direction.Z, 0.f);
				FVector4 wd4 = md * ModelMatrix;
				const float wdX = wd4.X, wdY = wd4.Y, wdZ = wd4.Z;
				const float wdLen = sqrtf(wdX * wdX + wdY * wdY + wdZ * wdZ);

				// Compute dot with camera forward once
				const FVector CameraForward = InActiveCamera->GetForward();
				const float wdDotCam = wdX * CameraForward.X + wdY * CameraForward.Y + wdZ * CameraForward.Z;

				if (wdLen > 1e-8f && LocalBest < D3D11_FLOAT32_MAX * 0.5f)
					CutoffT = LocalBest / wdLen;

				// Hoist raw pointers for faster per-triangle fetch
				const FNormalVertex* VertPtr = Vertices->empty() ? nullptr : Vertices->data();
				const uint32* IdxPtr = Indices ? Indices->data() : nullptr;

				MBVH->TraverseFrontToBack(ModelRay, CutoffT,
					[&](int32 TriIndex)
					{
#ifdef ENABLE_BVH_STATS
						FScopeCycleCounter Counter(GetVisitTriangleStatId());
#endif

						// fetch triangle data from mesh-level SoA (v0 + edges)
						FVector v0, e1, e2;
						MBVH->GetTriV0E1E2(TriIndex, v0, e1, e2);

						// reconstruct v1/v2 from v0 + edges
						const FVector v1 = FVector(v0.X + e1.X, v0.Y + e1.Y, v0.Z + e1.Z);
						const FVector v2 = FVector(v0.X + e2.X, v0.Y + e2.Y, v0.Z + e2.Z);

						float Dist = LocalBest;

						// Pass precomputed wdLen and wdDotCam to avoid per-hit transforms
						if (IsRayTriangleCollided(InActiveCamera, ModelRay, v0, v1, v2, wdLen, wdDotCam, &Dist))
						{
							if (Dist < LocalBest)
							{
								LocalBest = Dist;
								*ShortestDistance = Dist;
								bHit = true;
							}
						}
					});

			return bHit;
		}
	}
    else
    {
        // For other primitive types (Cube, Sphere), use the old method
        Vertices = Primitive->GetVerticesData();
        Indices = Primitive->GetIndicesData();
    }

	// Fallback for non-BVH static meshes or simple primitives
	if (!Vertices || Vertices->empty()) return false;

	const uint32 NumVertices = Vertices->size();
	const uint32 NumIndices = Indices ? Indices->size() : 0;

	float Distance = D3D11_FLOAT32_MAX;
	bool bAnyHit = false;
	const int32 NumTriangles = (NumIndices > 0) ? (NumIndices / 3) : (NumVertices / 3);

	// Precompute world direction length & dot with camera forward for fallback
	{
		FVector4 md(ModelRay.Direction.X, ModelRay.Direction.Y, ModelRay.Direction.Z, 0.f);
		FVector4 wd4 = md * ModelMatrix;
		const float wdX = wd4.X, wdY = wd4.Y, wdZ = wd4.Z;
		const float wdLen = sqrtf(wdX * wdX + wdY * wdY + wdZ * wdZ);
		const FVector CameraForward = InActiveCamera->GetForward();
		const float wdDotCam = wdX * CameraForward.X + wdY * CameraForward.Y + wdZ * CameraForward.Z;

		for (int32 TriIndex = 0; TriIndex < NumTriangles; ++TriIndex)
		{
			FVector v0, v1, v2;
			if (Indices)
			{
				v0 = (*Vertices)[(*Indices)[TriIndex * 3 + 0]].Position;
				v1 = (*Vertices)[(*Indices)[TriIndex * 3 + 1]].Position;
				v2 = (*Vertices)[(*Indices)[TriIndex * 3 + 2]].Position;
			}
			else
			{
				v0 = (*Vertices)[TriIndex * 3 + 0].Position;
				v1 = (*Vertices)[TriIndex * 3 + 1].Position;
				v2 = (*Vertices)[TriIndex * 3 + 2].Position;
			}

			if (IsRayTriangleCollided(InActiveCamera, ModelRay, v0, v1, v2, wdLen, wdDotCam, &Distance))
			{
				bAnyHit = true;
				if (Distance < *ShortestDistance)
				{
					*ShortestDistance = Distance;
				}
			}
		}
	}

	return bAnyHit;
}

// Updated triangle test: no per-hit world matrix multiplications or vector-lengths
bool UObjectPicker::IsRayTriangleCollided(UCamera* InActiveCamera, const FRay& Ray, const FVector& Vertex1, const FVector& Vertex2, const FVector& Vertex3,
	float WorldDirLen, float WorldDirDotCam, float* Distance)
{
	// Early locals in model space
	FVector RayDirection{ Ray.Direction.X, Ray.Direction.Y, Ray.Direction.Z };
	FVector RayOrigin{ Ray.Origin.X, Ray.Origin.Y, Ray.Origin.Z };
	FVector E1 = Vertex2 - Vertex1;
	FVector E2 = Vertex3 - Vertex1;
	FVector Result = (RayOrigin - Vertex1);

	FVector CrossE2Ray = E2.Cross(RayDirection);
	FVector CrossE1Result = E1.Cross(Result);

	float Determinant = E1.Dot(CrossE2Ray);

	const float NoInverse = 0.0001f;
	if (abs(Determinant) <= NoInverse) return false;

	// Back-face culling
	if (Determinant >= 0.0f) return false;

	float V = Result.Dot(CrossE2Ray) / Determinant;
	if (V < 0 || V > 1) return false;

	float U = RayDirection.Dot(CrossE1Result) / Determinant;
	if (U < 0 || U + V > 1) return false;

	float T = E2.Dot(CrossE1Result) / Determinant;

	// Use precomputed world direction info to test view frustum (near/far) and compute world distance
	// projection along camera forward = T * (wd dot cameraForward)
	const float projAlongCamera = T * WorldDirDotCam;
	const float NearZ = InActiveCamera->GetNearZ();
	const float FarZ = InActiveCamera->GetFarZ();
	if (projAlongCamera >= NearZ && projAlongCamera <= FarZ)
	{
		// World-space distance from ray origin to hit = T * worldDirLen
		*Distance = T * WorldDirLen;
		return true;
	}
	return false;
}

bool UObjectPicker::IsRayCollideWithPlane(const FRay& WorldRay, FVector PlanePoint, FVector Normal, FVector& PointOnPlane)
{
	FVector WorldRayOrigin{ WorldRay.Origin.X, WorldRay.Origin.Y ,WorldRay.Origin.Z };

	if (abs(WorldRay.Direction.Dot3(Normal)) < 0.01f)
		return false;

	float Distance = (PlanePoint - WorldRayOrigin).Dot(Normal) / WorldRay.Direction.Dot3(Normal);

	if (Distance < 0)
		return false;
	PointOnPlane = WorldRay.Origin + WorldRay.Direction * Distance;


	return true;
}
