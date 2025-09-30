#include "pch.h"
#include "Core/Public/Name.h"
#include <algorithm> // for std::transform
#include <cctype>    // for std::tolower

// '최초 사용 시 생성(Construct on First Use)' 기법을 적용하기 위한 헬퍼 함수들입니다.
// 익명 네임스페이스를 사용하여 이 파일 외부에서는 접근할 수 없도록 합니다.
namespace
{
	// DisplayNames 배열에 대한 접근자
	TArray<FString>& GetDisplayNames()
	{
		// 이 함수가 최초로 호출될 때 단 한 번만 안전하게 초기화됩니다.
		static TArray<FString> DisplayNames = { "None" };
		return DisplayNames;
	}

	// NameMap에 대한 접근자
	TMap<FString, uint32>& GetNameMap()
	{
		static TMap<FString, uint32> NameMap = { {"none", 0} };
		return NameMap;
	}

	// NextIndex 카운터에 대한 접근자
	uint32& GetNextIndex()
	{
		static uint32 NextIndex = 1;
		return NextIndex;
	}
}

/**
 * @brief 'None' 값을 나타내는 정적 FName 객체를 반환합니다.
 */
const FName& FName::GetNone()
{
	static const FName NoneInstance(0);
	return NoneInstance;
}

/**
 * @brief FName 생성자
 * @param InString FString 타입의 문자열
 */
FName::FName(const FString& InString)
{
	// 비교는 Lower로 진행
	FString LowerString = InString;
	std::transform(LowerString.begin(), LowerString.end(), LowerString.begin(),
		[](unsigned char InChar) { return std::tolower(InChar); });

	// 헬퍼 함수를 통해 안전하게 NameMap에 접근합니다.
	auto& NameMapRef = GetNameMap();
	auto FindResult = NameMapRef.find(LowerString);

	// 동일 이름이 존재하지 않는 경우
	if (FindResult == NameMapRef.end())
	{
		// 헬퍼 함수를 통해 안전하게 정적 데이터들을 수정합니다.
		auto& NextIndexRef = GetNextIndex();
		NameMapRef.insert({ LowerString, NextIndexRef });
		GetDisplayNames().push_back(InString);

		// 인덱스 제공
		ComparisonIndex = NextIndexRef++;
		DisplayIndex = ComparisonIndex;
	}
	// 이미 존재하는 경우
	else
	{
		ComparisonIndex = FindResult->second;
		DisplayIndex = ComparisonIndex;
	}
}

FName::FName() : FName(FString(""))
{
}

/**
 * @brief c-style 문자열을 인자로 받는 FName 생성자
 * @param InStringPtr c-style 문자열
 */
FName::FName(const char* InStringPtr)
	: FName(FString(InStringPtr))
{
}

/**
 * @brief 특정 인덱스로 FName을 생성하는 private 생성자
 */
FName::FName(int32 InComparisonIndex)
{
	this->ComparisonIndex = InComparisonIndex;
	this->DisplayIndex = this->ComparisonIndex;
}


/**
 * @brief 두 FName을 비교하는 멤버 함수
 * @param InOther 다른 FName
 * @return 둘 사이의 인덱스 거리 차이
 */
int32 FName::Compare(const FName& InOther) const
{
	return this->ComparisonIndex - InOther.ComparisonIndex;
}

/**
 * FName 비교 연산자
 * @param InOther 비교할 다른 FName
 * @return 같은지 여부
 */
bool FName::operator==(const FName& InOther) const
{
	return this->Compare(InOther) == 0;
}

/**
 * FName 비교 연산자 (부등호)
 * @param InOther 비교할 다른 FName
 * @return 다른지 여부
 */
bool FName::operator!=(const FName& InOther) const
{
	return !(*this == InOther);
}

/**
 * @brief 사용자가 제공한 이름을 반환하는 멤버 함수
 * @return DisplayName
 */
const FString& FName::ToString() const
{
	// 헬퍼 함수를 통해 안전하게 DisplayNames에 접근합니다.
	return GetDisplayNames()[DisplayIndex];
}

/**
 * @brief Display 이름을 변경하는 함수
 * @param InDisplayName 변경할 새로운 이름
 */
void FName::SetDisplayName(const FString& InDisplayName) const
{
	GetDisplayNames()[this->DisplayIndex] = InDisplayName;
}
