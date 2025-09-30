#include "pch.h"
#include "Utility/Public/ObjExporter.h"
#include "Component/Mesh/Public/StaticMesh.h"
#include "Global/CoreTypes.h"
#include <fstream>
#include <filesystem>

bool UObjExporter::ExportMesh(const FStaticMesh* InMesh, const FString& InFilePath, const FString& InMtlName)
{
	if (!InMesh || InMesh->Vertices.empty())
	{
		UE_LOG_ERROR("ExportMesh: Invalid mesh data provided");
		return false;
	}

	// Create output directory if it doesn't exist
	std::filesystem::path FilePath(InFilePath);
	std::filesystem::path ParentDir = FilePath.parent_path();
	if (!ParentDir.empty() && !std::filesystem::exists(ParentDir))
	{
		std::filesystem::create_directories(ParentDir);
	}

	std::ofstream OutFile(InFilePath);
	if (!OutFile.is_open())
	{
		UE_LOG_ERROR("ExportMesh: Failed to create output file: %s", InFilePath.c_str());
		return false;
	}

	// Write OBJ header
	OutFile << "# OBJ file exported from GTL Engine\n";
	OutFile << "# Vertices: " << InMesh->Vertices.size() << "\n";
	OutFile << "# Triangles: " << InMesh->Indices.size() / 3 << "\n";
	OutFile << "\n";

	// Write MTL reference if a material name is provided
	if (!InMtlName.empty())
	{
		OutFile << "mtllib " << InMtlName << "\n\n";
	}

	// Write vertex data
	WriteVertexData(OutFile, InMesh->Vertices);

	// Write face data with material groups
	WriteFaceDataWithMaterials(OutFile, InMesh->Indices, InMesh->Sections);

	OutFile.close();
	
	UE_LOG("ExportMesh: Successfully exported mesh to %s", InFilePath.c_str());
	return true;
}

bool UObjExporter::ExportLOD(const UStaticMesh* InStaticMesh, int32 InLODIndex, const FString& InFilePath)
{
	if (!InStaticMesh || !InStaticMesh->IsValid())
	{
		UE_LOG_ERROR("ExportLOD: Invalid UStaticMesh provided");
		return false;
	}

	FStaticMesh* LODMesh = InStaticMesh->GetLOD(InLODIndex);
	if (!LODMesh)
	{
		UE_LOG_ERROR("ExportLOD: LOD %d not found in mesh", InLODIndex);
		return false;
	}

	return ExportMesh(LODMesh, InFilePath, "");
}

int32 UObjExporter::ExportAllLODs(const UStaticMesh* InStaticMesh, const FString& InBasePath)
{
	if (!InStaticMesh || !InStaticMesh->IsValid())
	{
		UE_LOG_ERROR("ExportAllLODs: Invalid UStaticMesh provided");
		return 0;
	}

	int32 ExportedCount = 0;
	int32 NumLODs = InStaticMesh->GetNumLODs();

	for (int32 LODIndex = 0; LODIndex < NumLODs; ++LODIndex)
	{
		FString LODFilePath = InBasePath + "_LOD" + std::to_string(LODIndex) + ".obj";
		
		if (ExportLOD(InStaticMesh, LODIndex, LODFilePath))
		{
			ExportedCount++;
		}
	}

	UE_LOG("ExportAllLODs: Exported %d/%d LOD levels for mesh", ExportedCount, NumLODs);
	return ExportedCount;
}

void UObjExporter::WriteVertexData(std::ofstream& OutFile, const TArray<FNormalVertex>& InVertices)
{
	// Write positions
	OutFile << "# Vertex positions\n";
	for (const auto& Vertex : InVertices)
	{
		// Multiple rotations: final result after X-axis 180° rotation → (-X,Y,Z)
		float ObjX = Vertex.Position.X;  // X stays flipped (X-axis rotation doesn't change X)
		float ObjY = Vertex.Position.Y;   // Y restored (X-axis rotation flips Y back)
		float ObjZ = -Vertex.Position.Z;   // Z restored (X-axis rotation flips Z back)
		OutFile << "v " << ObjX << " " << ObjY << " " << ObjZ << "\n";
	}
	OutFile << "\n";

	// Write texture coordinates
	OutFile << "# Texture coordinates\n";
	for (const auto& Vertex : InVertices)
	{
		// Convert back from UE coordinate system to OBJ coordinate system
		float ObjY = 1.0f - Vertex.TexCoord.Y;
		OutFile << "vt " << Vertex.TexCoord.X << " " << ObjY << "\n";
	}
	OutFile << "\n";

	// Write normals
	OutFile << "# Vertex normals\n";
	for (const auto& Vertex : InVertices)
	{
		OutFile << "vn " << Vertex.Normal.X << " " << Vertex.Normal.Y << " " << Vertex.Normal.Z << "\n";
	}
	OutFile << "\n";
}

void UObjExporter::WriteFaceData(std::ofstream& OutFile, const TArray<uint32>& InIndices)
{
	OutFile << "# Faces\n";

	for (size_t i = 0; i < InIndices.size(); i += 3)
	{
		if (i + 2 >= InIndices.size()) break;

		// OBJ indices are 1-based, so add 1 to each index
		uint32 v1 = InIndices[i] + 1;
		uint32 v2 = InIndices[i + 1] + 1;
		uint32 v3 = InIndices[i + 2] + 1;

		// Format: f v/vt/vn v/vt/vn v/vt/vn
		OutFile << "f " << v1 << "/" << v1 << "/" << v1 << " "
				<< v2 << "/" << v2 << "/" << v2 << " "
				<< v3 << "/" << v3 << "/" << v3 << "\n";
	}
}

void UObjExporter::WriteFaceDataWithMaterials(std::ofstream& OutFile, const TArray<uint32>& InIndices, const TArray<FMeshSection>& InSections)
{
	OutFile << "# Faces\n";

	if (InSections.empty())
	{
		// No materials, fallback to simple face export
		WriteFaceData(OutFile, InIndices);
		return;
	}

	for (const auto& Section : InSections)
	{
		// Write material group
		OutFile << "usemtl MatID_" << Section.MaterialSlot + 1 << "\n";

		// Write faces for this material
		uint32 StartIndex = Section.StartIndex;
		uint32 EndIndex = StartIndex + Section.IndexCount;

		for (uint32 i = StartIndex; i < EndIndex; i += 3)
		{
			if (i + 2 >= InIndices.size()) break;

			// OBJ indices are 1-based, so add 1 to each index
			uint32 v1 = InIndices[i] + 1;
			uint32 v2 = InIndices[i + 1] + 1;
			uint32 v3 = InIndices[i + 2] + 1;

			// Format: f v/vt/vn v/vt/vn v/vt/vn
			OutFile << "f " << v1 << "/" << v1 << "/" << v1 << " "
					<< v2 << "/" << v2 << "/" << v2 << " "
					<< v3 << "/" << v3 << "/" << v3 << "\n";
		}
		OutFile << "\n";
	}
}