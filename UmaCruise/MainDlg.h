// MainDlg.h : interface of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <thread>
#include <atomic>

#include "UmaEventLibrary.h"
#include "RaceDateLibrary.h"
#include "SkillLibrary.h"
#include "UmaTextRecognizer.h"

#include "resource.h"

#include "aboutdlg.h"
#include "PreviewWindow.h"
#include "RaceListWindow.h"
#include "RichEditPopup.h"
#include "Config.h"

#include "DarkModeUI.h"


class CMainDlg : 
	public CDialogImpl<CMainDlg>, 
	public CUpdateUI<CMainDlg>,
	public CMessageFilter, 
	public CIdleHandler,
	public CWinDataExchange<CMainDlg>,
	public DarkModeUI<CMainDlg>
{
public:
	enum { IDD = IDD_MAINDLG };

	enum {
		kAutoOCRTimerID = 1,
		kAutoOCRTimerInterval = 1000,

		kMaxOptionEffect = 4,
		kMaxEffectTextLength = 1024,

		kWindowBottomMargin = 23,
		kGroupOptionMargin = 10,

		kPopupRichEditTopLeftMargin = -1,
		kPopupRichEditRightBottomMargin = 10,
	};

	CMainDlg();

	virtual BOOL PreTranslateMessage(MSG* pMsg);

	virtual BOOL OnIdle()
	{
		UIUpdateChildWindows();
		return FALSE;
	}

	void	ChangeWindowTitle(const std::wstring& title);

	BEGIN_UPDATE_UI_MAP(CMainDlg)
	END_UPDATE_UI_MAP()

	BEGIN_DDX_MAP(CMainDlg)
		DDX_CONTROL_HANDLE(IDC_COMBO_UMAMUSUME, m_cmbUmaMusume)
		DDX_TEXT(IDC_EDIT_EVENTNAME, m_eventName)
		DDX_TEXT(IDC_EDIT_EVENT_SOURCE, m_eventSource)
		DDX_CONTROL(IDC_BUTTON_SCREENSHOT, m_wndScreenShotButton)
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

		MESSAGE_HANDLER_EX(WM_ENTERSIZEMOVE, OnDockingProcess)
		MESSAGE_HANDLER_EX(WM_MOVING, OnDockingProcess)
		MESSAGE_HANDLER_EX(WM_EXITSIZEMOVE, OnDockingProcess)

		MSG_WM_CTLCOLORDLG(OnCtlColorDlg)
		MSG_WM_CTLCOLORSTATIC(OnCtlColorDlg)
		MSG_WM_CTLCOLORBTN(OnCtlColorDlg)
		MSG_WM_CTLCOLOREDIT(OnCtlColorDlg)

		COMMAND_ID_HANDLER_EX(IDC_BUTTON_SCREENSHOT, OnScreenShot)
		COMMAND_ID_HANDLER_EX(IDC_CHECK_START, OnStart)

		COMMAND_HANDLER_EX(IDC_EDIT_EVENTNAME, EN_CHANGE, OnEventNameChanged)

		COMMAND_ID_HANDLER_EX(IDC_BUTTON_REVISION, OnEventRevision)

		MSG_WM_SETCURSOR(OnSetCursor)

		CHAIN_MSG_MAP(DarkModeUI<CMainDlg>)
	ALT_MSG_MAP(1)
		MSG_WM_RBUTTONUP(OnScreenShotButtonUp)
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

	LRESULT OnDockingProcess(UINT uMsg, WPARAM wParam, LPARAM lParam);

	HBRUSH OnCtlColorDlg(CDCHandle dc, CWindow wnd);

	void OnScreenShot(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnStart(UINT uNotifyCode, int nID, CWindow wndCtl);

	void OnEventNameChanged(UINT uNotifyCode, int nID, CWindow wndCtl);

	void OnEventRevision(UINT uNotifyCode, int nID, CWindow wndCtl);

	// for EffectEdit
	BOOL OnSetCursor(CWindow wnd, UINT nHitTest, UINT message);

	void OnScreenShotButtonUp(UINT nFlags, CPoint point);

private:
	bool	_ReloadUmaMusumeLibrary();
	void	_DockOrPopupRaceListWindow();
	void	_ExtentOrShrinkWindow(bool bExtent);
	void	_SwitchRow3Row4(bool row3);

	void	_UpdateEventOptions(const UmaEventLibrary::UmaEvent& umaEvent);
	void	_UpdateEventEffect(CRichEditCtrl richEdit, const std::wstring& effectText);

	Config	m_config;
	bool	m_bShowRaceList = true;

	UmaEventLibrary	m_umaEventLibrary;
	SkillLibrary	m_skillLibrary;
	UmaTextRecognizer	m_umaTextRecoginzer;

	PreviewWindow	m_previewWindow;
	RaceListWindow	m_raceListWindow;

	CString	m_targetWindowName;
	CString m_targetClassName;

	CContainedWindow	m_wndScreenShotButton;
	CComboBox	m_cmbUmaMusume;
	COLORREF	m_optionBkColor[kMaxOptionEffect];
	CBrush	m_brsOptions[kMaxOptionEffect];
	COLORREF	m_effectStatusInc;
	COLORREF	m_effectStatusDec;

	RichEditPopup	m_popupRichEdit;
	CFont	m_effectFont;

	CString	m_eventName;
	CString	m_eventSource;
	CRect	m_rcBounds;

	std::thread	m_threadAutoDetect;
	std::atomic_bool	m_cancelAutoDetect;

	bool	m_bDockingMove = false;
	CPoint	m_ptRelativeDockingPos;

	bool	m_effectRow3 = false;
};
