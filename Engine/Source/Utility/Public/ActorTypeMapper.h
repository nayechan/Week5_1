#pragma once
#include "Global/Types.h"

class UClass;

// 과제에 주어진 Scene과 정확한 매핑을 위해 문자열 타입과 Actor의 UClass 타입을 변환하는 헬퍼 클래스
struct FActorTypeMapper
{
public:
	/**
	 * @brief 주어진 액터 클래스에 해당하는 문자열 타입을 반환합니다.
	 * @param InClass 변환할 액터의 UClass 객체
	 * @return 직렬화에 사용될 문자열 타입
	 */
	static FString ActorToType(UClass* InClass);

	/**
	 * @brief 주어진 문자열 타입에 해당하는 액터 클래스를 반환합니다.
	 * @param InTypeString 직렬화된 데이터에서 읽어온 문자열 타입
	 * @return 생성할 액터의 UClass 객체
	 */
	static UClass* TypeToActor(const FString& InTypeString);

private:
	// 외부에서 인스턴스화 방지
	FActorTypeMapper() = default;
};
