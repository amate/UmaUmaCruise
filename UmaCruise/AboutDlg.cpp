
#include "stdafx.h"
#include "AboutDlg.h"
#include "PreviewWindow.h"

#include <future>
#include <vector>
#include <thread>

#include <tesseract\baseapi.h>
#include <leptonica\allheaders.h>

#include <opencv2\opencv.hpp>

#include <boost\algorithm\string\trim_all.hpp>

#include "Utility\CodeConvert.h"
#include "Utility\CommonUtility.h"
#include "Utility\json.hpp"
#include "Utility\timer.h"
#include "Utility\WinHTTPWrapper.h"
#include "TesseractWrapper.h"
using namespace TesseractWrapper;

using json = nlohmann::json;
using namespace CodeConvert;
using namespace cv;

CString	g_versionCheckText;

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
cv::Rect	cvRectFromCRect(const CRect& rcBounds);
double ImageWhiteRatio(cv::Mat thresImage);

bool IsEventNameIcon(cv::Mat srcImage, const CRect& rcIconBounds)
{
	cv::Mat cutImage(srcImage, cvRectFromCRect(rcIconBounds));

	ATLASSERT(cutImage.channels() == 3);	// must color image
	std::vector<cv::Mat> splitColors(3);	// 予め確保していないと落ちる
	cv::split(cutImage, splitColors);
	//cv::imshow("Blue", splitColors[0]);	// [0] -> Blue
	//cv::imshow("Green", splitColors[1]);// [1] -> Green
	//cv::imshow("Red", splitColors[2]);	// [2] -> Red

	cv::Mat blueThresImage;
	enum { kBlueThreshold = 230 };
	cv::threshold(splitColors[0], blueThresImage, kBlueThreshold, 255.0, cv::THRESH_BINARY);
	//cv::imshow("Blue thres", blueThresImage);
	const double blueRatio = ImageWhiteRatio(blueThresImage);
	constexpr double kBlueBackgroundThreshold = 0.7;	// 青背景率の閾値
	if (kBlueBackgroundThreshold < blueRatio) {
		return false;	// サポートカードイベント
	}

	cv::Mat grayImage;
	cv::cvtColor(cutImage, grayImage, cv::COLOR_RGB2GRAY);

	enum { kIconThreshold = 155 };
	cv::Mat thresImage;
	cv::threshold(grayImage, thresImage, kIconThreshold, 255.0, cv::THRESH_BINARY);

	//cv::imwrite((GetExeDirectory() / L"icon.bmp").string().c_str(), thresImage);
	cv::imshow("icon", thresImage);

	const double whiteRatio = ImageWhiteRatio(thresImage);
	constexpr double kEventNameIconThreshold = 0.5;
	bool isIcon = whiteRatio > kEventNameIconThreshold;	// 白の比率が一定以上ならアイコンとみなす
	INFO_LOG << L"IsEventNameIcon: " << isIcon << L" whiteRatio: " << whiteRatio;
	return isIcon;
}


/////////////////////////////////////////////////////////////////////////////////////

CAboutDlg::CAboutDlg(PreviewWindow& previewWindow): m_previewWindow(previewWindow)
{
}

