#pragma once

#include <filesystem>

// Forward declarations
struct FStaticMesh;

class StaticMeshSerializer
{
public:
    static void SaveFStaticMeshToBin(FStaticMesh* Mesh, const std::filesystem::path& Path);
    static FStaticMesh* LoadFStaticMeshFromBin(const std::filesystem::path& Path);
};
