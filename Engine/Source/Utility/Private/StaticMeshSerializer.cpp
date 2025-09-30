#include "pch.h"
#include "Utility/Public/StaticMeshSerializer.h"
#include "Component/Mesh/Public/StaticMesh.h"
#include "Global/CoreTypes.h"
#include <fstream>

namespace
{
    // Helper to write a std::string or FString
    void WriteString(std::ofstream& stream, const FString& str)
    {
        size_t len = str.length();
        stream.write(reinterpret_cast<const char*>(&len), sizeof(len));
        if (len > 0)
        {
            stream.write(str.c_str(), len);
        }
    }

    // Helper to read into a std::string or FString
    void ReadString(std::ifstream& stream, FString& str)
    {
        size_t len;
        stream.read(reinterpret_cast<char*>(&len), sizeof(len));
        if (len > 0)
        {
            str.resize(len);
            stream.read(&str[0], len);
        }
        else
        {
            str.clear();
        }
    }

    // Helper to write a TArray of POD types
    template<typename T>
    void WriteTArray(std::ofstream& stream, const TArray<T>& arr)
    {
        size_t size = arr.size();
        stream.write(reinterpret_cast<const char*>(&size), sizeof(size));
        if (size > 0)
        {
            stream.write(reinterpret_cast<const char*>(arr.data()), size * sizeof(T));
        }
    }

    // Helper to read a TArray of POD types
    template<typename T>
    void ReadTArray(std::ifstream& stream, TArray<T>& arr)
    {
        size_t size;
        stream.read(reinterpret_cast<char*>(&size), sizeof(size));
        if (size > 0)
        {
            arr.resize(size);
            stream.read(reinterpret_cast<char*>(arr.data()), size * sizeof(T));
        }
        else
        {
            arr.clear();
        }
    }
}

void StaticMeshSerializer::SaveFStaticMeshToBin(FStaticMesh* Mesh, const std::filesystem::path& Path)
{
    if (!Mesh) return;

    std::ofstream outFile(Path, std::ios::binary | std::ios::trunc);
    if (!outFile.is_open())
    {
        // UE_LOG_ERROR("Failed to open file for writing: %s", Path.string().c_str());
        return;
    }

    // --- Serialize FStaticMesh ---
    WriteString(outFile, Mesh->PathFileName.ToString());
    WriteTArray(outFile, Mesh->Vertices);
    WriteTArray(outFile, Mesh->Indices);
    WriteTArray(outFile, Mesh->Sections);

    // Serialize MaterialInfo
    size_t numMaterials = Mesh->MaterialInfo.size();
    outFile.write(reinterpret_cast<const char*>(&numMaterials), sizeof(numMaterials));
    for (const auto& material : Mesh->MaterialInfo)
    {
        WriteString(outFile, material.Name);
        outFile.write(reinterpret_cast<const char*>(&material.Ka), sizeof(FVector));
        outFile.write(reinterpret_cast<const char*>(&material.Kd), sizeof(FVector));
        outFile.write(reinterpret_cast<const char*>(&material.Ks), sizeof(FVector));
        outFile.write(reinterpret_cast<const char*>(&material.Ke), sizeof(FVector));
        outFile.write(reinterpret_cast<const char*>(&material.Ns), sizeof(float));
        outFile.write(reinterpret_cast<const char*>(&material.Ni), sizeof(float));
        outFile.write(reinterpret_cast<const char*>(&material.D), sizeof(float));
        outFile.write(reinterpret_cast<const char*>(&material.Illumination), sizeof(int32));
        WriteString(outFile, material.KaMap);
        WriteString(outFile, material.KdMap);
        WriteString(outFile, material.KsMap);
        WriteString(outFile, material.NsMap);
        WriteString(outFile, material.DMap);
        WriteString(outFile, material.BumpMap);
    }
}

FStaticMesh* StaticMeshSerializer::LoadFStaticMeshFromBin(const std::filesystem::path& Path)
{
    std::ifstream inFile(Path, std::ios::binary);
    if (!inFile.is_open())
    {
        // UE_LOG_WARN("Failed to open file for reading: %s", Path.string().c_str());
        return nullptr;
    }

    FStaticMesh* LoadedMesh = new FStaticMesh();

    // --- Deserialize FStaticMesh ---
    FString pathName;
    ReadString(inFile, pathName);
    LoadedMesh->PathFileName = FName(pathName);

    ReadTArray(inFile, LoadedMesh->Vertices);
    ReadTArray(inFile, LoadedMesh->Indices);
    ReadTArray(inFile, LoadedMesh->Sections);

    // Deserialize MaterialInfo
    size_t numMaterials;
    inFile.read(reinterpret_cast<char*>(&numMaterials), sizeof(numMaterials));
    LoadedMesh->MaterialInfo.resize(numMaterials);
    for (size_t i = 0; i < numMaterials; ++i)
    {
        FMaterial& material = LoadedMesh->MaterialInfo[i];
        ReadString(inFile, material.Name);
        inFile.read(reinterpret_cast<char*>(&material.Ka), sizeof(FVector));
        inFile.read(reinterpret_cast<char*>(&material.Kd), sizeof(FVector));
        inFile.read(reinterpret_cast<char*>(&material.Ks), sizeof(FVector));
        inFile.read(reinterpret_cast<char*>(&material.Ke), sizeof(FVector));
        inFile.read(reinterpret_cast<char*>(&material.Ns), sizeof(float));
        inFile.read(reinterpret_cast<char*>(&material.Ni), sizeof(float));
        inFile.read(reinterpret_cast<char*>(&material.D), sizeof(float));
        inFile.read(reinterpret_cast<char*>(&material.Illumination), sizeof(int32));
        ReadString(inFile, material.KaMap);
        ReadString(inFile, material.KdMap);
        ReadString(inFile, material.KsMap);
        ReadString(inFile, material.NsMap);
        ReadString(inFile, material.DMap);
        ReadString(inFile, material.BumpMap);
    }

    return LoadedMesh;
}
