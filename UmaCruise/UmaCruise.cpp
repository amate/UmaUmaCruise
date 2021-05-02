// UmaCruise.cpp : main source file for UmaCruise.exe
//

#include "stdafx.h"

#include "MainDlg.h"

#include "Utility\CommonUtility.h"
#include "Utility\Logger.h"
#include "Utility\GdiplusUtil.h"
#include "Utility\WinHTTPWrapper.h"
#include "TesseractWrapper.h"

// グローバル変数
CAppModule _Module;

using SetThreadDpiAwarenessContextFunc = DPI_AWARENESS_CONTEXT(*)(DPI_AWARENESS_CONTEXT);
SetThreadDpiAwarenessContextFunc	g_funcSetThreadDpiAwarenessContext = nullptr;


std::string	LogFileName()	// for boost::log
{
	auto logPath = GetExeDirectory() / L"info.log";
	return logPath.string();
}

int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	CMainDlg dlgMain;

	if(dlgMain.Create(NULL) == NULL)
	{
		ATLTRACE(_T("Main dialog creation failed!\n"));
		return 0;
	}

	dlgMain.ShowWindow(nCmdShow);

	int nRet = theLoop.Run();

	_Module.RemoveMessageLoop();
	return nRet;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
	HRESULT hRes = ::CoInitialize(NULL);
	ATLASSERT(SUCCEEDED(hRes));

	AtlInitCommonControls(ICC_BAR_CLASSES);	// add flags to support other controls

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	GdiplusInit();
	TesseractWrapper::TesseractInit();
	WinHTTPWrapper::InitWinHTTP();

	HMODULE hModUser32 = ::LoadLibraryW(L"User32.dll");
	ATLASSERT(hModUser32);
	g_funcSetThreadDpiAwarenessContext = (SetThreadDpiAwarenessContextFunc)::GetProcAddress(hModUser32, "SetThreadDpiAwarenessContext");
	if (g_funcSetThreadDpiAwarenessContext) {
		g_funcSetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED);
	}

	int nRet = Run(lpstrCmdLine, nCmdShow);

	g_funcSetThreadDpiAwarenessContext = nullptr;
	::FreeLibrary(hModUser32);
	hModUser32 = NULL;

	WinHTTPWrapper::TermWinHTTP();
	TesseractWrapper::TesseractTerm();
	GdiplusTerm();

	_Module.Term();
	::CoUninitialize();

	return nRet;
}
