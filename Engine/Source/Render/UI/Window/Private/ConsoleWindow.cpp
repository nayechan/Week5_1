#include "pch.h"
#include "Render/UI/Window/Public/ConsoleWindow.h"
#include "Render/UI/Widget/Public/ConsoleWidget.h"

IMPLEMENT_SINGLETON_CLASS(UConsoleWindow, UUIWindow)

UConsoleWindow::~UConsoleWindow()
{
	if (ConsoleWidget)
	{
		ConsoleWidget->CleanupSystemRedirect();
		ConsoleWidget->ClearLog();
	}
}

UConsoleWindow::UConsoleWindow()
	: ConsoleWidget(nullptr)
{
	// 콘솔 윈도우 기본 설정
	FUIWindowConfig Config;
	Config.WindowTitle = "GTL Console";
	Config.DefaultSize = ImVec2(1000, 260);
	Config.DefaultPosition = ImVec2(10, 770);
	Config.MinSize = ImVec2(1000, 260);
	Config.InitialState = EUIWindowState::Minimized; // 기본적으로 축소된 상태로 시작
	Config.bResizable = true;
	Config.bMovable = true;
	Config.bCollapsible = true;
	Config.DockDirection = EUIDockDirection::Bottom; // 바텀업 도킹 설정
	SetConfig(Config);

	ConsoleWidget = &UConsoleWidget::GetInstance();
	AddWidget(ConsoleWidget);
}

void UConsoleWindow::Initialize()
{
	// ConsoleWidget이 유효한지 확인
	if (!ConsoleWidget)
	{
		// 싱글톤 인스턴스를 다시 가져오기 시도
		try
		{
			ConsoleWidget = &UConsoleWidget::GetInstance();
		}
		catch (...)
		{
			// 싱글톤 인스턴스 가져오기 실패 시 에러 발생
			return;
		}
	}

	// Initialize System Output Redirection
	try
	{
		ConsoleWidget->InitializeSystemRedirect();
		AddLog(ELogType::Success, "ConsoleWindow: Game Console 초기화 성공");
		AddLog(ELogType::System, "ConsoleWindow: Logging System Ready");
	}
	catch (const std::exception& Exception)
	{
		// 초기화 실패 시 기본 로그만 출력 (예외를 다시 던지지 않음)
		if (ConsoleWidget)
		{
			ConsoleWidget->AddLog(ELogType::Error, "ConsoleWindow: System Redirection Failed: %s", Exception.what());
		}
	}
	catch (...)
	{
		if (ConsoleWidget)
		{
			ConsoleWidget->AddLog(ELogType::Error, "ConsoleWindow: System Redirection Failed: Unknown Error");
		}
	}
}

/**
 * @brief 외부에서 ConsoleWidget에 접근할 수 있도록 하는 래핑 함수
 */
void UConsoleWindow::AddLog(const char* fmt, ...) const
{
	if (ConsoleWidget)
	{
		char buf[1024];
		va_list args;
		va_start(args, fmt);
		(void)vsnprintf(buf, sizeof(buf), fmt, args);
		va_end(args);

		ConsoleWidget->AddLog(buf);
	}
}

void UConsoleWindow::AddLog(ELogType InType, const char* fmt, ...) const
{
	if (ConsoleWidget)
	{
		char buf[1024];
		va_list args;
		va_start(args, fmt);
		(void)vsnprintf(buf, sizeof(buf), fmt, args);
		va_end(args);

		ConsoleWidget->AddLog(InType, buf);
	}
}

void UConsoleWindow::AddSystemLog(const char* InText, bool bInIsError) const
{
	if (ConsoleWidget)
	{
		ConsoleWidget->AddSystemLog(InText, bInIsError);
	}
}
