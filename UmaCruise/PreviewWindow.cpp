#include "stdafx.h"
#include "PreviewWindow.h"
#include <shlwapi.h>

#include "Utility\GdiplusUtil.h"
#include "Utility\CommonUtility.h"
#include "Utility\json.hpp"
#include "Utility\DropHandler.h"

#pragma comment(lib, "Shlwapi.lib")

using json = nlohmann::json;

/// 最大サイズに収まるように画像の比率を考えて縮小する
CSize	CalcActualSize(Gdiplus::Image* image, CSize maxImageSize)
{
	CSize ActualSize;
	int nImageWidth = image->GetWidth();
	int nImageHeight = image->GetHeight();
	const int kMaxImageWidth = maxImageSize.cx;
	const int kMaxImageHeight = maxImageSize.cy;
	if (nImageWidth > kMaxImageWidth || nImageHeight > kMaxImageHeight) {
		if (nImageHeight > kMaxImageHeight) {
			ActualSize.cx = (int)((kMaxImageHeight / (double)nImageHeight) * nImageWidth);
			ActualSize.cy = kMaxImageHeight;
			if (ActualSize.cx > kMaxImageWidth) {
				ActualSize.cy = (int)((kMaxImageWidth / (double)ActualSize.cx) * ActualSize.cy);
				ActualSize.cx = kMaxImageWidth;
			}
		} else {
			ActualSize.cy = (int)((kMaxImageWidth / (double)nImageWidth) * nImageHeight);
			ActualSize.cx = kMaxImageWidth;
		}
	} else {
		ActualSize.SetSize(nImageWidth, nImageHeight);
	}
	return ActualSize;
}


void PreviewWindow::UpdateImage(const std::wstring& imagePath)
{
	CComPtr<IStream> spFileStream = LoadFileToMemoryStream(imagePath);
	//::SHCreateStreamOnFileW(imagePath.c_str(), STGM_READ, &spFileStream);
	ATLASSERT(spFileStream);
	m_pImage.reset(Gdiplus::Image::FromStream(spFileStream));

	m_detectedRect.clear();
	Invalidate();
}

void PreviewWindow::UpdateImage(Gdiplus::Bitmap* image)
{
	m_pImage.reset(image);

	m_detectedRect.clear();
	Invalidate();
}

void PreviewWindow::ChangeDragdropBounds(const CRect& rcActualBounds)
{
	if (!m_pImage) {
		return;
	}
	m_rcDragdropActualBounds = rcActualBounds;
	CSize actualSize = _CalcPreviewImageActualSize();

	double widthRatio = actualSize.cx / static_cast<double>(m_pImage->GetWidth());
	double heightRatio = actualSize.cy / static_cast<double>(m_pImage->GetHeight());

	m_rcDragdropBounds = m_rcDragdropActualBounds;
	m_rcDragdropBounds.left = static_cast<LONG>(m_rcDragdropBounds.left * widthRatio);
	m_rcDragdropBounds.right = static_cast<LONG>(m_rcDragdropBounds.right * widthRatio);
	m_rcDragdropBounds.top = static_cast<LONG>(m_rcDragdropBounds.top * heightRatio);
	m_rcDragdropBounds.bottom = static_cast<LONG>(m_rcDragdropBounds.bottom * heightRatio);

	Invalidate();
}

void PreviewWindow::SetDetectedRectangle(const std::vector<cv::Rect>& detectedRect)
{
	m_detectedRect = detectedRect;
	Invalidate();
}


LRESULT PreviewWindow::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	DragAcceptFiles(TRUE);
	ATLVERIFY(::ChangeWindowMessageFilterEx(m_hWnd, WM_DROPFILES, MSGFLT_ALLOW, nullptr));
	//ATLVERIFY(::ChangeWindowMessageFilterEx(m_hWnd, WM_COPYDATA, MSGFLT_ALLOW, nullptr));
	ATLVERIFY(::ChangeWindowMessageFilterEx(m_hWnd, 0x0049 , MSGFLT_ALLOW, nullptr));

	return 0;
}

LRESULT PreviewWindow::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	return 0;
}

LRESULT PreviewWindow::OnCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	ShowWindow(SW_HIDE);
	return 0;
}

void PreviewWindow::OnDropFiles(HDROP hDropInfo)
{
	DropHandler drop(hDropInfo);
	UINT nCount = drop.GetCount();
	if (nCount == 0) {
		return;
	}
	std::wstring dropFilePath = drop.GetFilePath();
	UpdateImage(dropFilePath);
}

