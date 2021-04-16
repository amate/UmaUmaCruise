
#include "stdafx.h"
#include "AboutDlg.h"
#include "PreviewWindow.h"

#include <tesseract\baseapi.h>
#include <leptonica\allheaders.h>

#include <opencv2\opencv.hpp>

#include "Utility\CodeConvert.h"
#include "Utility\CommonUtility.h"
#include "Utility\json.hpp"
#include "Utility\timer.h"
#include "TesseractWrapper.h"
using namespace TesseractWrapper;

using json = nlohmann::json;
using namespace CodeConvert;
using namespace cv;

cv::Mat GdiPlusBitmapToOpenCvMat(Gdiplus::Bitmap* bmp);

void	LoadPointSizeFromJson(const json& json, const std::string& key, CPoint& pt, CSize& size);

CRect AdjustBounds(const cv::Mat& srcImage, CRect bounds)
{
	const CSize baseSize = { 588,  1045 };	// デバッグ用なので決め打ちで
	//CSize imageSize(static_cast<int>(image->GetWidth()), static_cast<int>(image->GetHeight()));
	CSize imageSize(srcImage.size().width, srcImage.size().height);
	const double Xratio = static_cast<double>(imageSize.cx) / baseSize.cx;
	const double Yratio = static_cast<double>(imageSize.cy) / baseSize.cy;

	CRect adjustRect = bounds;
	adjustRect.top *= Yratio;
	adjustRect.left *= Xratio;
	adjustRect.right *= Xratio;
	adjustRect.bottom *= Yratio;

	return adjustRect;
}

CRect GetTextBounds(cv::Mat cutImage, const CRect& rcBounds);	// UmaTextRecognizer.cpp

/////////////////////////////////////////////////////////////////////////////////////

CAboutDlg::CAboutDlg(PreviewWindow& previewWindow): m_previewWindow(previewWindow)
{
}

LRESULT CAboutDlg::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&)
{
	CenterWindow(GetParent());

	m_cmbTestBounds = GetDlgItem(IDC_COMBO_TESTBOUNDS);
	m_editResult = GetDlgItem(IDC_EDIT_RESULT);
	m_sliderThreshold = GetDlgItem(IDC_SLIDER_THRESHOLD);
	m_sliderThreshold.SetRange(0, 255);

	for (LPCWSTR name : kTestBoundsName) {
		m_cmbTestBounds.AddString(name);
	}
	m_cmbTestBounds.SetCurSel(0);

	return TRUE;
}

LRESULT CAboutDlg::OnCloseCmd(WORD, WORD wID, HWND, BOOL&)
{
	cv::destroyAllWindows();
	EndDialog(wID);
	return 0;
}

