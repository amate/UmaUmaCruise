#include "stdafx.h"
#include "ConfigDlg.h"



ConfigDlg::ConfigDlg(Config& config) : m_config(config)
{
}

LRESULT ConfigDlg::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&)
{
	DoDataExchange(DDX_LOAD);

	enum { kMaxRefershCount = 10 };
	for (int i = 1; i <= kMaxRefershCount; ++i) {
		m_cmbRefreshInterval.AddString(std::to_wstring(i).c_str());
	}
	m_cmbRefreshInterval.SetCurSel(m_config.refreshInterval - 1);

	m_autoStart = m_config.autoStart;
	DoDataExchange(DDX_LOAD);

	return 0;
}

LRESULT ConfigDlg::OnOK(WORD, WORD wID, HWND, BOOL&)
{
	DoDataExchange(DDX_SAVE);

	const int index = m_cmbRefreshInterval.GetCurSel();
	if (index == -1) {
		ATLASSERT(FALSE);
		ERROR_LOG << L"m_cmbRefreshInterval.GetCurSel == -1";
	} else {
		m_config.refreshInterval = index + 1;
	}
	m_config.autoStart = m_autoStart;

	m_config.SaveConfig();

	EndDialog(IDOK);
	return 0;
}

LRESULT ConfigDlg::OnCancel(WORD, WORD, HWND, BOOL&)
{
	EndDialog(IDCANCEL);
	return 0;
}
