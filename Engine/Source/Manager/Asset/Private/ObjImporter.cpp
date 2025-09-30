#include "pch.h"

#include "Core/Public/WindowsBinReader.h"
#include "Core/Public/WindowsBinWriter.h"
#include "Manager/Asset/Public/ObjImporter.h"

bool FObjImporter::LoadObj(const std::filesystem::path& FilePath, FObjInfo* OutObjInfo, Configuration Config)
{
	if (!OutObjInfo)
	{
		return false;
	}

	if (!std::filesystem::exists(FilePath))
	{
		UE_LOG_ERROR("파일을 찾지 못했습니다: %s", FilePath.string().c_str());
		return false;
	}

	std::filesystem::path BinFilePath = FilePath;
	BinFilePath.replace_extension(".objbin");

	if (Config.bIsBinaryEnabled && std::filesystem::exists(BinFilePath))
	{
		auto ObjTime = std::filesystem::last_write_time(FilePath);
		auto BinTime = std::filesystem::last_write_time(BinFilePath);

		if (BinTime >= ObjTime)
		{
			UE_LOG("바이너리 파일이 존재합니다: %s", BinFilePath.string().c_str());
			FWindowsBinReader WindowsBinReader(BinFilePath);
			WindowsBinReader << *OutObjInfo;

			return true;
		}
		else
		{
			UE_LOG("바이너리 파일이 원본보다 오래되었습니다. 무시합니다: %s", BinFilePath.string().c_str());
		}
	}

	if (FilePath.extension() != ".obj")
	{
		UE_LOG_ERROR("잘못된 파일 확장자입니다: %s", FilePath.string().c_str());
		return false;
	}

	std::ifstream File(FilePath);
	if (!File)
	{
		UE_LOG_ERROR("파일을 열지 못했습니다: %s", FilePath.string().c_str());
		return false;
	}

	size_t FaceCount = 0;

	TOptional<FObjectInfo> OptObjectInfo;

	FString Buffer;
	while (std::getline(File, Buffer))
	{
		std::istringstream Tokenizer(Buffer);
		FString Prefix;

		Tokenizer >> Prefix;

		// ========================== Vertex Information ============================ //

		/** Vertex Position */
		if (Prefix == "v")
		{
			FVector Position;
			if (!(Tokenizer >> Position.X >> Position.Y >> Position.Z))
			{
				UE_LOG_ERROR("정점 위치 형식이 잘못되었습니다");
				return false;
			}

			if (Config.bPositionToUEBasis)
			{
				OutObjInfo->VertexList.emplace_back(PositionToUEBasis(Position));
			}
			else
			{
				OutObjInfo->VertexList.emplace_back(Position);
			}

		}
		/** Vertex Normal */
		else if (Prefix == "vn")
		{
			FVector Normal;
			if (!(Tokenizer >> Normal.X >> Normal.Y >> Normal.Z))
			{
				UE_LOG_ERROR("정점 법선 형식이 잘못되었습니다");
				return false;
			}

			OutObjInfo->NormalList.emplace_back(Normal);
		}
		/** Texture Coordinate */
		else if (Prefix == "vt")
		{
			/** @note: Ignore 3D Texture */
			FVector2 TexCoord;
			if (!(Tokenizer >> TexCoord.X >> TexCoord.Y))
			{
				UE_LOG_ERROR("정점 텍스쳐 좌표 형식이 잘못되었습니다");
				return false;
			}

			if (Config.bUVToUEBasis)
			{
				OutObjInfo->TexCoordList.emplace_back(UVToUEBasis(TexCoord));
			}
			else
			{
				OutObjInfo->TexCoordList.emplace_back(TexCoord);
			}
		}

		// =========================== Group Information ============================ //

		/** Object Information */
		else if (Prefix == "o")
		{
			if (!Config.bIsObjectEnabled)
			{
				continue; // Ignore 'o' prefix
			}

			if (OptObjectInfo)
			{
				OutObjInfo->ObjectInfoList.emplace_back(std::move(*OptObjectInfo));
			}

			FString ObjectName;
			if (!(Tokenizer >> ObjectName))
			{
				UE_LOG_ERROR("오브젝트 이름 형식이 잘못되었습니다");
				return false;
			}
			OptObjectInfo.emplace();
			OptObjectInfo->Name = std::move(ObjectName);

			FaceCount = 0;
		}

		/** Group Information */
		else if (Prefix == "g")
		{
			if (!OptObjectInfo)
			{
				OptObjectInfo.emplace();
				OptObjectInfo->Name = Config.DefaultName;
			}

			FString GroupName;
			if (!(Tokenizer >> GroupName))
			{
				UE_LOG_ERROR("잘못된 그룹 이름 형식입니다");
				return false;
			}

			OptObjectInfo->GroupNameList.emplace_back(std::move(GroupName));
			OptObjectInfo->GroupIndexList.emplace_back(FaceCount);
		}

		// ============================ Face Information ============================ //

		/** Face Information */
		else if (Prefix == "f")
		{
			if (!OptObjectInfo)
			{
				OptObjectInfo.emplace();
				OptObjectInfo->Name = Config.DefaultName;
			}

			TArray<FString> FaceBuffers;
			FString FaceBuffer;
			while (Tokenizer >> FaceBuffer)
			{
				FaceBuffers.emplace_back(FaceBuffer);
			}

			if (FaceBuffers.size() < 2)
			{
				UE_LOG_ERROR("면 형식이 잘못되었습니다");
				return false;
			}

			/** @todo: 오목 다각형에 대한 지원 필요, 현재는 볼록 다각형만 지원 */
			for (size_t i = 1; i + 1 < FaceBuffers.size(); ++i)
			{
				if (Config.bFlipWindingOrder)
				{
					if (!ParseFaceBuffer(FaceBuffers[0], &(*OptObjectInfo)))
					{
						UE_LOG_ERROR("면 파싱에 실패했습니다");
						return false;
					}

					if (!ParseFaceBuffer(FaceBuffers[i + 1], &(*OptObjectInfo)))
					{
						UE_LOG_ERROR("면 파싱에 실패했습니다");
						return false;
					}

					if (!ParseFaceBuffer(FaceBuffers[i], &(*OptObjectInfo)))
					{
						UE_LOG_ERROR("면 파싱에 실패했습니다");
						return false;
					}
				}
				else
				{
					if (!ParseFaceBuffer(FaceBuffers[0], &(*OptObjectInfo)))
					{
						UE_LOG_ERROR("면 파싱에 실패했습니다");
						return false;
					}

					if (!ParseFaceBuffer(FaceBuffers[i], &(*OptObjectInfo)))
					{
						UE_LOG_ERROR("면 파싱에 실패했습니다");
						return false;
					}

					if (!ParseFaceBuffer(FaceBuffers[i + 1], &(*OptObjectInfo)))
					{
						UE_LOG_ERROR("면 파싱에 실패했습니다");
						return false;
					}
				}
				++FaceCount;
			}
		}

		// ============================ Material Information ============================ //

		else if (Prefix == "mtllib")
		{
			/** @todo: Parse material data */
			/** @todo: Support relative path from .obj file to find .mtl file */
			FString MaterialFileName;
			Tokenizer >> MaterialFileName;

			std::filesystem::path MaterialFilePath = FilePath.parent_path() / MaterialFileName;

			MaterialFilePath = std::filesystem::weakly_canonical(MaterialFilePath);

			if (!LoadMaterial(MaterialFilePath, OutObjInfo))
			{
				UE_LOG_ERROR("머티리얼을 불러오는데 실패했습니다: %s", MaterialFilePath.string().c_str());
				return false;
			}
		}

		else if (Prefix == "usemtl")
		{
			FString MaterialName;
			Tokenizer >> MaterialName;

			if (!OptObjectInfo)
			{
				OptObjectInfo.emplace();
				OptObjectInfo->Name = Config.DefaultName;
			}

			OptObjectInfo->MaterialNameList.emplace_back(std::move(MaterialName));
			OptObjectInfo->MaterialIndexList.emplace_back(FaceCount);
		}
	}

	if (OptObjectInfo)
	{
		OutObjInfo->ObjectInfoList.emplace_back(std::move(*OptObjectInfo));
	}

	if (Config.bIsBinaryEnabled)
	{
		FWindowsBinWriter WindowsBinWriter(BinFilePath);
		WindowsBinWriter << *OutObjInfo;
	}

	return true;
}