void PreviewWindow::OnPaint(CDCHandle dc)
{
	CPaintDC dcPaint(m_hWnd);
	CMemoryDC memDC(dcPaint, dcPaint.m_ps.rcPaint);

	CRect rcClient;
	GetClientRect(&rcClient);
	memDC.FillSolidRect(rcClient, RGB(0xFF, 0xFF, 0xFF));
	if (m_pImage) {
		Gdiplus::Graphics gClient(memDC);

		CSize actualSize = _CalcPreviewImageActualSize();
		gClient.DrawImage(m_pImage.get(), 0, 0, actualSize.cx, actualSize.cy);

		for (const auto& rect : _ConvertOriginRectToActualRect()) {
			Gdiplus::Pen pen(Gdiplus::Color::Blue, 2);
			Gdiplus::Rect rcDetected(rect.x, rect.y, rect.width, rect.height);
			gClient.DrawRectangle(&pen, rcDetected);
		}

		if (m_rcDragdropBounds.Width() > 0 && m_rcDragdropBounds.Height() > 0) {
			Gdiplus::Pen pen(Gdiplus::Color(0, 0xFF, 0), 2);
			Gdiplus::Rect rcDrag(m_rcDragdropBounds.left, m_rcDragdropBounds.top, 
				m_rcDragdropBounds.Width(), m_rcDragdropBounds.Height());
			gClient.DrawRectangle(&pen, rcDrag);
		}
	}
}

void PreviewWindow::OnSize(UINT nType, CSize size)
{
	Invalidate();
}


void PreviewWindow::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (!m_pImage) {
		return;
	}
	SetCapture();
	m_ptDragBegin = point;
	m_ptDragEnd = point;
	_ConvertDragdropPointToBounds();
	m_funcNotifyDragdropBounds(m_rcDragdropActualBounds);

	Invalidate();
}

void PreviewWindow::OnMouseMove(UINT nFlags, CPoint point)
{
	if (GetCapture() != m_hWnd) {
		return;
	}
	m_ptDragEnd = point;
	_ConvertDragdropPointToBounds();
	m_funcNotifyDragdropBounds(m_rcDragdropActualBounds);

	Invalidate();
}

void PreviewWindow::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (GetCapture() != m_hWnd) {
		return;
	}
	ReleaseCapture();

	m_ptDragEnd = point;
	_ConvertDragdropPointToBounds();
	m_funcNotifyDragdropBounds(m_rcDragdropActualBounds);

	Invalidate();
}

void PreviewWindow::_ConvertDragdropPointToBounds()
{
	m_rcDragdropBounds = CRect(
		CPoint(std::min(m_ptDragBegin.x, m_ptDragEnd.x), std::min(m_ptDragBegin.y, m_ptDragEnd.y)),
		CSize(std::abs(m_ptDragEnd.x - m_ptDragBegin.x), std::abs(m_ptDragEnd.y - m_ptDragBegin.y))
	);
	
	CSize actualSize = _CalcPreviewImageActualSize();

	// ドラッグドロップが画像内に収まるように補正
	m_rcDragdropBounds.top = std::max(m_rcDragdropBounds.top, 0L);
	m_rcDragdropBounds.left = std::max(m_rcDragdropBounds.left, 0L);
	m_rcDragdropBounds.right = std::min(m_rcDragdropBounds.right, actualSize.cx);
	m_rcDragdropBounds.bottom = std::min(m_rcDragdropBounds.bottom, actualSize.cy);

	double widthRatio = static_cast<double>(m_pImage->GetWidth()) / actualSize.cx;
	double heightRatio = static_cast<double>(m_pImage->GetHeight()) / actualSize.cy;
	m_rcDragdropActualBounds = m_rcDragdropBounds;
	m_rcDragdropActualBounds.left = static_cast<LONG>(m_rcDragdropActualBounds.left * widthRatio);
	m_rcDragdropActualBounds.right = static_cast<LONG>(m_rcDragdropActualBounds.right * widthRatio);
	m_rcDragdropActualBounds.top = static_cast<LONG>(m_rcDragdropActualBounds.top * heightRatio);
	m_rcDragdropActualBounds.bottom = static_cast<LONG>(m_rcDragdropActualBounds.bottom * heightRatio);
}

CSize PreviewWindow::_CalcPreviewImageActualSize()
{
	CRect rcClient;
	GetClientRect(&rcClient);
	CSize maxImageSize = { rcClient.right, rcClient.bottom };
	CSize actualSize = CalcActualSize(m_pImage.get(), maxImageSize);
	return actualSize;
}

std::vector<cv::Rect> PreviewWindow::_ConvertOriginRectToActualRect()
{
	CSize actualSize = _CalcPreviewImageActualSize();

	double widthRatio = actualSize.cx / static_cast<double>(m_pImage->GetWidth());
	double heightRatio = actualSize.cy / static_cast<double>(m_pImage->GetHeight());

	std::vector<cv::Rect> actualRects;
	for (const auto& rect : m_detectedRect) {
		cv::Rect actualRect;
		actualRect.x = static_cast<LONG>(rect.x * widthRatio);
		actualRect.width = static_cast<LONG>(rect.width * widthRatio);
		actualRect.y = static_cast<LONG>(rect.y * heightRatio);
		actualRect.height = static_cast<LONG>(rect.height * heightRatio);
		actualRects.push_back(actualRect);
	}
	return actualRects;
}

