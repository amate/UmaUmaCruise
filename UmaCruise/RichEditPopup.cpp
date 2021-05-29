#include "stdafx.h"
#include "RichEditPopup.h"

HWND RichEditPopup::Create(HWND hwndParent)
{
    __super::Create(hwndParent, rcDefault, nullptr, WS_POPUP | WS_BORDER);
    return m_hWnd;
}

int RichEditPopup::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    DWORD popupEditStyle = WS_CHILD | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_WANTRETURN;
    m_effectEdit.Create(m_hWnd, rcDefault, L"", popupEditStyle);
    m_effectEdit.ShowWindow(SW_NORMAL);

    
    m_effectEdit.SendMessage(EM_SETLANGOPTIONS, 0, (LPARAM)IMF_UIFONTS);
    m_effectEdit.SetFont(m_font);

    DarkModeInit();
    return 0;
}

void RichEditPopup::OnDestroy()
{
}

void RichEditPopup::OnSize(UINT nType, CSize size)
{
    m_effectEdit.SetWindowPos(NULL, 0, 0, size.cx, size.cy, SWP_NOZORDER | SWP_NOMOVE);
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
