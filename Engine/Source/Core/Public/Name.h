#pragma once

/**
 * @brief 오브젝트의 이름을 담당하는 구조체
 * 대소문자 관계 없는 비교 처리와 사용자가 직접 작성한 Display Name을 동시에 사용할 수 있음
 * @param DisplayIndex 표시용 이름 배열에 접근하기 위한 인덱스
 * @param ComparisonIndex 이름 비교를 위한 인덱스
 */
struct FName
{
public:
	int32 DisplayIndex;
	int32 ComparisonIndex;

	// 'None' 값에 접근하기 위한 정적 함수
	static const FName& GetNone();

	FName();
	FName(const char* InStringPtr);
	FName(const FString& InString);

	int32 Compare(const FName& InOther) const;
	bool operator==(const FName& InOther) const;
	bool operator!=(const FName& InOther) const;

	const FString& ToString() const;

	// 해시 함수를 위한 GetHash() 메서드
	size_t GetHash() const
	{
		return std::hash<int32>{}(ComparisonIndex);
	}

	// Display 이름 변경 함수
	void SetDisplayName(const FString& InDisplayName) const;

private:
	// 정적 멤버 변수 선언을 제거하고 구현 파일에서 관리하도록 변경합니다.

	// 특정 인덱스로 FName을 생성하는 private 생성자
	FName(int32 InComparisonIndex);
};

// std::hash에 대한 FName 특수화
namespace std
{
	template <>
	struct hash<FName>
	{
		size_t operator()(const FName& InName) const noexcept
		{
			return InName.GetHash();
		}
	};
}