LRESULT CAboutDlg::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&)
{
	CenterWindow(GetParent());

	CWindow wndStaticAbout = GetDlgItem(IDC_STATIC_ABOUT);
	CString about;
	wndStaticAbout.GetWindowText(about);
	about.Replace(L"{version}", kAppVersion);
	wndStaticAbout.SetWindowText(about);

	m_cmbTestBounds = GetDlgItem(IDC_COMBO_TESTBOUNDS);
	m_editResult = GetDlgItem(IDC_EDIT_RESULT);
	m_editResult2 = GetDlgItem(IDC_EDIT_RESULT2);
	m_sliderThreshold = GetDlgItem(IDC_SLIDER_THRESHOLD);
	m_sliderThreshold.SetRange(0, 255);

	for (LPCWSTR name : kTestBoundsName) {
		m_cmbTestBounds.AddString(name);
	}
	m_cmbTestBounds.SetCurSel(0);

#ifdef _DEBUG
	CWindow wndVersionCheck = GetDlgItem(IDC_SYSLINK_VERSIONCHECK);
	wndVersionCheck.SetWindowText(L"デバッグ中");
#else
	if (g_versionCheckText.IsEmpty()) {
		std::thread([this]() {
			CWindow wndVersionCheck = GetDlgItem(IDC_SYSLINK_VERSIONCHECK);

			CString versionURL = L"https://raw.githubusercontent.com/amate/UmaUmaCruise/master/appversion.txt";
			if (auto optVersion = WinHTTPWrapper::HttpDownloadData(versionURL)) {
				std::wstring latestVersion = UTF16fromUTF8(optVersion.get());
				boost::algorithm::trim_all(latestVersion);

				if (latestVersion != kAppVersion) {
					g_versionCheckText.Format(L"更新あり: <a>%s</a>", latestVersion.c_str());
					wndVersionCheck.SetWindowText(g_versionCheckText);
				} else {
					g_versionCheckText = L"更新なし";
					wndVersionCheck.SetWindowText(g_versionCheckText);
				}				
			} else {
				g_versionCheckText = L"更新確認に失敗";
				wndVersionCheck.SetWindowText(g_versionCheckText);
			}
		}).detach();
	} else {
		CWindow wndVersionCheck = GetDlgItem(IDC_SYSLINK_VERSIONCHECK);
		wndVersionCheck.SetWindowText(g_versionCheckText);
	}
#endif
	DarkModeInit();

	return TRUE;
}

LRESULT CAboutDlg::OnCloseCmd(WORD, WORD wID, HWND, BOOL&)
{
	cv::destroyAllWindows();
	EndDialog(wID);
	return 0;
}

LRESULT CAboutDlg::OnLinkClick(int, LPNMHDR, BOOL&)
{
	::ShellExecute(NULL, nullptr, L"https://gamerch.com/umamusume/", nullptr, nullptr, SW_NORMAL);
	return LRESULT();
}

LRESULT CAboutDlg::OnLinkClickHomePage(int, LPNMHDR, BOOL&)
{
	::ShellExecute(NULL, nullptr, L"https://github.com/amate/UmaUmaCruise", nullptr, nullptr, SW_NORMAL);
	return LRESULT();
}

