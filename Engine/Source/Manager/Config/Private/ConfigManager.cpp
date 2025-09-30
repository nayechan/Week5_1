#include "pch.h"
#include "Manager/Config/Public/ConfigManager.h"
#include "Core/Public/Class.h"
#include "Editor/Public/Camera.h"
#include "Utility/Public/JsonSerializer.h"

#include <json.hpp>

IMPLEMENT_SINGLETON_CLASS_BASE(UConfigManager)

UConfigManager::UConfigManager()
	: EditorIniFileName("editor.ini")
	, bLODEnabled(true)
	, LODDistance0(10.0f)
	, LODDistance1(80.0f)
{
	LoadEditorSetting();
}

UConfigManager::~UConfigManager()
{
	SaveEditorSetting();
}

void UConfigManager::LoadEditorSetting()
{
	const FString& FileNameStr = EditorIniFileName.ToString();
	std::ifstream Ifs(FileNameStr);
	if (!Ifs.is_open())
	{
		CellSize = 1.0f;
		CameraSensitivity = UCamera::DEFAULT_SPEED;
		return; // 파일이 없으면 기본값 유지
	}

	FString Line;
	while (std::getline(Ifs, Line))
	{
		size_t DelimiterPos = Line.find('=');
		if (DelimiterPos == FString::npos) continue;

		FString Key = Line.substr(0, DelimiterPos);
		FString Value = Line.substr(DelimiterPos + 1);

		try
		{
			if (Key == "CellSize") CellSize = std::stof(Value);
			else if (Key == "CameraSensitivity") CameraSensitivity = std::stof(Value);
			else if (Key == "RootSplitterRatio") RootSplitterRatio = std::stof(Value);
			else if (Key == "LeftSplitterRatio") LeftSplitterRatio = std::stof(Value);
			else if (Key == "RightSplitterRatio") RightSplitterRatio = std::stof(Value);
			else if (Key == "LastUsedLevelPath") LastUsedLevelPath = Value;
			else if (Key == "LODEnabled") bLODEnabled = (Value == "true" || Value == "1");
			else if (Key == "LODDistance0") LODDistance0 = std::stof(Value);
			else if (Key == "LODDistance1") LODDistance1 = std::stof(Value);
		}
		catch (const std::exception&) {}
	}
}

void UConfigManager::SaveEditorSetting()
{
	std::ofstream Ofs(EditorIniFileName.ToString());
	if (Ofs.is_open())
	{
		Ofs << "CellSize=" << CellSize << "\n";
		Ofs << "CameraSensitivity=" << CameraSensitivity << "\n";
		Ofs << "RootSplitterRatio=" << RootSplitterRatio << "\n";
		Ofs << "LeftSplitterRatio=" << LeftSplitterRatio << "\n";
		Ofs << "RightSplitterRatio=" << RightSplitterRatio << "\n";
		Ofs << "LastUsedLevelPath=" << LastUsedLevelPath << "\n";
		Ofs << "\n";
		Ofs << "; Rendering Settings\n";
		Ofs << "LODEnabled=" << (bLODEnabled ? "true" : "false") << "\n";
		Ofs << "LODDistance0=" << LODDistance0 << "\n";
		Ofs << "LODDistance1=" << LODDistance1 << "\n";
	}
}

JSON UConfigManager::GetCameraSettingsAsJson()
{
	/* *
	* @brief 현재 주석처리된 코드는 Week04 기준으로 작성된 코드입니다. [PYB]
	*/
	//  JSON RootJson = json::Object();
	//
	//for (int32 Index = 0; Index < 4; ++Index)
	//{
	//	const auto& Data = ViewportCameraSettings[Index];
	//	JSON ViewportJson = json::Object();
	//
	//	ViewportJson["CameraType"] = static_cast<int>(Data.ViewportCameraType);
	//
	//	// FJsonSerializer 유틸리티 함수를 사용하여 FVector를 JSON 배열로 변환
	//	ViewportJson["Location"] = FJsonSerializer::VectorToJson(Data.Location);
	//	ViewportJson["Rotation"] = FJsonSerializer::VectorToJson(Data.Rotation);
	//	ViewportJson["FocusLocation"] = FJsonSerializer::VectorToJson(Data.FocusLocation);
	//
	//	ViewportJson["FarClip"] = Data.FarClip;
	//	ViewportJson["NearClip"] = Data.NearClip;
	//	ViewportJson["FovY"] = Data.FovY;
	//	ViewportJson["OrthoWidth"] = Data.OrthoWidth;
	//
	//	// Index를 FString 키로 변환 ("0", "1", "2", "3")
	//	FString Key = std::to_string(Index);
	//	RootJson[Key] = ViewportJson;
	//}

	// 현재 이 코드는 Week05 전용으로 사용되는 코드입니다.
	JSON RootJson = json::Object();

	const auto& Data = ViewportCameraSettings[0];

	// FJsonSerializer 유틸리티 함수를 사용하여 FVector를 JSON 배열로 변환
	RootJson["Location"] = FJsonSerializer::VectorToJson(Data.Location);
	RootJson["Rotation"] = FJsonSerializer::VectorToJson(Data.Rotation);
	RootJson["FarClip"] = FJsonSerializer::FloatToArrayJson(Data.FarClip);
	RootJson["NearClip"] = FJsonSerializer::FloatToArrayJson(Data.NearClip);
	RootJson["FOV"] = FJsonSerializer::FloatToArrayJson(Data.FovY);

	return RootJson;
}