LRESULT CAboutDlg::OnOCR(WORD, WORD, HWND, BOOL&)
{
	const int index = m_cmbTestBounds.GetCurSel();
	if (index == -1) {
		return 0;
	}
	auto image = m_previewWindow.GetImage();
	if (!image) {
		return 0;
	}

	CRect rcBounds;
	if (index != kDirect) {
		json jsonCommon;
		std::ifstream ifs((GetExeDirectory() / L"Common.json").string());
		ATLASSERT(ifs);
		if (!ifs) {
			return 0;
		}
		ifs >> jsonCommon;

		json jsonOCR = jsonCommon["Common"]["TestBounds"];

		CPoint pt;
		CSize size;
		LoadPointSizeFromJson(jsonOCR, UTF8fromUTF16(kTestBoundsName[index]), pt, size);
		rcBounds = CRect(pt, size);
		m_previewWindow.ChangeDragdropBounds(rcBounds);
	} else {
		rcBounds = m_previewWindow.GetDragdropActualBounds();
	}

	if (rcBounds.Size() == CSize()) {
		return 0;
	}

	const bool bManualThresOnly = ::GetKeyState(VK_CONTROL) < 0;	// ctrl でスライドバーOCRのみ
	const bool bNoAdjustBounds = ::GetKeyState(VK_SHIFT) < 0;	// shift でテキストを囲わない
	const bool bScale4 = ::GetKeyState(VK_MENU) < 0;		// atl で4倍拡大

	// 切り抜き＆変換
	Utility::timer timer;

	Gdiplus::Bitmap bmp(image->GetWidth(), image->GetHeight(), PixelFormat24bppRGB);
	Gdiplus::Graphics graphics(&bmp);
	graphics.DrawImage(image, 0, 0);
	auto srcImage = GdiPlusBitmapToOpenCvMat(&bmp);//cv::imread(ssPath.string());
	rcBounds = AdjustBounds(srcImage, rcBounds);

	cv::Rect rcTrim(rcBounds.left, rcBounds.top, rcBounds.Width(), rcBounds.Height());
	cv::Mat cutImage(srcImage, rcTrim);

	if (index == kEventNameBounds) {
		if (!bNoAdjustBounds) {	// テキストを正確囲む
			CRect rcAdjustTextBounds = GetTextBounds(cutImage, rcBounds);
			rcBounds = rcAdjustTextBounds;
		}
	}
	m_previewWindow.ChangeDragdropBounds(rcBounds);

	{
		cv::Mat resizedImage;
		//constexpr double scale = 4.0;
		double scale = 1.0;//4.0;
		if (bScale4) {
			scale = 4.0;
		}
		cv::resize(cutImage, resizedImage, cv::Size(), scale, scale, cv::INTER_CUBIC);
		auto saveImagePath = GetExeDirectory() / L"debug1.png";
		cv::imwrite(saveImagePath.string(), resizedImage);

		cv::Mat grayImage;
		cv::cvtColor(resizedImage, grayImage, cv::COLOR_RGB2GRAY);

		cv::Mat invertedImage;
		cv::bitwise_not(grayImage, invertedImage);

		cv::Mat thresImage;
		//cv::threshold(grayImage, thresImage, 190.0, 255.0, cv::THRESH_BINARY_INV);
		cv::threshold(grayImage, thresImage, 0.0, 255.0, cv::THRESH_OTSU);

		cv::Mat thresImage2;
		//cv::threshold(grayImage, thresImage, 190.0, 255.0, cv::THRESH_BINARY_INV);
		cv::threshold(invertedImage, thresImage2, 0.0, 255.0, cv::THRESH_OTSU);

		// ==================================================

		const int threshold = m_sliderThreshold.GetPos();
		cv::Mat manualThresholdImage;
		cv::threshold(grayImage, manualThresholdImage, threshold, 255.0, cv::THRESH_BINARY);


		ATLTRACE(L"切り抜き変換 %s\n", UTF16fromUTF8(timer.format()).c_str());

		cv::imshow("1", cutImage);
		cv::imshow("2", resizedImage);
		cv::imshow("3", grayImage);
		cv::imshow("4", invertedImage);
		cv::imshow("5", thresImage);
		cv::imshow("6", thresImage2);
		cv::imshow("7", manualThresholdImage);

		cv::Mat targetImage = cutImage;//thresImage;
		CString result;
		if (!bManualThresOnly) {
			result += L"1: " + CString(TextFromImage(cutImage).c_str()) + L"\r\n";
			result += L"2: " + CString(TextFromImage(resizedImage).c_str()) + L"\r\n";
			result += L"3: " + CString(TextFromImage(grayImage).c_str()) + L"\r\n";
			result += L"4: " + CString(TextFromImage(invertedImage).c_str()) + L"\r\n";
			result += L"5: " + CString(TextFromImage(thresImage).c_str()) + L"\r\n";
			result += L"6: " + CString(TextFromImage(thresImage2).c_str()) + L"\r\n";
		}
		result += L"7: " + CString(TextFromImage(manualThresholdImage).c_str()) + L" thres:" + std::to_wstring(threshold).c_str() + L"\r\n";
		m_editResult.SetWindowTextW(result);

	}

	return LRESULT();
}
