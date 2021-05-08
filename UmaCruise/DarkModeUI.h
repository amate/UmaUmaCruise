#pragma once

#include <unordered_set>
#include <vector>
#include <string>
#include <fstream>

#include "Utility\CommonUtility.h"
#include "Utility\json.hpp"

#include "win32-darkmode\DarkMode.h"
#include "win32-darkmode\ListViewUtil.h"

inline	COLORREF ColorFromText(const std::string& colorCode)
{
	ATLASSERT(colorCode.front() == '#' && colorCode.length() == 7);
	BYTE r = std::stoi(colorCode.substr(1, 2), nullptr, 16);
	BYTE g = std::stoi(colorCode.substr(3, 2), nullptr, 16);
	BYTE b = std::stoi(colorCode.substr(5, 2), nullptr, 16);
	COLORREF color = RGB(r, g, b);
	return color;
}

template<class T>
class DarkModeUI
{
public:
	void	DarkModeInit()
	{
		json jsonCommon;
		std::ifstream fs((GetExeDirectory() / L"UmaLibrary" / "Common.json").wstring());
		ATLASSERT(fs);
		fs >> jsonCommon;
		fs.close();

		m_dark.bkColor = ColorFromText(jsonCommon["Theme"]["Dark"]["Background"].get<std::string>());
		m_dark.textColor = ColorFromText(jsonCommon["Theme"]["Dark"]["Text"].get<std::string>());
		m_light.bkColor = ColorFromText(jsonCommon["Theme"]["Light"]["Background"].get<std::string>());
		m_light.textColor = ColorFromText(jsonCommon["Theme"]["Light"]["Text"].get<std::string>());

		m_hbrDarkBkgnd.CreateSolidBrush(m_dark.bkColor);
		m_hbrLightBkgnd.CreateSolidBrush(m_light.bkColor);

		m_bLastDarkModeEnabled = UpdateDarkModeEnabled();
		//g_darkModeEnabled = true;
		OnThemeChanged(true);
	}

	bool	IsDarkMode() const {
		return m_bLastDarkModeEnabled;
	}

	HBRUSH GetBkgndBrush() const {
		if (g_darkModeSupported && IsDarkMode()) {
			return m_hbrDarkBkgnd;
		} else {
			return m_hbrLightBkgnd;
		}
	}

	COLORREF GetTextColor() const {
		if (g_darkModeSupported && IsDarkMode()) {
			return m_dark.textColor;
		} else {
			return m_light.textColor;
		}
	}

	BEGIN_MSG_MAP_EX(DarkModeUI<T>)
		MSG_WM_SETTINGCHANGE(OnSettingChange)
		MSG_WM_THEMECHANGED(OnThemeChanged)

		MSG_WM_CTLCOLORDLG(OnCtlColorDlg)
		MSG_WM_CTLCOLORSTATIC(OnCtlColorDlg)
		MSG_WM_CTLCOLORBTN(OnCtlColorDlg)
		MSG_WM_CTLCOLOREDIT(OnCtlColorDlg)
		MSG_WM_CTLCOLORLISTBOX(OnCtlColorDlg)
	END_MSG_MAP()

	void OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
	{
		if (g_darkModeSupported && IsColorSchemeChangeMessage((LPARAM)lpszSection)) {
			OnThemeChanged();
		}
	}

