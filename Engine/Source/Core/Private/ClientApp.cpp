#include "pch.h"
#include "Core/Public/ClientApp.h"

#include "Editor/Public/Editor.h"
#include "Core/Public/AppWindow.h"
#include "Manager/Input/Public/InputManager.h"
#include "Manager/Level/Public/LevelManager.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Manager/Time/Public/TimeManager.h"

#include "Manager/UI/Public/UIManager.h"
#include "Manager/Config/Public/ConfigManager.h"
#include "Render/Renderer/Public/Renderer.h"

#include "Render/UI/Window/Public/ConsoleWindow.h"
#include "Render/UI/Overlay/Public/StatOverlay.h"

#ifdef IS_OBJ_VIEWER
#include "Utility/Public/FileDialog.h"
#endif

FClientApp::FClientApp() = default;

FClientApp::~FClientApp() = default;
/**
 * @brief Client Main Runtime Function
 * App 초기화, Main Loop 실행을 통한 전체 Cycle
 *
 * @param InInstanceHandle Process Instance Handle
 * @param InCmdShow Window Display Method
 *
 *
 * @return Program Termination Code
 */
int FClientApp::Run(HINSTANCE InInstanceHandle, int InCmdShow)
{
	// Memory Leak Detection & Report
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetBreakAlloc(0);
#endif

	// Window Object Initialize
	Window = new FAppWindow(this);
	if (!Window->Init(InInstanceHandle, InCmdShow))
	{
		assert(!"Window Creation Failed");
		return 0;
	}

	// Create Console
	// #ifdef _DEBUG
	// 	Window->InitializeConsole();
	// #endif

	// Keyboard Accelerator Table Setting
	// AcceleratorTable = LoadAccelerators(InInstanceHandle, MAKEINTRESOURCE(IDC_CLIENT));

	// Initialize Base System
	int InitResult = InitializeSystem();
	if (InitResult != S_OK)
	{
		assert(!"Initialize Failed");
		return 0;
	}

	// Execute Main Loop
	MainLoop();

	// Termination Process
	ShutdownSystem();

	return static_cast<int>(MainMessage.wParam);
}

/**
 * @brief Initialize System For Game Execution
 */
int FClientApp::InitializeSystem() const
{
	// 현재 시간을 랜덤 시드로 설정
	srand(static_cast<unsigned int>(time(NULL)));

	// Initialize By Get Instance
	UTimeManager::GetInstance();
	UInputManager::GetInstance();

	auto& Renderer = URenderer::GetInstance();
	Renderer.Init(Window->GetWindowHandle());

	// StatOverlay Initialize
	auto& StatOverlay = UStatOverlay::GetInstance();
	StatOverlay.Initialize();

	// UIManager Initialize
	auto& UIManger = UUIManager::GetInstance();
	UIManger.Initialize(Window->GetWindowHandle());
	UUIWindowFactory::CreateDefaultUILayout();

	UAssetManager::GetInstance().Initialize();

	// Create Default Level
	FString LastSavedLevelPath = UConfigManager::GetInstance().GetLastSavedLevelPath();
	if (ULevelManager::GetInstance().LoadLevel(LastSavedLevelPath))
	{
		// 마지막을 저장한 레벨을 성공적으로 로드
	}
	else
	{
		ULevelManager::GetInstance().CreateNewLevel();
	}

	return S_OK;
}

/**
 * @brief Update System While Game Processing
 */
void FClientApp::UpdateSystem() const
{
	auto& TimeManager = UTimeManager::GetInstance();
	auto& InputManager = UInputManager::GetInstance();
	auto& UIManager = UUIManager::GetInstance();
	auto& Renderer = URenderer::GetInstance();
	auto& LevelManager = ULevelManager::GetInstance();

	LevelManager.Update();
	TimeManager.Update();
	InputManager.Update(Window);
	UIManager.Update();
	Renderer.Update();
}

/**
 * @brief Execute Main Message Loop
 * 윈도우 메시지 처리 및 게임 시스템 업데이트를 담당
 */
void FClientApp::MainLoop()
{
#ifdef IS_OBJ_VIEWER
	if (!OpenObjFromFileDialog())
	{
		MessageBoxW(
			nullptr,
			L"파일을 열 수 없습니다",
			L"오류",
			MB_OK | MB_ICONERROR
		);
		std::exit(1);
	}
#endif

	while (true)
	{
		// Async Message Process
		if (PeekMessage(&MainMessage, nullptr, 0, 0, PM_REMOVE))
		{
			// Process Termination
			if (MainMessage.message == WM_QUIT)
			{
				break;
			}
			// Shortcut Key Processing
			if (!TranslateAccelerator(MainMessage.hwnd, AcceleratorTable, &MainMessage))
			{
				TranslateMessage(&MainMessage);
				DispatchMessage(&MainMessage);
			}
		}
		// Game System Update
		else
		{
			UpdateSystem();
		}
	}
}

/**
 * @brief 시스템 종료 처리
 * 모든 리소스를 안전하게 해제하고 매니저들을 정리합니다.
 */
void FClientApp::ShutdownSystem() const
{
	UStatOverlay::GetInstance().Release();
	URenderer::GetInstance().Release();
	UUIManager::GetInstance().Shutdown();
	ULevelManager::GetInstance().Shutdown();
	UAssetManager::GetInstance().Release();

	// Release되지 않은 UObject의 메모리 할당 해제
	// 추후 GC가 처리할 것
	UClass::Shutdown();

	delete Window;
}
