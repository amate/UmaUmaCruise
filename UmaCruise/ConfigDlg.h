#pragma once

#include "Config.h"

#include "resource.h"


class ConfigDlg : 
	public CDialogImpl<ConfigDlg>,
	public CWinDataExchange<ConfigDlg>
{
public:
	enum { IDD = IDD_CONFIG };

	ConfigDlg(Config& config);

	BEGIN_DDX_MAP(ConfigDlg)
		DDX_CONTROL_HANDLE(IDC_COMBO_REFRESHINTERVAL, m_cmbRefreshInterval)
		DDX_CHECK(IDC_CHECK_AUTOSTART, m_autoStart)
		DDX_CHECK(IDC_CHECK_STOPUPDATEPREVIEWONTRAINING, m_stopUpdatePreviewOnTraining)
	END_DDX_MAP()

	BEGIN_MSG_MAP_EX(ConfigDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)

		COMMAND_ID_HANDLER_EX(IDC_BUTTON_CHECK_UMALIBRARY, OnCheckUmaLibrary)
	END_MSG_MAP()

	// Handler prototypes (uncomment arguments if needed):
	//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void OnCheckUmaLibrary(UINT uNotifyCode, int nID, CWindow wndCtl);

private:
	Config&		m_config;

	CComboBox	m_cmbRefreshInterval;
	bool	m_autoStart = false;
	bool	m_stopUpdatePreviewOnTraining = false;

};
