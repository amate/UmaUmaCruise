#include "stdafx.h"
#include "UmaTextRecognizer.h"

#include <boost\algorithm\string\trim.hpp>
#include <boost\algorithm\string\replace.hpp>

#include <opencv2\opencv.hpp>

#include "Utility\CodeConvert.h"
#include "Utility\CommonUtility.h"
#include "Utility\json.hpp"
#include "Utility\timer.h"
#include "Utility\Logger.h"
#include "Utility\GdiplusUtil.h"

#include "TesseractWrapper.h"
using namespace TesseractWrapper;

using json = nlohmann::json;
using namespace CodeConvert;
using namespace cv;

void	LoadPointSizeFromJson(const json& json, const std::string& key, CPoint& pt, CSize& size)
{
	pt.x = json[key]["x"];
	pt.y = json[key]["y"];
	size.cx = json[key]["width"];
	size.cy = json[key]["height"];
}

void	SavePointSizeFromJson(json& json, const std::string& key, const CPoint& pt, const CSize& size)
{
	json[key]["x"] = pt.x;
	json[key]["y"] = pt.y;
	json[key]["width"] = size.cx;
	json[key]["height"] = size.cy;
}

cv::Mat GdiPlusBitmapToOpenCvMat(Gdiplus::Bitmap* bmp)
{
	auto format = bmp->GetPixelFormat();
	//if (format != PixelFormat24bppRGB)
	//	return cv::Mat();

	int wd = bmp->GetWidth();
	int hgt = bmp->GetHeight();
	Gdiplus::Rect rcLock(0, 0, wd, hgt);
	Gdiplus::BitmapData bmpData;

	if (!bmp->LockBits(&rcLock, Gdiplus::ImageLockModeRead, format, &bmpData) == Gdiplus::Ok)
		return cv::Mat();

	cv::Mat mat = cv::Mat(hgt, wd, CV_8UC3, static_cast<unsigned char*>(bmpData.Scan0), bmpData.Stride).clone();

	bmp->UnlockBits(&bmpData);
	return mat;
}

cv::Rect	cvRectFromCRect(const CRect& rcBounds)
{
	return cv::Rect(rcBounds.left, rcBounds.top, rcBounds.Width(), rcBounds.Height());
}

// ===============================================================================


// cutImage にあるテキストを囲う範囲を調べて返す
// cutImageは黒背景に白文字である必要がある
// 今のところイベント名の時だけ使ってる
CRect GetTextBounds(cv::Mat cutImage, const CRect& rcBounds)
{
	//cv::Mat resizedImage;
	//constexpr double scale = 1.0;//4.0;
	//cv::resize(cutImage, resizedImage, cv::Size(), scale, scale, cv::INTER_CUBIC);
	//auto saveImagePath = GetExeDirectory() / L"debug1.png";
	//cv::imwrite(saveImagePath.string(), resizedImage);

	cv::Mat grayImage;
	cv::cvtColor(cutImage/*resizedImage*/, grayImage, cv::COLOR_RGB2GRAY);

	cv::Mat thresImage;
	cv::threshold(grayImage, thresImage, 0.0, 255.0, cv::THRESH_OTSU);

	int c = thresImage.channels();
	CPoint ptLT = { thresImage.cols , thresImage.rows };
	CPoint ptRB = { 0, 0 };
	for (int y = 0; y < thresImage.rows; y++) {
		for (int x = 0; x < thresImage.cols; x++) {
			uchar val = thresImage.at<uchar>(y, x);
			if (val >= 255) {
				// 白が見つかった時点で一番小さい地点を探す
				ptLT.y = std::min(ptLT.y, (LONG)y);
				ptLT.x = std::min(ptLT.x, (LONG)x);
				// 白が見つかった時点で一番大きい地点を探す
				ptRB.y = std::max(ptRB.y, (LONG)y);
				ptRB.x = std::max(ptRB.x, (LONG)x);
			}
		}
	}
	enum { 
		kTextMargin = 5 
	};
	CRect rcAdjustTextBounds(ptLT, ptRB);
	//rcAdjustTextBounds.top /= scale;
	//rcAdjustTextBounds.left /= scale;
	//rcAdjustTextBounds.right /= scale;
	//rcAdjustTextBounds.bottom /= scale;
	rcAdjustTextBounds.MoveToXY(rcBounds.left + rcAdjustTextBounds.left, rcBounds.top + rcAdjustTextBounds.top);

	rcAdjustTextBounds.InflateRect(kTextMargin, kTextMargin, kTextMargin, kTextMargin);	// 膨らませる
	ATLASSERT(rcAdjustTextBounds.Width() > 0 && rcAdjustTextBounds.Height() > 0);
	rcAdjustTextBounds.NormalizeRect();
	return rcAdjustTextBounds;
}