bool FObjImporter::LoadMaterial(const std::filesystem::path& FilePath, FObjInfo* OutObjInfo)
{
	if (!OutObjInfo)
	{
		return false;
	}

	if (!std::filesystem::exists(FilePath))
	{
		UE_LOG_ERROR("파일을 찾지 못했습니다: %s", FilePath.string().c_str());
		return false;
	}

	if (FilePath.extension() != ".mtl")
	{
		UE_LOG_ERROR("잘못된 파일 확장자입니다: %s", FilePath.string().c_str());
		return false;
	}

	std::ifstream File(FilePath);
	if (!File)
	{
		UE_LOG_ERROR("파일을 열지 못했습니다: %s", FilePath.string().c_str());
		return false;
	}

	TOptional<FObjectMaterialInfo> OptMaterialInfo;

	FString Buffer;
	while (std::getline(File, Buffer))
	{
		std::istringstream Tokenizer(Buffer);
		FString Prefix;

		Tokenizer >> Prefix;

		if (Prefix == "newmtl")
		{
			if (OptMaterialInfo)
			{
				OutObjInfo->ObjectMaterialInfoList.emplace_back(std::move(*OptMaterialInfo));
			}

			OptMaterialInfo.emplace();
			if (!(Tokenizer >> OptMaterialInfo->Name))
			{
				UE_LOG_ERROR("머티리얼 이름 형식이 잘못되었습니다");
				return false;
			}
		}
		else if (Prefix == "Ns")
		{
			if (!OptMaterialInfo)
			{
				UE_LOG_ERROR("머티리얼이 정의되지 않았습니다");
				return false;
			}

			if (!(Tokenizer >> OptMaterialInfo->Ns))
			{
				UE_LOG_ERROR("Ns(광택) 속성 형식이 잘못되었습니다");
				return false;
			}
		}
		else if (Prefix == "Ka")
		{
			if (!OptMaterialInfo)
			{
				UE_LOG_ERROR("머티리얼이 정의되지 않았습니다");
				return false;
			}

			if (!(Tokenizer >> OptMaterialInfo->Ka.X >> OptMaterialInfo->Ka.Y >> OptMaterialInfo->Ka.Z))
			{
				UE_LOG_ERROR("Ka(주변) 속성 형식이 잘못되었습니다");
				return false;
			}
		}
		else if (Prefix == "Kd")
		{
			if (!OptMaterialInfo)
			{
				UE_LOG_ERROR("머티리얼이 정의되지 않았습니다");
				return false;
			}

			if (!(Tokenizer >> OptMaterialInfo->Kd.X >> OptMaterialInfo->Kd.Y >> OptMaterialInfo->Kd.Z))
			{
				UE_LOG_ERROR("Kd(분산) 속성 형식이 잘못되었습니다");
				return false;
			}
		}
		else if (Prefix == "Ks")
		{
			if (!OptMaterialInfo)
			{
				UE_LOG_ERROR("머티리얼이 정의되지 않았습니다");
				return false;
			}

			if (!(Tokenizer >> OptMaterialInfo->Ks.X >> OptMaterialInfo->Ks.Y >> OptMaterialInfo->Ks.Z))
			{
				UE_LOG_ERROR("Ks(반사) 속성 형식이 잘못되었습니다");
				return false;
			}
		}
		else if (Prefix == "Ke")
		{
			// Ke is not in FObjectMaterialInfo, skipping
		}
		else if (Prefix == "Ni")
		{
			if (!OptMaterialInfo)
			{
				UE_LOG_ERROR("머티리얼이 정의되지 않았습니다");
				return false;
			}

			if (!(Tokenizer >> OptMaterialInfo->Ni))
			{
				UE_LOG_ERROR("Ni(굴절률) 속성 형식이 잘못되었습니다");
				return false;
			}
		}
		else if (Prefix == "d")
		{
			if (!OptMaterialInfo)
			{
				UE_LOG_ERROR("머티리얼이 정의되지 않았습니다");
				return false;
			}

			if (!(Tokenizer >> OptMaterialInfo->D))
			{
				UE_LOG_ERROR("d(투명도) 속성 형식이 잘못되었습니다");
				return false;
			}
		}
		else if (Prefix == "Tr")
		{
			if (!OptMaterialInfo)
			{
				UE_LOG_ERROR("머티리얼이 정의되지 않았습니다");
				return false;
			}
			float Tr;
			if (!(Tokenizer >> Tr))
			{
				UE_LOG_ERROR("Tr(투명도) 속성 형식이 잘못되었습니다");
				return false;
			}
			OptMaterialInfo->D = 1.0f - Tr;
		}
		else if (Prefix == "Tf")
		{
			// Tf is not in FObjectMaterialInfo, skipping
		}
		else if (Prefix == "illum")
		{
			if (!OptMaterialInfo)
			{
				UE_LOG_ERROR("머티리얼이 정의되지 않았습니다");
				return false;
			}

			if (!(Tokenizer >> OptMaterialInfo->Illumination))
			{
				UE_LOG_ERROR("illum(조명 모델) 속성 형식이 잘못되었습니다");
				return false;
			}
		}
		else if (Prefix == "map_Ka")
		{
			if (!OptMaterialInfo)
			{
				UE_LOG_ERROR("머티리얼이 정의되지 않았습니다");
				return false;
			}

			if (!(Tokenizer >> OptMaterialInfo->KaMap))
			{
				UE_LOG_ERROR("map_Ka(주변 텍스처) 속성 형식이 잘못되었습니다");
				return false;
			}
		}
		else if (Prefix == "map_Kd")
		{
			if (!OptMaterialInfo)
			{
				UE_LOG_ERROR("머티리얼이 정의되지 않았습니다");
				return false;
			}

			if (!(Tokenizer >> OptMaterialInfo->KdMap))
			{
				UE_LOG_ERROR("map_Kd(분산 텍스처) 속성 형식이 잘못되었습니다");
				return false;
			}
		}
		else if (Prefix == "map_Ks")
		{
			if (!OptMaterialInfo)
			{
				UE_LOG_ERROR("머티리얼이 정의되지 않았습니다");
				return false;
			}

			if (!(Tokenizer >> OptMaterialInfo->KsMap))
			{
				UE_LOG_ERROR("map_Ks(반사 텍스처) 속성 형식이 잘못되었습니다");
				return false;
			}
		}
		else if (Prefix == "map_Ns")
		{
			if (!OptMaterialInfo)
			{
				UE_LOG_ERROR("머티리얼이 정의되지 않았습니다");
				return false;
			}

			if (!(Tokenizer >> OptMaterialInfo->NsMap))
			{
				UE_LOG_ERROR("map_Ns(광택 텍스처) 속성 형식이 잘못되었습니다");
				return false;
			}
		}
		else if (Prefix == "map_d")
		{
			if (!OptMaterialInfo)
			{
				UE_LOG_ERROR("머티리얼이 정의되지 않았습니다");
				return false;
			}

			if (!(Tokenizer >> OptMaterialInfo->DMap))
			{
				UE_LOG_ERROR("map_d(투명도 텍스처) 속성 형식이 잘못되었습니다");
				return false;
			}
		}
		else if (Prefix == "map_bump" || Prefix == "bump")
		{
			if (!OptMaterialInfo)
			{
				UE_LOG_ERROR("머티리얼이 정의되지 않았습니다");
				return false;
			}

			if (!(Tokenizer >> OptMaterialInfo->BumpMap))
			{
				UE_LOG_ERROR("map_bump(범프 텍스처) 속성 형식이 잘못되었습니다");
				return false;
			}
		}
	}

	if (OptMaterialInfo)
	{
		OutObjInfo->ObjectMaterialInfoList.emplace_back(std::move(*OptMaterialInfo));
	}

	return true;
}

