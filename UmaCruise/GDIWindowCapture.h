#pragma once

#include <memory>

#include <wtl\atlgdi.h>

#include "Utility\Logger.h"
#include "Utility\GdiplusUtil.h"

#include "IScreenShotWindow.h"

class GDIWindowCapture : public IScreenShotWindow
{
public:

	std::unique_ptr<Gdiplus::Bitmap>	ScreenShot(HWND hwndTarget, const CRect& rcAdjustClient) override
	{
		CWindowDC dc(NULL/*hWndTarget*/);	// desktop

		CDC dcMemory;
		dcMemory.CreateCompatibleDC(dc);

		LPVOID           lp = nullptr;
		BITMAPINFO       bmi = {};
		BITMAPINFOHEADER bmiHeader = { sizeof(BITMAPINFOHEADER) };
		bmiHeader.biWidth = rcAdjustClient.Width();
		bmiHeader.biHeight = rcAdjustClient.Height();
		bmiHeader.biPlanes = 1;
		bmiHeader.biBitCount = 24;
		bmi.bmiHeader = bmiHeader;

		//CBitmap hbmp = ::CreateCompatibleBitmap(dc, rcAdjustClient.Width(), rcAdjustClient.Height());
		CBitmap hbmp = ::CreateDIBSection(dc, (LPBITMAPINFO)&bmi, DIB_RGB_COLORS, &lp, NULL, 0);
		auto prevhbmp = dcMemory.SelectBitmap(hbmp);

		//dcMemory.BitBlt(0, 0, rcWindow.Width(), rcWindow.Height(), dc, 0, 0, SRCCOPY);
		dcMemory.BitBlt(0, 0, rcAdjustClient.Width(), rcAdjustClient.Height(), dc, rcAdjustClient.left, rcAdjustClient.top, SRCCOPY);
		dcMemory.SelectBitmap(prevhbmp);
		//WARN_LOG << "sstime: " << timer.format().c_str();
		return std::unique_ptr<Gdiplus::Bitmap>(Gdiplus::Bitmap::FromHBITMAP(hbmp, NULL));
	}

};