// ==========================================================================

bool UmaTextRecognizer::LoadSetting()
{
	std::ifstream ifs((GetExeDirectory() / L"Common.json").string());
	ATLASSERT(ifs);
	if (!ifs) {
		ERROR_LOG << L"LoadSetting failed";
		return false;
	}
	json jsonCommon;
	ifs >> jsonCommon;

	m_targetWindowName = UTF16fromUTF8(jsonCommon["Common"]["TargetWindow"]["WindowName"].get<std::string>()).c_str();
	m_targetClassName = UTF16fromUTF8(jsonCommon["Common"]["TargetWindow"]["ClassName"].get<std::string>()).c_str();

	json jsonTestBounds = jsonCommon["Common"]["TestBounds"];

	m_baseClientSize.cx = jsonTestBounds["BaseClientSize"]["width"];
	m_baseClientSize.cy = jsonTestBounds["BaseClientSize"]["height"];

	for (int i = 0; i < kMaxCount; ++i) {
		CPoint pt;
		CSize size;
		LoadPointSizeFromJson(jsonTestBounds, UTF8fromUTF16(kTestBoundsName[i]), pt, size);
		CRect rcBounds = CRect(pt, size);
		m_testBounds[i] = rcBounds;
	}

	m_kCurrentTurnThreshold = jsonCommon["Common"]["ImageProcess"]["CurrentTurnThreshold"];

	for (const json& jsonTypo : jsonCommon["TypoDictionary"]) {
		std::wstring typo = UTF16fromUTF8(jsonTypo["Typo"].get<std::string>());
		std::wstring correct = UTF16fromUTF8(jsonTypo["Corret"].get<std::string>());
		m_typoDictionary[typo] = correct;
	}

	return true;
}

std::unique_ptr<Gdiplus::Bitmap> UmaTextRecognizer::ScreenShot()
{
	HWND hwndTarget = ::FindWindow(m_targetClassName, m_targetWindowName);
	if (!hwndTarget) {
		return nullptr;
	}

	CWindowDC dc(NULL/*hWndTarget*/);	// desktop

	CRect rcWindow;
	::GetWindowRect(hwndTarget, &rcWindow);

	CRect rcClient;
	::GetClientRect(hwndTarget, rcClient);

	CRect rcAdjustClient = rcWindow;
	const int topMargin = (rcWindow.Height() - rcClient.Height() - GetSystemMetrics(SM_CXFRAME) * 2 - GetSystemMetrics(SM_CYCAPTION)) / 2;
	rcAdjustClient.top += GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CYCAPTION) + topMargin;
	rcAdjustClient.left += (rcWindow.Width() - rcClient.Width()) / 2;
	rcAdjustClient.right = rcAdjustClient.left + rcClient.right;
	rcAdjustClient.bottom = rcAdjustClient.top + rcClient.bottom;

	CDC dcMemory;
	dcMemory.CreateCompatibleDC(dc);

	LPVOID           lp;
	BITMAPINFO       bmi;
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
	return std::unique_ptr<Gdiplus::Bitmap>(Gdiplus::Bitmap::FromHBITMAP(hbmp, NULL));
}