void UConfigManager::SetCameraSettingsFromJson(const JSON& InData)
{
	if (InData.JSONType() != JSON::Class::Object)
	{
		return;
	}

	// 주석 처리된 ViewportCameraSettings[4]는 Week04 기준으로 작성된 코드입니다.
	//for (int32 Index = 0; Index < 4; ++Index)
	//{
	//	// Index를 FString 키로 변환하여 데이터를 찾음
	//	FString Key = std::to_string(Index);
	//	JSON ViewportJson;
	//
	//	// ReadObject 유틸리티 함수로 해당 뷰포트의 JSON 데이터를 안전하게 가져옴
	//	if (FJsonSerializer::ReadObject(InData, Key, ViewportJson))
	//	{
	//		// 유틸리티 함수를 사용하여 반복적인 검사 없이 간결하게 데이터 파싱
	//		// 실패 시 각 함수 내부에서 로그를 남기고 기본값을 할당함
	//		FJsonSerializer::ReadVector(ViewportJson, "Location", ViewportCameraSettings[Index].Location);
	//		FJsonSerializer::ReadVector(ViewportJson, "Rotation", ViewportCameraSettings[Index].Rotation);
	//		FJsonSerializer::ReadVector(ViewportJson, "FocusLocation", ViewportCameraSettings[Index].FocusLocation);
	//
	//		FJsonSerializer::ReadFloat(ViewportJson, "FarClip", ViewportCameraSettings[Index].FarClip);
	//		FJsonSerializer::ReadFloat(ViewportJson, "NearClip", ViewportCameraSettings[Index].NearClip);
	//		FJsonSerializer::ReadFloat(ViewportJson, "FovY", ViewportCameraSettings[Index].FovY);
	//		FJsonSerializer::ReadFloat(ViewportJson, "OrthoWidth", ViewportCameraSettings[Index].OrthoWidth);
	//
	//		// CameraType은 int로 읽은 후 Enum으로 캐스팅 (Enum은 동적 캐스팅이 안됨)
	//		int32 CameraTypeInt;
	//		if (FJsonSerializer::ReadInt32(ViewportJson, "CameraType", CameraTypeInt))
	//		{
	//			ViewportCameraSettings[Index].ViewportCameraType = ToClientCameraType(CameraTypeInt);
	//		}
	//	}
	//}

	for (int32 Index = 0; Index < 4; ++Index)
	{
		// ReadObject 유틸리티 함수로 해당 뷰포트의 JSON 데이터를 안전하게 가져옴
		// 유틸리티 함수를 사용하여 반복적인 검사 없이 간결하게 데이터 파싱
		// 실패 시 각 함수 내부에서 로그를 남기고 기본값을 할당함
		FJsonSerializer::ReadArrayFloat(InData, "FOV", ViewportCameraSettings[Index].FovY);
		FJsonSerializer::ReadArrayFloat(InData, "FarClip", ViewportCameraSettings[Index].FarClip);
		FJsonSerializer::ReadVector(InData, "Location", ViewportCameraSettings[Index].Location);
		FJsonSerializer::ReadArrayFloat(InData, "NearClip", ViewportCameraSettings[Index].NearClip);
		FJsonSerializer::ReadVector(InData, "Rotation", ViewportCameraSettings[Index].Rotation);
	}
}

bool UConfigManager::GetConfigValueBool(const FString& Key, bool DefaultValue)
{
	if (Key == "LODEnabled")
		return bLODEnabled;

	return DefaultValue;
}

float UConfigManager::GetConfigValueFloat(const FString& Key, float DefaultValue)
{
	if (Key == "LODDistance0")
		return LODDistance0;
	else if (Key == "LODDistance1")
		return LODDistance1;

	return DefaultValue;
}
