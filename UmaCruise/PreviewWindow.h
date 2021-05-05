#pragma once

#include <memory>
#include <string>
#include <functional>
#include <vector>

#include <wtl\atlmisc.h>

#include "Utility\GdiplusUtil.h"
#include <opencv2\opencv.hpp>

#include "DarkModeUI.h"

#include "resource.h"


CSize	CalcActualSize(Gdiplus::Image* image, CSize maxImageSize);

class PreviewWindow : public CDialogImpl<PreviewWindow>, DarkModeUI<PreviewWindow>
{
public:
	enum { IDD = IDD_PREVIEW };

	void	UpdateImage(const std::wstring& imagePath);
	void	UpdateImage(Gdiplus::Bitmap* image);

	CSize	GetImageSize() const { 
		if (m_pImage) {
			return { static_cast<int>(m_pImage->GetWidth()), static_cast<int>(m_pImage->GetHeight()) };
		} else {
			return CSize();
		}
	}
	Gdiplus::Image* GetImage() const { 
		return m_pImage.get();
	}

	CRect	GetDragdropActualBounds() const { 
		return m_rcDragdropActualBounds;
	}

	void	SetNotifyDragdropBounds(std::function<void(const CRect&)> func) {
		m_funcNotifyDragdropBounds = func;
	}
	void	ChangeDragdropBounds(const CRect& rcActualBounds);

	void	SetDetectedRectangle(const std::vector<cv::Rect>& detectedRect);

	BEGIN_MSG_MAP_EX(CMainDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		MSG_WM_DROPFILES(OnDropFiles)

		MSG_WM_ERASEBKGND(OnEraseBkgnd)
		MSG_WM_PAINT(OnPaint)
		MSG_WM_SIZE(OnSize)
		MSG_WM_LBUTTONDOWN(OnLButtonDown)
		MSG_WM_MOUSEMOVE(OnMouseMove)
		MSG_WM_LBUTTONUP(OnLButtonUp)

		CHAIN_MSG_MAP(DarkModeUI<PreviewWindow>)
	END_MSG_MAP()

	// Handler prototypes (uncomment arguments if needed):
	//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	void OnDropFiles(HDROP hDropInfo);

	BOOL OnEraseBkgnd(CDCHandle dc) { return TRUE; }
	void OnPaint(CDCHandle dc);
	void OnSize(UINT nType, CSize size);

	void OnLButtonDown(UINT nFlags, CPoint point);
	void OnMouseMove(UINT nFlags, CPoint point);
	void OnLButtonUp(UINT nFlags, CPoint point);

private:
	void	_ConvertDragdropPointToBounds();
	CSize	_CalcPreviewImageActualSize();
	std::vector<cv::Rect>	_ConvertOriginRectToActualRect();

	std::unique_ptr<Gdiplus::Image>	m_pImage;
	std::function<void(const CRect&)> m_funcNotifyDragdropBounds;
	std::vector<cv::Rect>	m_detectedRect;

	CPoint	m_ptDragBegin;
	CPoint	m_ptDragEnd;
	CRect	m_rcDragdropBounds;
	CRect	m_rcDragdropActualBounds;
};