bool UmaTextRecognizer::TextRecognizer(Gdiplus::Bitmap* image)
{
	m_umaMusumeName.clear();
	m_currentTurn.clear();
	m_eventName.clear();

	std::unique_ptr<Gdiplus::Bitmap> bmpHolder;
	if (!image) {
		bmpHolder = ScreenShot();
		if (!bmpHolder) {
			return false;
		}
		image = bmpHolder.get();
	}

	cv::Mat srcImage = GdiPlusBitmapToOpenCvMat(image);
	if (srcImage.empty()) {
		ATLASSERT(FALSE);
		ERROR_LOG << L"GdiPlusBitmapToOpenCvMat failed";
		return false;
	}

	{	// 育成ウマ娘名
		CRect rcSubName = _AdjustBounds(srcImage, m_testBounds[kUmaMusumeSubNameBounds]);
		CRect rcName = _AdjustBounds(srcImage, m_testBounds[kUmaMusumeNameBounds]);

		std::vector<std::wstring>	subNameList;
		std::vector<std::wstring>	nameList;

		auto funcImageToText = [this, &srcImage](int testBoundsIndex, std::vector<std::wstring>& list) {
			CRect rcName = _AdjustBounds(srcImage, m_testBounds[testBoundsIndex]);
			cv::Mat cutImage(srcImage, cvRectFromCRect(rcName));

			cv::Mat resizedImage;
			constexpr double scale = 4.0;
			cv::resize(cutImage, resizedImage, cv::Size(), scale, scale, cv::INTER_CUBIC);

			cv::Mat grayImage;
			cv::cvtColor(resizedImage, grayImage, cv::COLOR_RGB2GRAY);

			cv::Mat thresImage;
			cv::threshold(grayImage, thresImage, 0.0, 255.0, cv::THRESH_OTSU);

			std::wstring thresImageText = TextFromImage(thresImage);
			list.emplace_back(thresImageText);
		};
		funcImageToText(kUmaMusumeSubNameBounds, subNameList);
		funcImageToText(kUmaMusumeNameBounds, nameList);

		const size_t size = subNameList.size();
		for (int i = 0; i < size; ++i) {
			std::wstring umamusumeName = subNameList[i] + nameList[i];
			m_umaMusumeName.emplace_back(umamusumeName);
		}
	}
	{	// イベント名
		CRect rcEventName = _AdjustBounds(srcImage, m_testBounds[kEventNameBounds]);
		if (_IsEventNameIcon(srcImage)) {	// アイコンが存在した場合、認識範囲を右にずらす
			enum { kIconTextMargin = 5 };
			rcEventName.left = _AdjustBounds(srcImage, m_testBounds[kEventNameIconBounds]).right + kIconTextMargin;
		}
		// テキストを囲う範囲を見つける
		cv::Mat cutImage(srcImage, cvRectFromCRect(rcEventName));
		CRect rcAdjustTextBounds = GetTextBounds(cutImage, rcEventName);

		// テキストを正確に囲ったイメージを切り出す
		cv::Mat cutImage2(srcImage, cvRectFromCRect(rcAdjustTextBounds));
		//cv::imshow("cutImage2", cutImage2);
		// 
		//cv::Mat resizedImage;
		//constexpr double scale = 4.0;
		//cv::resize(cutImage, resizedImage, cv::Size(), scale, scale, cv::INTER_CUBIC);

		cv::Mat grayImage;
		cv::cvtColor(cutImage2/*resizedImage*/, grayImage, cv::COLOR_RGB2GRAY);

		cv::Mat invertedImage;
		cv::bitwise_not(grayImage, invertedImage);

		cv::Mat thresImage;
		cv::threshold(grayImage, thresImage, 0.0, 255.0, cv::THRESH_OTSU);

		auto funcPushBackImageText = [this](cv::Mat& image) {
			std::wstring text = TextFromImage(image);

			// typo を正誤表で変換
			auto itFound = m_typoDictionary.find(text);
			if (itFound != m_typoDictionary.end()) {
				text = itFound->second;
			}
			// '！' が '7' に誤認識されてしまうっぽいので置換して候補に追加しておく
			if (text.find(L"7") != std::wstring::npos) {
				std::wstring replacedText = boost::algorithm::replace_all_copy(text, L"7", L"！");
				m_eventName.emplace_back(replacedText);
			}
			m_eventName.emplace_back(text);
		};

		funcPushBackImageText(cutImage);
		funcPushBackImageText(grayImage);
		funcPushBackImageText(invertedImage);
		funcPushBackImageText(thresImage);
	}
	{	// 現在の日付
		CRect rcTurnBounds = _AdjustBounds(srcImage, m_testBounds[kCurrentTurnBounds]);
		cv::Mat cutImage(srcImage, cvRectFromCRect(rcTurnBounds));

		//cv::Mat resizedImage;
		//constexpr double scale = 4.0;
		//cv::resize(cutImage, resizedImage, cv::Size(), scale, scale, cv::INTER_CUBIC);

		cv::Mat grayImage;
		cv::cvtColor(cutImage, grayImage, cv::COLOR_RGB2GRAY);

		cv::Mat thresImage;
		cv::threshold(grayImage, thresImage, 0.0, 255.0, cv::THRESH_OTSU);
		
		cv::Mat manualThresholdImage;
		cv::threshold(grayImage, manualThresholdImage, m_kCurrentTurnThreshold, 255.0, cv::THRESH_BINARY);

		std::wstring manualThresImageText = TextFromImage(manualThresholdImage);
		m_currentTurn.emplace_back(manualThresImageText);	// 優先

		std::wstring cutImageText = TextFromImage(cutImage);
		m_currentTurn.emplace_back(cutImageText);

		std::wstring thresImageText = TextFromImage(thresImage);
		m_currentTurn.emplace_back(thresImageText);

		//INFO_LOG << L"CurrentTurn, cut: " << cutImageText << L" thres: " << thresImageText;
	}
	{	// 現在メニュー[育成/トレーニング]
		CRect rcCurrentMenuBounds = _AdjustBounds(srcImage, m_testBounds[kCurrentMenuBounds]);
		cv::Mat cutImage(srcImage, cvRectFromCRect(rcCurrentMenuBounds));

		std::wstring cutImageText = TextFromImage(cutImage);
		m_currentMenu = cutImageText;
	}

	return true;
}