	void OnThemeChanged(bool force = false)
	{
		if (g_darkModeSupported) {
			UpdateDarkModeEnabled();

			if (!force) {
				if (m_bLastDarkModeEnabled == g_darkModeEnabled) {
					return;
				}
			}
			m_bLastDarkModeEnabled = g_darkModeEnabled;
			//ATLASSERT(g_darkModeEnabled);

			T* pThis = static_cast<T*>(this);
			AllowDarkModeForWindow(pThis->m_hWnd, g_darkModeEnabled);
			RefreshTitleBarThemeColor(pThis->m_hWnd);

			std::vector<HWND> childWindowList;
			EnumChildWindows(pThis->m_hWnd, [](HWND hwnd, LPARAM lParam) -> BOOL {	
				auto pList = (std::vector<HWND>*)lParam;
				pList->emplace_back(hwnd);
				return TRUE;
			}, (LPARAM)&childWindowList);

			for (HWND hwnd : childWindowList) {
				if (::GetParent(hwnd) != pThis->m_hWnd) {
					continue;
				}

				CString className;
				::GetClassName(hwnd, className.GetBuffer(128), 128);
				className.ReleaseBuffer();

				if (className == L"ComboBox") {
					CComboBox cmb = hwnd;
					COMBOBOXINFO cbi = { sizeof(COMBOBOXINFO) };
					cmb.GetComboBoxInfo(&cbi);

					SetWindowTheme(cbi.hwndList, L"DarkMode_Explorer", nullptr);
					AllowDarkModeForWindow(hwnd, g_darkModeEnabled);

					if (g_darkModeEnabled) {
						SetWindowTheme(hwnd, L"DarkMode_CFD", nullptr);
					} else {
						SetWindowTheme(hwnd, L"Explorer", nullptr);
					}
				} else if (className == L"SysListView32") {
					//SetWindowTheme(hwnd, L"DarkMode_ItemsView", nullptr);
					if (m_setListView.find(hwnd) == m_setListView.end()) {
						InitListView(hwnd);
						m_setListView.emplace(hwnd);
					}
					HWND hHeader = ListView_GetHeader(hwnd);
					LPCWSTR theme = g_darkModeEnabled ? L"DarkMode_ItemsView" : L"ItemsView";
					SetWindowTheme(hHeader, theme, nullptr);
					SetWindowTheme(hwnd, theme, nullptr);
					::SendMessage(hwnd, WM_THEMECHANGED, 0, 0);

				} else if (className == L"RICHEDIT50W") {
					CRichEditCtrl richEdit = hwnd;
					COLORREF	bkColor = g_darkModeEnabled ? m_dark.bkColor : m_light.bkColor;
					COLORREF	textColor = g_darkModeEnabled ? m_dark.textColor : m_light.textColor;
					richEdit.SetBackgroundColor(bkColor);
					CHARFORMAT charFormat = {};
					charFormat.dwMask = CFM_COLOR;
					charFormat.crTextColor = textColor;
					richEdit.SetDefaultCharFormat(charFormat);

				} else {
					const DWORD windowStyle = CWindow(hwnd).GetStyle();
					auto funcDisableThemeCtrl = [windowStyle]() -> bool {
						if ((windowStyle & BS_PUSHLIKE) == BS_PUSHLIKE) {
							return false;
						} else if ((windowStyle & BS_CHECKBOX) == BS_CHECKBOX) {
							return true;
						} else if((windowStyle& BS_GROUPBOX) == BS_GROUPBOX) {
							return true;
						} else {
							return false;
						}
					};

					if (g_darkModeEnabled && funcDisableThemeCtrl()) {
						SetWindowTheme(hwnd, L"dummy", L"dummy");
						CString name;
						CWindow(hwnd).GetWindowText(name);
						//INFO_LOG << L"name: " << (LPCWSTR)name;
					} else {
						SetWindowTheme(hwnd, g_darkModeEnabled ? L"DarkMode_Explorer" : L"Explorer", nullptr);
					}
				}

				AllowDarkModeForWindow(hwnd, g_darkModeEnabled);
				::SendMessageW(hwnd, WM_THEMECHANGED, 0, 0);
			}

			pThis->Invalidate();
		}
	}

	HBRUSH OnCtlColorDlg(CDCHandle dc, CWindow wnd)
	{
		if (g_darkModeSupported && IsDarkMode()) {
			dc.SetTextColor(m_dark.textColor);
			dc.SetBkColor(m_dark.bkColor);
		} else {
			dc.SetTextColor(m_light.textColor);
			dc.SetBkColor(m_light.bkColor);
		}
		return GetBkgndBrush();
	}

private:
	struct ThemeColor {
		COLORREF	bkColor;
		COLORREF	textColor;
	};
	ThemeColor	m_dark;
	ThemeColor	m_light;

	CBrush	m_hbrDarkBkgnd;
	CBrush	m_hbrLightBkgnd;

	bool	m_bLastDarkModeEnabled = false;
	std::unordered_set<HWND>	m_setListView;
};

