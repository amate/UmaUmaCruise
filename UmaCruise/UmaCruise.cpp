// UmaCruise.cpp : main source file for UmaCruise.exe
//

#include "stdafx.h"

#include "MainDlg.h"

#include "Utility\CommonUtility.h"
#include "Utility\Logger.h"
#include "Utility\GdiplusUtil.h"
#include "Utility\WinHTTPWrapper.h"
#include "TesseractWrapper.h"

CAppModule _Module;


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

	::SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED);

	int nRet = Run(lpstrCmdLine, nCmdShow);

	WinHTTPWrapper::TermWinHTTP();
	TesseractWrapper::TesseractTerm();
	GdiplusTerm();

	_Module.Term();
	::CoUninitialize();

	return nRet;
}
