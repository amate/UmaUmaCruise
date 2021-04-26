// MainDlg.h : interface of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <thread>
#include <atomic>

#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>
#include <atlcrack.h>
#include <atlstr.h>
#include <atlddx.h>

#include "UmaEventLibrary.h"
#include "RaceDateLibrary.h"
#include "UmaTextRecognizer.h"

#include "resource.h"

#include "aboutdlg.h"
#include "PreviewWindow.h"
#include "Config.h"


class CMainDlg : 
	public CDialogImpl<CMainDlg>, 
	public CUpdateUI<CMainDlg>,
	public CMessageFilter, 
	public CIdleHandler,
	public CWinDataExchange<CMainDlg>,
	public CCustomDraw<CMainDlg>
{
public:
	enum { IDD = IDD_MAINDLG };

	enum {
		kAutoOCRTimerID = 1,
		kAutoOCRTimerInterval = 1000,
	};

	virtual BOOL PreTranslateMessage(MSG* pMsg);

	virtual BOOL OnIdle()
	{
		UIUpdateChildWindows();
		return FALSE;
	}

	void	ChangeWindowTitle(const std::wstring& title);

	// overrides
	DWORD OnPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/);
	DWORD OnItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/);

	BEGIN_UPDATE_UI_MAP(CMainDlg)
	END_UPDATE_UI_MAP()

	BEGIN_DDX_MAP(CMainDlg)
		DDX_CONTROL_HANDLE(IDC_COMBO_UMAMUSUME, m_cmbUmaMusume)
		DDX_TEXT(IDC_EDIT_EVENTNAME, m_eventName)
		DDX_TEXT(IDC_EDIT_EVENT_SOURCE, m_eventSource)

		// Race
		DDX_TEXT(IDC_EDIT_NOWDATE, m_currentTurn)
		DDX_CHECK(IDC_CHECK_SHOWRACE_AFTERCURRENTDATE, m_showRaceAfterCurrentDate)

		DDX_CHECK(IDC_CHECK_G1, m_gradeG1)
		DDX_CHECK(IDC_CHECK_G2, m_gradeG2)
		DDX_CHECK(IDC_CHECK_G3, m_gradeG3)

		DDX_CHECK(IDC_CHECK_SPRINT, m_sprint)
		DDX_CHECK(IDC_CHECK_MILE, m_mile)
		DDX_CHECK(IDC_CHECK_MIDDLE, m_middle)
		DDX_CHECK(IDC_CHECK_LONG, m_long)

		DDX_CHECK(IDC_CHECK_GRASS, m_grass)
		DDX_CHECK(IDC_CHECK_DART, m_dart)

		DDX_CHECK(IDC_CHECK_RIGHT, m_right)
		DDX_CHECK(IDC_CHECK_LEFT, m_left)
		DDX_CHECK(IDC_CHECK_LINE, m_line)

		DDX_CONTROL_HANDLE(IDC_LIST_RACE, m_raceListView)

	END_DDX_MAP()


	BEGIN_MSG_MAP_EX(CMainDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
		COMMAND_ID_HANDLER_EX(IDC_BUTTON_SHOWHIDE_RACELIST, OnShowHideRaceList)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER_EX(IDC_BUTTON_CONFIG, OnShowConfigDlg)
		COMMAND_ID_HANDLER_EX(IDC_BUTTON_PREVIEW, OnShowPreviewWindow)
		MSG_WM_TIMER(OnTimer)
		COMMAND_HANDLER_EX(IDC_COMBO_UMAMUSUME, CBN_SELCHANGE, OnSelChangeUmaMusume)

		MSG_WM_CTLCOLORDLG(OnCtlColorDlg)
		MSG_WM_CTLCOLORSTATIC(OnCtlColorDlg)
		MSG_WM_CTLCOLORBTN(OnCtlColorDlg)
		MSG_WM_CTLCOLOREDIT(OnCtlColorDlg)

		COMMAND_ID_HANDLER_EX(IDC_BUTTON_SCREENSHOT, OnScreenShot)
		COMMAND_ID_HANDLER_EX(IDC_CHECK_START, OnStart)

		COMMAND_HANDLER_EX(IDC_EDIT_EVENTNAME, EN_CHANGE, OnEventNameChanged)

		COMMAND_ID_HANDLER_EX(IDC_BUTTON_REVISION, OnEventRevision)
		
		// Race List
		COMMAND_ID_HANDLER_EX(IDC_CHECK_SHOWRACE_AFTERCURRENTDATE, OnShowRaceAfterCurrentDate)
		COMMAND_RANGE_HANDLER_EX(IDC_CHECK_G1, IDC_CHECK_LOCATION_OOI, OnRaceFilterChanged)
		
		CHAIN_MSG_MAP(CCustomDraw<CMainDlg>)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	void	OnShowHideRaceList(UINT uNotifyCode, int nID, CWindow wndCtl);

	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	void	OnShowConfigDlg(UINT uNotifyCode, int nID, CWindow wndCtl);
	void	OnShowPreviewWindow(UINT uNotifyCode, int nID, CWindow wndCtl);
	void	OnTimer(UINT_PTR nIDEvent);
	void	OnSelChangeUmaMusume(UINT uNotifyCode, int nID, CWindow wndCtl);

	HBRUSH OnCtlColorDlg(CDCHandle dc, CWindow wnd);

	void OnScreenShot(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnStart(UINT uNotifyCode, int nID, CWindow wndCtl);

	void OnEventNameChanged(UINT uNotifyCode, int nID, CWindow wndCtl);

	void OnEventRevision(UINT uNotifyCode, int nID, CWindow wndCtl);

	void OnShowRaceAfterCurrentDate(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnRaceFilterChanged(UINT uNotifyCode, int nID, CWindow wndCtl);

private:
	void	_UpdateEventOptions(const UmaEventLibrary::UmaEvent& umaEvent);
	int32_t	_GetRaceMatchState();
	void	_SetRaceMatchState(int32_t state);
	void	_UpdateRaceList(const std::wstring& turn);

	Config	m_config;
	bool	m_bShowRaceList = true;

	UmaEventLibrary	m_umaEventLibrary;
	RaceDateLibrary	m_raceDateLibrary;
	UmaTextRecognizer	m_umaTextRecoginzer;

	PreviewWindow	m_previewWindow;

	CString	m_targetWindowName;
	CString m_targetClassName;

	CComboBox	m_cmbUmaMusume;
	COLORREF	m_optionBkColor[3];
	CBrush	m_brsOptions[3];

	CString	m_eventName;
	CString	m_eventSource;
	CRect	m_rcBounds;

	// Race
	CString	m_currentTurn;

	bool	m_showRaceAfterCurrentDate = true;
	
	bool	m_gradeG1 = true;
	bool	m_gradeG2 = true;
	bool	m_gradeG3 = true;

	bool	m_sprint = true;
	bool	m_mile = true;
	bool	m_middle = true;
	bool	m_long = true;

	bool	m_grass = true;
	bool	m_dart = true;

	bool	m_right = true;
	bool	m_left = true;
	bool	m_line = true;

	bool	m_raceLocation[RaceDateLibrary::Race::Location::kMaxLocationCount];

	CListViewCtrl	m_raceListView;

	std::thread	m_threadAutoDetect;
	std::atomic_bool	m_cancelAutoDetect;
};
