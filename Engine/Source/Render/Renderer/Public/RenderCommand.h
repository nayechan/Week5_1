#pragma once
#include "Global/Types.h"

class UPrimitiveComponent;

/**
 * @brief 렌더링 커맨드를 표현하는 경량 구조체
 *
 * 컬링 후 가시적인 Primitive들을 수집하고 정렬하기 위한 구조체입니다.
 * Material/Shader 기준으로 정렬하여 상태 변경을 최소화합니다.
 */
struct FRenderCommand
{
	UPrimitiveComponent* Primitive;  // 8바이트
	uint64 SortKey;                  // 8바이트
	// 총 16바이트 (캐시 친화적)

	/**
	 * @brief Primitive로부터 정렬 키를 생성합니다.
	 *
	 * SortKey 비트 레이아웃:
	 * [63:48] Shader Type (16비트) - 같은 셰이더끼리 그룹화
	 * [47:32] Material ID (16비트) - 같은 Material끼리 그룹화
	 * [31:16] Texture Hash (16비트) - 같은 텍스처 세트끼리 그룹화
	 * [15:0]  Reserved (16비트) - 투명 객체의 깊이 정렬용 예약
	 */
	static uint64 BuildSortKey(UPrimitiveComponent* Primitive);

	/**
	 * @brief SortKey에서 Shader Type을 추출합니다.
	 */
	static uint16 GetShaderType(uint64 SortKey) { return (SortKey >> 48) & 0xFFFF; }

	/**
	 * @brief SortKey에서 Material ID를 추출합니다.
	 */
	static uint16 GetMaterialID(uint64 SortKey) { return (SortKey >> 32) & 0xFFFF; }
};