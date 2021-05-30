#include "stdafx.h"
#include "RichEditPopup.h"

HWND RichEditPopup::Create(HWND hwndParent)
{
    __super::Create(hwndParent, rcDefault, nullptr, WS_POPUP | WS_BORDER);
    return m_hWnd;
}

CString RichEditPopup::GetEffectText(const CString eventName) const
{
    if (eventName != m_eventName) {
        return L""; // 同一イベントでなければ巻き戻さない
    }

    CString text;
    const int textLength = m_effectEdit.GetWindowTextLengthW() + 1;
    m_effectEdit.GetWindowText(text.GetBuffer(textLength), textLength);
    text.ReleaseBuffer();
    text.Trim();
    return text;
}

int RichEditPopup::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    DWORD popupEditStyle = WS_CHILD | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_WANTRETURN;
    m_effectEdit.Create(m_hWnd, rcDefault, L"", popupEditStyle);
    m_effectEdit.ShowWindow(SW_NORMAL);
    
    m_effectEdit.SendMessage(EM_SETLANGOPTIONS, 0, (LPARAM)IMF_UIFONTS);
    m_effectEdit.SetFont(m_font);

    {
        nlohmann::json jsonCommon;
        std::ifstream fs((GetExeDirectory() / L"UmaLibrary" / "Common.json").wstring());
        ATLASSERT(fs);
        fs >> jsonCommon;
        fs.close();

        m_darkEditBkColor = ColorFromText(
            jsonCommon["Theme"]["Dark"]["RichEditPopup"]["Background"].get<std::string>());
        m_lightEditBkColor = ColorFromText(
            jsonCommon["Theme"]["Light"]["RichEditPopup"]["Background"].get<std::string>());
    }

    DarkModeInit();
    OnThemeChanged();

    return 0;
}

void RichEditPopup::OnDestroy()
{
}

void RichEditPopup::OnSize(UINT nType, CSize size)
{
    m_effectEdit.SetWindowPos(NULL, 0, 0, size.cx, size.cy, SWP_NOZORDER | SWP_NOMOVE);
}

void RichEditPopup::OnThemeChanged()
{
    __super::OnThemeChanged();
    m_effectEdit.SetBackgroundColor(IsDarkMode() ? m_darkEditBkColor : m_lightEditBkColor);
}

BOOL RichEditPopup::OnSetCursor(CWindow wnd, UINT nHitTest, UINT message)
{
    if (::GetFocus() == m_effectEdit) {
        return 0;
    }

    if (message == WM_MOUSEMOVE) {
        CPoint ptCursor;
        ::GetCursorPos(&ptCursor);
        if (!m_rcOriginalEffectRichEdit.PtInRect(ptCursor)) {
            // 親にカーソルがオリジナルリッチエディットの範囲外に移動したことを伝える
            GetParent().PostMessage(WM_SETCURSOR, 0, WM_MOUSEMOVE);
        }
    }
    return 0;
}