CRect UmaTextRecognizer::_AdjustBounds(const cv::Mat& srcImage, CRect bounds)
{
	//CSize imageSize(static_cast<int>(image->GetWidth()), static_cast<int>(image->GetHeight()));
	CSize imageSize(srcImage.size().width, srcImage.size().height);
	const double Xratio = static_cast<double>(imageSize.cx) / m_baseClientSize.cx;
	const double Yratio = static_cast<double>(imageSize.cy) / m_baseClientSize.cy;

	CRect adjustRect = bounds;
	adjustRect.top *= Yratio;
	adjustRect.left *= Xratio;
	adjustRect.right *= Xratio;
	adjustRect.bottom *= Yratio;

	return adjustRect;
}

bool UmaTextRecognizer::_IsEventNameIcon(cv::Mat srcImage)
{
	const CRect rcIcon = _AdjustBounds(srcImage, m_testBounds[kEventNameIconBounds]);

	cv::Mat cutImage(srcImage, cvRectFromCRect(rcIcon));

	cv::Mat grayImage;
	cv::cvtColor(cutImage, grayImage, cv::COLOR_RGB2GRAY);

	cv::Mat thresImage;
	cv::threshold(grayImage, thresImage, 0.0, 255.0, cv::THRESH_OTSU);

	int c = grayImage.channels();
	ATLASSERT(c == 1);

	// 画素中の白の数を数える
	int whilteCount = 0;
	for (int y = 0; y < thresImage.rows; y++) {
		for (int x = 0; x < thresImage.cols; x++) {
			uchar val = thresImage.at<uchar>(y, x);
			if (val >= 255) {
				++whilteCount;
			}
		}
	}
	const int imagePixelCount = thresImage.rows * thresImage.cols;
	const double whiteRatio = static_cast<double>(whilteCount) / imagePixelCount;
	bool isIcon = whiteRatio > kEventNameIconThreshold;	// 白の比率が一定以上ならアイコンとみなす
	return isIcon;
}
