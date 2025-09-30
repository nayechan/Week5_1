#pragma once

#include "Core/Public/Object.h"       // UObject 기반 클래스 및 매크로
#include "Core/Public/ObjectPtr.h" // TObjectPtr 사용
#include "Global/CoreTypes.h"        // TArray 등

// 전방 선언: FStaticMesh의 전체 정의를 포함할 필요 없이 포인터만 사용
struct FMeshSection
{
	uint32 StartIndex;
	uint32 IndexCount;
	uint32 MaterialSlot;
};

// Cooked Data
struct FStaticMesh
{
	FName PathFileName;

	TArray<FNormalVertex> Vertices;
	TArray<uint32> Indices;

	// --- 2. 재질 정보 (Materials) ---
	// 이 메시에 사용되는 모든 고유 재질의 목록 (페인트 팔레트)
	//TArray<UMaterial> Materials;

	TArray<FMaterial> MaterialInfo;

	// --- 3. 연결 정보 (Sections) ---
	// 각 재질을 어떤 기하 구간에 칠할지에 대한 지시서
	TArray<FMeshSection> Sections;
};


/**
 * @brief FStaticMesh(Cooked Data)를 엔진 오브젝트 시스템에 통합하는 래퍼 클래스.
 * 가비지 컬렉션, 리플렉션, 애셋 참조 관리의 대상이 됩니다.
 */
UCLASS()
class UStaticMesh : public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UStaticMesh, UObject)

public:
	UStaticMesh();
	virtual ~UStaticMesh(); // 가상 소멸자 권장

	// --- LOD 데이터 관리 ---
	void AddLOD(FStaticMesh* InLODData);
	FStaticMesh* GetLOD(int32 LODIndex) const;
	int32 GetNumLODs() const;

	// --- 데이터 접근자 (Getters) ---
	const FName& GetAssetPathFileName() const;

	// Geometry Data (LOD-aware)
	const TArray<FNormalVertex>& GetVertices(int32 LODIndex = 0) const;
	TArray<FNormalVertex>& GetVertices(int32 LODIndex = 0);
	const TArray<uint32>& GetIndices(int32 LODIndex = 0) const;

	// Material Data
	UMaterial* GetMaterial(int32 MaterialIndex) const;
	void SetMaterial(int32 MaterialIndex, UMaterial* Material);
	int32 GetNumMaterials() const;
	const TArray<FMeshSection>& GetSections(int32 LODIndex = 0) const;

	// 유효성 검사
	bool IsValid() const;

private:
	// 이 UStaticMesh가 소유하는 모든 LOD 레벨의 실제 데이터 본체.
	TArray<FStaticMesh*> AllLODs;

	TArray<UMaterial*> Materials;
};
