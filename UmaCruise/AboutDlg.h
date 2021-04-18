// aboutdlg.h : interface of the CAboutDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

class PreviewWindow;

#include "resource.h"

constexpr LPCWSTR	kAppVersion = L"v1.1";

class CAboutDlg : public CDialogImpl<CAboutDlg>
{
public:
	enum { IDD = IDD_ABOUTBOX };

	CAboutDlg(PreviewWindow& previewWindow);

	BEGIN_MSG_MAP(CAboutDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		COMMAND_ID_HANDLER(IDC_BUTTON_OCR, OnOCR)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT OnOCR(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

private:
	PreviewWindow& m_previewWindow;

	enum TestBounds {
		kDirect, kUmaMusumeSubNameBounds, kUmaMusumeNameBounds, kCurrentTurnBounds, kEventCategoryBounds, kEventNameBounds, kEventNameIconBounds, kCurrentMenuBounds, kMaxCount
	};
	static constexpr LPCWSTR kTestBoundsName[kMaxCount] = {
		L"Direct", L"UmaMusumeSubNameBounds", L"UmaMusumeNameBounds", L"CurrentTurnBounds", L"EventCategoryBounds", L"EventNameBounds", L"EventNameIconBounds", L"CurrentMenuBounds"
	};


	CComboBox	m_cmbTestBounds;
	CEdit	m_editResult;
	CTrackBarCtrl	m_sliderThreshold;
};