bool FObjImporter::ParseFaceBuffer(const FString& FaceBuffer, FObjectInfo* OutObjectInfo)
{
	/** Ignore data when ObjInfo is nullptr */
	if (!OutObjectInfo)
	{
		return false;
	}

	TArray<FString> IndexBuffers;
	std::istringstream Tokenizer(FaceBuffer);
	FString IndexBuffer;
	while (std::getline(Tokenizer, IndexBuffer, '/'))
	{
		IndexBuffers.emplace_back(IndexBuffer);
	}

	if (IndexBuffers.empty())
	{
		UE_LOG_ERROR("면 형식이 잘못되었습니다");
		return false;
	}

	if (IndexBuffers[0].empty())
	{
		UE_LOG_ERROR("정점 위치 형식이 잘못되었습니다");
		return false;
	}

	try
	{
		OutObjectInfo->VertexIndexList.push_back(std::stoull(IndexBuffers[0]) - 1);
	}
	catch ([[maybe_unused]] const std::invalid_argument& Exception)
	{
		UE_LOG_ERROR("정점 위치 인덱스 형식이 잘못되었습니다");
		return false;
	}

	switch (IndexBuffers.size())
	{
	case 1:
		/** @brief: Only position data (e.g., 'f 1 2 3') */
		break;
	case 2:
		/** @brief: Position and texture coordinate data (e.g., 'f 1/1 2/1') */
		if (IndexBuffers[1].empty())
		{
			UE_LOG_ERROR("정점 텍스쳐 좌표 인덱스 형식이 잘못되었습니다");
			return false;
		}

		try
		{
			OutObjectInfo->TexCoordIndexList.push_back(std::stoull(IndexBuffers[1]) - 1);
		}
		catch ([[maybe_unused]] const std::invalid_argument& Exception)
		{
			UE_LOG_ERROR("정점 텍스쳐 좌표 인덱스 형식이 잘못되었습니다");
			return false;
		}
		break;
	case 3:
		/** @brief: Position, texture coordinate and vertex normal data (e.g., 'f 1/1/1 2/2/1' or 'f 1//1 2//1') */
		if (IndexBuffers[1].empty()) /** Position and vertex normal */
		{
			if (IndexBuffers[2].empty())
			{
				UE_LOG_ERROR("정점 법선 인덱스 형식이 잘못되었습니다");
				return false;
			}

			try
			{
				OutObjectInfo->NormalIndexList.push_back(std::stoull(IndexBuffers[2]) - 1);
			}
			catch ([[maybe_unused]] const std::invalid_argument& Exception)
			{
				UE_LOG_ERROR("정점 법선 인덱스 형식이 잘못되었습니다");
				return false;
			}
		}
		else /** Position, texture coordinate, and vertex normal */
		{
			if (IndexBuffers[1].empty() || IndexBuffers[2].empty())
			{
				UE_LOG_ERROR("정점 텍스쳐 좌표 또는 법선 인덱스 형식이 잘못되었습니다");
				return false;
			}

			try
			{
				OutObjectInfo->TexCoordIndexList.push_back(std::stoull(IndexBuffers[1]) - 1);
				OutObjectInfo->NormalIndexList.push_back(std::stoull(IndexBuffers[2]) - 1);
			}
			catch ([[maybe_unused]] const std::invalid_argument& Exception)
			{
				UE_LOG_ERROR("정점 텍스쳐 좌표 또는 법선 인덱스 형식이 잘못되었습니다");
				return false;
			}
		}
		break;
	}

	return true;
}
