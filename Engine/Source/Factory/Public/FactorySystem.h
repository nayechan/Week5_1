#pragma once

/**
 * @brief Factory 시스템 초기화 및 관리 클래스
 */
class FFactorySystem
{
public:
	static void Initialize();
	static void Shutdown();
	static void PrintAllFactories();

private:
	static bool bIsInitialized;
};
