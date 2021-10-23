#pragma once

#include <memory>

#include <atlwin.h>
#include <atltypes.h>

#include "Utility\GdiplusUtil.h"

using SetThreadDpiAwarenessContextFunc = DPI_AWARENESS_CONTEXT(*)(DPI_AWARENESS_CONTEXT);
extern SetThreadDpiAwarenessContextFunc	g_funcSetThreadDpiAwarenessContext;

class IScreenShotWindow
{
public:
	virtual ~IScreenShotWindow() {}

	std::unique_ptr<Gdiplus::Bitmap>	ScreenShot(CWindow hwndTarget)
	{
		if (g_funcSetThreadDpiAwarenessContext) {	// 高DPIモニターで取得ウィンドウの位置がずれるバグを回避するため
			g_funcSetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
		}

		CRect rcClient;
		hwndTarget.GetClientRect(&rcClient);

		CWindow wind;
		hwndTarget.MapWindowPoints(NULL, &rcClient);
		CRect rcAdjustClient = rcClient;

		if (g_funcSetThreadDpiAwarenessContext) {
			g_funcSetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED);
		}

		return ScreenShot(hwndTarget, rcAdjustClient);
	}

protected:
	virtual std::unique_ptr<Gdiplus::Bitmap>	ScreenShot(HWND hwndTarget, const CRect& rcAdjustClient) = 0;

};