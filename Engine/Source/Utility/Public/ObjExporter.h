#pragma once

#include "Core/Public/Object.h"
#include "Global/Types.h"
#include "Global/CoreTypes.h"
#include "Component/Mesh/Public/StaticMesh.h" // For FMeshSection

struct FStaticMesh;
class UStaticMesh;

/**
 * @brief Utility class for exporting mesh data to OBJ format
 * Useful for debugging LOD generation and inspecting mesh data in external tools
 */
UCLASS()
class UObjExporter : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * @brief Exports a FStaticMesh to an OBJ file
	 * @param InMesh The mesh data to export
	 * @param InFilePath Output file path (should end with .obj)
	 * @return true if export succeeded, false otherwise
	 */
	static bool ExportMesh(const FStaticMesh* InMesh, const FString& InFilePath, const FString& InMtlName);

	/**
	 * @brief Exports a specific LOD level of a UStaticMesh to an OBJ file
	 * @param InStaticMesh The UStaticMesh to export from
	 * @param InLODIndex LOD level to export (0 = base mesh, 1 = LOD1, etc.)
	 * @param InFilePath Output file path (should end with .obj)
	 * @return true if export succeeded, false otherwise
	 */
	static bool ExportLOD(const UStaticMesh* InStaticMesh, int32 InLODIndex, const FString& InFilePath);

	/**
	 * @brief Exports all LOD levels of a UStaticMesh to separate OBJ files
	 * Files will be named as: basename_LOD0.obj, basename_LOD1.obj, etc.
	 * @param InStaticMesh The UStaticMesh to export from
	 * @param InBasePath Base file path (without .obj extension)
	 * @return Number of LOD levels successfully exported
	 */
	static int32 ExportAllLODs(const UStaticMesh* InStaticMesh, const FString& InBasePath);

private:
	/**
	 * @brief Internal helper to write vertex data to OBJ format
	 */
	static void WriteVertexData(std::ofstream& OutFile, const TArray<FNormalVertex>& InVertices);

	/**
	 * @brief Internal helper to write face data to OBJ format
	 */
	static void WriteFaceData(std::ofstream& OutFile, const TArray<uint32>& InIndices);

	/**
	 * @brief Internal helper to write face data with material groups to OBJ format
	 */
	static void WriteFaceDataWithMaterials(std::ofstream& OutFile, const TArray<uint32>& InIndices, const TArray<FMeshSection>& InSections);
};