LRESULT CAboutDlg::OnLinkClickLatestVersion(int, LPNMHDR, BOOL&)
{
	::ShellExecute(NULL, nullptr, L"https://github.com/amate/UmaUmaCruise/releases", nullptr, nullptr, SW_NORMAL);
	return LRESULT();
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
	CRect rcIconBounds;

	json jsonCommon;
	std::ifstream ifs((GetExeDirectory() / L"UmaLibrary" / L"Common.json").wstring());
	ATLASSERT(ifs);
	if (!ifs) {
		return 0;
	}
	ifs >> jsonCommon;
	const json& jsonHSVBounds = jsonCommon["Common"]["ImageProcess"]["HSV_TextColorBounds"];
	int h_min = jsonHSVBounds[0][0];
	int h_max = jsonHSVBounds[0][1];
	int s_min = jsonHSVBounds[1][0];
	int s_max = jsonHSVBounds[1][1];
	int v_min = jsonHSVBounds[2][0];
	int v_max = jsonHSVBounds[2][1];
	if (index != kDirect) {


		json jsonOCR = jsonCommon["Common"]["TestBounds"];

		CPoint pt;
		CSize size;
		LoadPointSizeFromJson(jsonOCR, UTF8fromUTF16(kTestBoundsName[index]), pt, size);
		rcBounds = CRect(pt, size);
		if (index == kEventNameBounds) {
			LoadPointSizeFromJson(jsonOCR, UTF8fromUTF16(kTestBoundsName[kEventNameIconBounds]), pt, size);
			rcIconBounds = CRect(pt, size);
		}

		//m_previewWindow.ChangeDragdropBounds(rcBounds);
	} else {
		rcBounds = m_previewWindow.GetDragdropActualBounds();
	}

	if (rcBounds.Size() == CSize()) {
		return 0;
	}

	const bool bManualThresOnly = ::GetKeyState(VK_CONTROL) < 0;	// ctrl でスライドバーOCRのみ
	const bool bNoAdjustBounds = ::GetKeyState(VK_SHIFT) < 0;	// shift でテキストを囲わない
	const bool bScale4 = ::GetKeyState(VK_MENU) < 0;		// atl で2倍拡大 "しない"

	// 切り抜き＆変換
	Utility::timer timer;

	Gdiplus::Bitmap bmp(image->GetWidth(), image->GetHeight(), PixelFormat24bppRGB);
	Gdiplus::Graphics graphics(&bmp);
	graphics.DrawImage(image, 0, 0);
	auto srcImage = GdiPlusBitmapToOpenCvMat(&bmp);//cv::imread(ssPath.string());
	if (index != kDirect) {	// kDirect 以外は 変更が必要
		rcBounds = AdjustBounds(srcImage, rcBounds);
		rcIconBounds = AdjustBounds(srcImage, rcIconBounds);
	}

	cv::Mat cutImage(srcImage, cvRectFromCRect(rcBounds));

	if (index == kEventNameBounds /*|| index == kCurrentMenuBounds*/) {
		if (!bNoAdjustBounds) {	// テキストを正確囲む
			// アイコン処理
			if (IsEventNameIcon(srcImage, rcIconBounds)) {	// アイコンが存在した場合、認識範囲を右にずらす
				enum { kIconTextMargin = 8 };
				rcBounds.left += rcIconBounds.Width() + kIconTextMargin;
			}

			CRect rcAdjustTextBounds = GetTextBounds(cutImage, rcBounds);
			rcBounds = rcAdjustTextBounds;

			cutImage = cv::Mat(srcImage, cvRectFromCRect(rcBounds));
		}
	} else if (index == kEventBottomOptionBounds) {
		CRect rcAdjustTextBounds = GetTextBounds(cutImage, rcBounds);
		rcBounds = rcAdjustTextBounds;

		cutImage = cv::Mat(srcImage, cvRectFromCRect(rcBounds));

	} else if (index == kEventNameIconBounds) {
		bool isIcon = IsEventNameIcon(srcImage, rcBounds);
		CString result;
		result.Format(L"isIcon : %s", isIcon ? L"true" : L"false");
		m_editResult.SetWindowTextW(result);
		m_previewWindow.ChangeDragdropBounds(rcBounds);
		return 0;
	}
	m_previewWindow.ChangeDragdropBounds(rcBounds);

	cv::imwrite((GetExeDirectory() / L"cutImage.png").string().c_str(), cutImage);

	if (index == kURACurrentTurnBounds || index == kAoharuCurrentTurnBounds || ::GetKeyState(VK_RWIN) < 0) {
		//cv::imwrite((GetExeDirectory() / L"test.png").string().c_str(), cutImage);
		//cv::Mat cutImage;
		//cutImage = cv::imread((GetExeDirectory() / L"test.png").string().c_str());
		cv::Mat hsvImage;
		cv::cvtColor(cutImage, hsvImage, cv::COLOR_BGR2HSV);

		const double scale = 2.0;
		cv::Mat resizedImage;
		cv::resize(hsvImage, resizedImage, cv::Size(), scale, scale, cv::INTER_LINEAR/*INTER_CUBIC*/);

		//int h_min = 12;
		//int h_max = 13;
		//int s_min = 75;
		//int s_max = 255;
		//int v_min = 100;
		//int v_max = 180;
		cv::Mat textImage;
		cv::inRange(resizedImage, cv::Scalar(h_min, s_min, v_min), cv::Scalar(h_max, s_max, v_max), textImage);

		cv::Mat invertedTextImage;
		cv::bitwise_not(textImage, invertedTextImage);	// 白背景化

		cv::imshow("1", invertedTextImage);
		cv::imshow("2", resizedImage);
		cv::imshow("3", textImage);

		CString result;
		result.AppendFormat(L"1: %s", TextFromImage(invertedTextImage).c_str());

		m_editResult.SetWindowTextW(result);
		return 0;
	}

	for (int i = 0; i < 2; ++i) {
		CEdit edit = m_editResult;
		if (i == 1) {
			if (bScale4) {
				break;	// 拡大OCRは実行しない
			}
			edit = m_editResult2;
		}
		cv::Mat resizedImage;
		//constexpr double scale = 4.0;
		double scale = 1.0;//4.0;
		//if (bScale4) {
		if (i == 1) {
			scale = 2.0;
		}
		cv::resize(cutImage, resizedImage, cv::Size(), scale, scale, cv::INTER_CUBIC);
		//auto saveImagePath = GetExeDirectory() / L"debug1.png";
		//cv::imwrite(saveImagePath.string(), resizedImage);

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

		ATLTRACE(L"切り抜き変換 %s\n", UTF16fromUTF8(timer.format()).c_str());

		std::function<std::wstring(cv::Mat)> funcTextFromImage = TextFromImage;
		//cv::imwrite((GetExeDirectory() / L"test.png").string().c_str(), cutImage);

		cv::imshow("1", cutImage);
		cv::imshow("2", resizedImage);
		cv::imshow("3", grayImage);
		cv::imshow("4", invertedImage);
		cv::imshow("5", thresImage);
		cv::imshow("6", thresImage2);

		// asyncに渡す関数オブジェクト
		auto asyncTextFromImage = [this](cv::Mat& image, std::shared_ptr<TextFromImageFunc> funcTextFromImage) -> std::wstring {
			std::wstring text = (*funcTextFromImage)(image);
			return text;
		};
		std::vector<std::future<std::wstring>> TextFromImageFutureList;
		
		TextFromImageFutureList.emplace_back(
			std::async(std::launch::async, asyncTextFromImage, cutImage, GetOCRFunction()));
		TextFromImageFutureList.emplace_back(
			std::async(std::launch::async, asyncTextFromImage, resizedImage, GetOCRFunction()));
		TextFromImageFutureList.emplace_back(
			std::async(std::launch::async, asyncTextFromImage, grayImage, GetOCRFunction()));
		TextFromImageFutureList.emplace_back(
			std::async(std::launch::async, asyncTextFromImage, invertedImage, GetOCRFunction()));
		TextFromImageFutureList.emplace_back(
			std::async(std::launch::async, asyncTextFromImage, thresImage, GetOCRFunction()));
		TextFromImageFutureList.emplace_back(
			std::async(std::launch::async, asyncTextFromImage, thresImage2, GetOCRFunction()));

		CString result;
		if (!bManualThresOnly) {
			result += L"1: " + CString(TextFromImageFutureList[0].get().c_str()) + L"\r\n";
			result += L"2: " + CString(TextFromImageFutureList[1].get().c_str()) + L"\r\n";
			result += L"3: " + CString(TextFromImageFutureList[2].get().c_str()) + L"\r\n";
			result += L"4: " + CString(TextFromImageFutureList[3].get().c_str()) + L"\r\n";
			result += L"5: " + CString(TextFromImageFutureList[4].get().c_str()) + L"\r\n";
			result += L"6: " + CString(TextFromImageFutureList[5].get().c_str()) + L"\r\n";
		}

		const int threshold = m_sliderThreshold.GetPos();
		cv::Mat manualThresholdImage;
		if (threshold > 0) {
			cv::threshold(grayImage, manualThresholdImage, threshold, 255.0, cv::THRESH_BINARY);
			cv::imshow("7", manualThresholdImage);

			result += L"7: " + CString(funcTextFromImage(manualThresholdImage).c_str()) + L" thres:" + std::to_wstring(threshold).c_str() + L"\r\n";
		}

		edit.SetWindowTextW(result);

	}

	return LRESULT();
}
