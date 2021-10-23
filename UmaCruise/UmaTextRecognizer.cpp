#include "stdafx.h"
#include "UmaTextRecognizer.h"

#include <future>
#include <list>
#include <regex>

#include <boost\algorithm\string\trim.hpp>
#include <boost\algorithm\string\replace.hpp>
#include <boost\optional.hpp>

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

using SetThreadDpiAwarenessContextFunc = DPI_AWARENESS_CONTEXT(*)(DPI_AWARENESS_CONTEXT);
extern SetThreadDpiAwarenessContextFunc	g_funcSetThreadDpiAwarenessContext;


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
	if (format != PixelFormat24bppRGB) {
		ERROR_LOG << L"GdiPlusBitmapToOpenCvMat: format != PixelFormat24bppRGB";
		ATLASSERT(FALSE);
		return cv::Mat();
	}

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

bool	CheckCutBounds(const cv::Mat& img, const cv::Rect& rcCut, std::wstring comment)
{
	if (rcCut.x < 0 || rcCut.y < 0
		|| img.size().width < rcCut.x + rcCut.width || img.size().height < rcCut.y + rcCut.height) {
		ERROR_LOG << L"invalidate cute bounds: " << comment 
			<< L"src width: " << img.size().width << L" height: " << img.size().height 
			<< L" rcCut x: " << rcCut.x << L" y: " << rcCut.y << L" width: " << rcCut.width << L" height: " << rcCut.height;
		ATLASSERT(FALSE);
		return false;
	}
	return true;	
}


double ImageWhiteRatio(cv::Mat thresImage)
{
	int c = thresImage.channels();
	ATLASSERT(c == 1);

	// 画素中の白の数を数える
	int whilteCount = cv::countNonZero(thresImage);
	//int whilteCount = 0;
	//for (int y = 0; y < thresImage.rows; y++) {
	//	for (int x = 0; x < thresImage.cols; x++) {
	//		uchar val = thresImage.at<uchar>(y, x);
	//		if (val >= 255) {
	//			++whilteCount;
	//		}
	//	}
	//}
	const int imagePixelCount = thresImage.rows * thresImage.cols;
	const double whiteRatio = static_cast<double>(whilteCount) / imagePixelCount;
	return whiteRatio;
}


// ===============================================================================


// cutImage にあるテキストを囲う範囲を調べて返す
CRect GetTextBounds(cv::Mat cutImage, const CRect& rcBounds)
{
	cv::Mat grayImage;
	cv::cvtColor(cutImage/*resizedImage*/, grayImage, cv::COLOR_RGB2GRAY);

	cv::Mat thresImage;
	cv::threshold(grayImage, thresImage, 0.0, 255.0, cv::THRESH_OTSU);

	int c = thresImage.channels();
	ATLASSERT(c == 1);
	uchar zeroPointVal = thresImage.at<uchar>(0, 0);	// 背景色確認
	ATLASSERT(zeroPointVal == 0 || zeroPointVal == 255);

	CPoint ptLT = { thresImage.cols , thresImage.rows };
	CPoint ptRB = { 0, 0 };
	for (int y = 0; y < thresImage.rows; y++) {
		for (int x = 0; x < thresImage.cols; x++) {
			uchar val = thresImage.at<uchar>(y, x);
			if (val != zeroPointVal) {	// 背景色以外なら
				// 見つかった時点で一番小さい地点を探す
				ptLT.y = std::min(ptLT.y, (LONG)y);
				ptLT.x = std::min(ptLT.x, (LONG)x);
				// 見つかった時点で一番大きい地点を探す
				ptRB.y = std::max(ptRB.y, (LONG)y);
				ptRB.x = std::max(ptRB.x, (LONG)x);
			}
		}
	}
	enum { 
		kTextMargin = 5 
	};
	CRect rcAdjustTextBounds(ptLT, ptRB);
	rcAdjustTextBounds.MoveToXY(rcBounds.left + rcAdjustTextBounds.left, rcBounds.top + rcAdjustTextBounds.top);

	rcAdjustTextBounds.InflateRect(kTextMargin, kTextMargin, kTextMargin, kTextMargin);	// 膨らませる
	//ATLASSERT(rcAdjustTextBounds.Width() > 0 && rcAdjustTextBounds.Height() > 0);
	rcAdjustTextBounds.NormalizeRect();
	return rcAdjustTextBounds;
}

// ==========================================================================
// UmaTextRecognizer

bool UmaTextRecognizer::LoadSetting()
{
	std::ifstream ifs((GetExeDirectory() / L"UmaLibrary" / L"Common.json").wstring());
	ATLASSERT(ifs);
	if (!ifs) {
		ERROR_LOG << L"LoadSetting failed";
		return false;
	}
	json jsonCommon;
	ifs >> jsonCommon;

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

	const json& jsonHSVBounds = jsonCommon["Common"]["ImageProcess"]["HSV_TextColorBounds"];
	m_kHSVTextBounds.h_min = jsonHSVBounds[0][0];
	m_kHSVTextBounds.h_max = jsonHSVBounds[0][1];
	m_kHSVTextBounds.s_min = jsonHSVBounds[1][0];
	m_kHSVTextBounds.s_max = jsonHSVBounds[1][1];
	m_kHSVTextBounds.v_min = jsonHSVBounds[2][0];
	m_kHSVTextBounds.v_max = jsonHSVBounds[2][1];

	for (const json& jsonTypo : jsonCommon["TypoDictionary"]) {
		std::wstring typo = UTF16fromUTF8(jsonTypo["Typo"].get<std::string>());
		std::wstring correct = UTF16fromUTF8(jsonTypo["Corret"].get<std::string>());
		m_typoDictionary[typo] = correct;
	}

	return true;
}


// 育成ウマ娘名[能力詳細]
std::wstring UmaTextRecognizer::GetIkuseiUmaMusumeName(Gdiplus::Bitmap* image)
{	
	if (!image) {
		return L"";
	}
	cv::Mat srcImage = GdiPlusBitmapToOpenCvMat(image);
	if (srcImage.empty()) {
		ATLASSERT(FALSE);
		ERROR_LOG << L"GdiPlusBitmapToOpenCvMat failed";
		return L"";
	}

	//Utility::timer timer;

	auto funcImageToText = [&, this](int testBoundsIndex, std::wstring& recognizerText) {
		CRect rcName = _AdjustBounds(srcImage, m_testBounds[testBoundsIndex]);
		if (!CheckCutBounds(srcImage, cvRectFromCRect(rcName), L"rcName")) {
			return;
		}
		//Utility::timer timer;
		cv::Mat cutImage(srcImage, cvRectFromCRect(rcName));
		cv::Mat textImage = _InRangeHSVTextColorBounds(cutImage);

		// 画像における白文字率を確認して、一定比率以下のときは無視する
		const double whiteRatio = ImageWhiteRatio(textImage);
		if (whiteRatio > kMinWhiteTextRatioThreshold) {
			cv::Mat invertedTextImage;
			cv::bitwise_not(textImage, invertedTextImage);	// 白背景化

			std::wstring invertedText = TextFromImage(invertedTextImage);
			recognizerText = invertedText;
		}
	};

	std::wstring	subName;
	std::wstring	name;

	funcImageToText(kUmaMusumeNameBounds, name);
	if (name.size()) {
		funcImageToText(kUmaMusumeSubNameBounds, subName);
		if (subName.size()) {
			std::wstring ikuseUmaMusumeName = subName + name;
			return ikuseUmaMusumeName;
		}
	}
	return L"";
	//INFO_LOG << L"・育成ウマ娘[能力詳細] " << timer.format();
}

bool UmaTextRecognizer::TextRecognizer(Gdiplus::Bitmap* image)
{
	Utility::timer timer;

	m_umaMusumeName.clear();
	m_currentTurn.clear();
	m_eventName.clear();
	m_eventBottomOption.clear();

	if (!image) {
		return false;
	}

	cv::Mat srcImage = GdiPlusBitmapToOpenCvMat(image);
	if (srcImage.empty()) {
		ATLASSERT(FALSE);
		ERROR_LOG << L"GdiPlusBitmapToOpenCvMat failed";
		return false;
	}
	INFO_LOG << L"TextRecognizer start! >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>";
	std::list<std::future<std::wstring>> TextFromImageFutureList;
	std::list<std::future<std::wstring>> eventBottomOptionFutureList;


	{	// イベント名
		//INFO_LOG << L"・イベント名";

		// asyncに渡す関数オブジェクト
		auto asyncTextFromImage = [this](cv::Mat image, std::shared_ptr<TextFromImageFunc> funcTextFromImage) -> std::wstring {
			std::wstring text = (*funcTextFromImage)(image);
			return text;
		};

		CRect rcEventName = _AdjustBounds(srcImage, m_testBounds[kEventNameBounds]);
		if (_IsEventNameIcon(srcImage)) {	// アイコンが存在した場合、認識範囲を右にずらす
			enum { kIconTextMargin = 5 };
			rcEventName.left = _AdjustBounds(srcImage, m_testBounds[kEventNameIconBounds]).right + kIconTextMargin;
		}
		// テキストを囲う範囲を見つける
		if (!CheckCutBounds(srcImage, cvRectFromCRect(rcEventName), L"rcEventName")) {
			return false;
		}
		cv::Mat cutImage(srcImage, cvRectFromCRect(rcEventName));
		CRect rcAdjustTextBounds = GetTextBounds(cutImage, rcEventName);

		// テキストを正確に囲ったイメージを切り出す
		if (!CheckCutBounds(srcImage, cvRectFromCRect(rcAdjustTextBounds), L"rcAdjustTextBounds")) {
			return false;
		}

		cv::Mat cutImage2(srcImage, cvRectFromCRect(rcAdjustTextBounds));

		cv::Mat grayImage;		// グレースケール
		cv::cvtColor(cutImage2/*resizedImage*/, grayImage, cv::COLOR_RGB2GRAY);

		cv::Mat invertedImage;	// 反転
		cv::bitwise_not(grayImage, invertedImage);

		cv::Mat resizedImage;	// 拡大
		cv::resize(invertedImage, resizedImage, cv::Size(), kResizeScale, kResizeScale, cv::INTER_CUBIC);	// 4

		cv::Mat thresImage2;	// 閾値
		//cv::threshold(grayImage, thresImage, 190.0, 255.0, cv::THRESH_BINARY_INV);
		cv::threshold(resizedImage, thresImage2, 0.0, 255.0, cv::THRESH_OTSU);			// 6

		
		TextFromImageFutureList.emplace_back(
				std::async(std::launch::async, asyncTextFromImage, cutImage2, GetOCRFunction()));
		TextFromImageFutureList.emplace_back(
			std::async(std::launch::async, asyncTextFromImage, resizedImage, GetOCRFunction()));
		TextFromImageFutureList.emplace_back(
			std::async(std::launch::async, asyncTextFromImage, thresImage2, GetOCRFunction()));

		// イベント選択肢
		{
			//INFO_LOG << L"・イベント選択肢";
			CRect rcEventBottomOption = _AdjustBounds(srcImage, m_testBounds[kEventBottomOptionBounds]);
			cv::Mat cutImage(srcImage, cvRectFromCRect(rcEventBottomOption));

			CRect rcAdjustTextBounds3 = GetTextBounds(cutImage, rcEventBottomOption);
			cv::Mat cutImage3(srcImage, cvRectFromCRect(rcAdjustTextBounds3));

			cv::Mat textImage = _InRangeHSVTextColorBounds(cutImage3);
			// 画像における白文字率を確認して、一定比率以下のときは無視する
			const double whiteRatio = ImageWhiteRatio(textImage);
			if (whiteRatio > kMinWhiteTextRatioThreshold) {
				cv::Mat grayImage;
				cv::cvtColor(cutImage3, grayImage, cv::COLOR_RGB2GRAY);

				cv::Mat resizedImage;
				cv::resize(grayImage, resizedImage, cv::Size(), kResizeScale, kResizeScale, cv::INTER_CUBIC);

				cv::Mat thresImage;
				cv::threshold(resizedImage, thresImage, 0.0, 255.0, cv::THRESH_OTSU);

				cv::Mat invertedTextImage;
				cv::bitwise_not(textImage, invertedTextImage);	// 白背景化

				eventBottomOptionFutureList.emplace_back(
					std::async(std::launch::async, asyncTextFromImage, thresImage, GetOCRFunction()));

				eventBottomOptionFutureList.emplace_back(
					std::async(std::launch::async, asyncTextFromImage, invertedTextImage, GetOCRFunction()));
			}
		}
	}
	{	// 現在の日付

		auto funcReadCurrentTurn = [&](int currentTurnBounds) {
			Utility::timer timer;
			CRect rcTurnBounds = _AdjustBounds(srcImage, m_testBounds[currentTurnBounds]);
			if (!CheckCutBounds(srcImage, cvRectFromCRect(rcTurnBounds), L"rcTurnBounds")) {
				return ;
			}
			cv::Mat cutImage(srcImage, cvRectFromCRect(rcTurnBounds));
			cv::Mat textImage = _InRangeHSVTextColorBounds(cutImage);

			std::wstring invertedText;
			// 画像における白文字率を確認して、一定比率以下のときは無視する
			const double whiteRatio = ImageWhiteRatio(textImage);
			if (whiteRatio > kMinWhiteTextRatioThreshold) {
				cv::Mat invertedTextImage;
				cv::bitwise_not(textImage, invertedTextImage);	// 白背景化

				invertedText = TextFromImage(invertedTextImage);
				m_currentTurn.emplace_back(invertedText);
#if 0
				cv::Mat resizedImage;
				constexpr double scale = 2.0;
				cv::resize(cutImage, resizedImage, cv::Size(), scale, scale, cv::INTER_CUBIC);

				std::wstring resizedText = TextFromImage(resizedImage);
				m_currentTurn.emplace_back(resizedText);
#endif
				//INFO_LOG << L"CurrentTurn, cut: " << cutImageText << L" thres: " << thresImageText;
			}
			INFO_LOG << L"・現在の日付 " << timer.format() << L" (" << invertedText << L")";
		};

		funcReadCurrentTurn(kURACurrentTurnBounds);
		funcReadCurrentTurn(kAoharuCurrentTurnBounds);
	}
	{	// 育成ウマ娘名[育成ウマ娘選択]
		Utility::timer timer;

		// 現在の日付が見えない時だけ判定をする
		bool bCurrentTurnVisible = false;
		std::wregex rx(LR"((ジュニア|クラシック|シニア|ファイナルズ))");
		for (const std::wstring currentTurn : m_currentTurn) {
			if (std::regex_search(currentTurn, rx)) {
				bCurrentTurnVisible = true;
				break;
			}
		}
		if (!bCurrentTurnVisible) {
			auto funcImageToUmaMusumeText = [&, this](int testBoundsIndex) -> std::wstring {
				CRect rcName = _AdjustBounds(srcImage, m_testBounds[testBoundsIndex]);
				if (!CheckCutBounds(srcImage, cvRectFromCRect(rcName), L"rcName")) {
					return L"";
				}

				cv::Mat cutImage(srcImage, cvRectFromCRect(rcName));

				cv::Mat resizedImage;
				cv::resize(cutImage, resizedImage, cv::Size(), kResizeScale, kResizeScale, cv::INTER_CUBIC);

				cv::Mat grayImage;
				cv::cvtColor(resizedImage, grayImage, cv::COLOR_RGB2GRAY);

				cv::Mat invertedImage;
				cv::bitwise_not(grayImage, invertedImage);

				cv::Mat thresImage2;
				cv::threshold(invertedImage, thresImage2, 0.0, 255.0, cv::THRESH_OTSU);

				std::wstring invertedThresImageText = TextFromImage(thresImage2);	// 6: 白背景黒文字(グレー反転閾値)
				return invertedThresImageText;
			};

			std::wstring subName = funcImageToUmaMusumeText(kIkuseiUmaMusumeSubNameBounds);
			if (subName.length()) {
				std::wstring name = funcImageToUmaMusumeText(kIkuseiUmaMusumeNameBounds);
				if (name.length()) {
					std::wstring umamusumeName = subName + name;
					m_umaMusumeName.emplace_back(umamusumeName);
					INFO_LOG << L"育成ウマ娘名[育成ウマ娘選択]: " << umamusumeName << L" " << timer.format();
				}
			}
		}
	}
	{	// 現在メニュー[トレーニング]
		Utility::timer timer;
		m_bTrainingMenu = false;

		CRect rcCurrentMenuBounds = _AdjustBounds(srcImage, m_testBounds[kCurrentMenuBounds]);
		if (!CheckCutBounds(srcImage, cvRectFromCRect(rcCurrentMenuBounds), L"rcCurrentMenuBounds")) {
			return false;
		}
		cv::Mat cutImage(srcImage, cvRectFromCRect(rcCurrentMenuBounds));
		cv::Mat grayImage;
		cv::cvtColor(cutImage, grayImage, cv::COLOR_RGB2GRAY);	// 3: グレー化

		std::wstring cutImageText = TextFromImage(grayImage);
		if (cutImageText == L"トレーニング") {
			CRect rcBackButtonBounds = _AdjustBounds(srcImage, m_testBounds[kBackButtonBounds]);
			if (!CheckCutBounds(srcImage, cvRectFromCRect(rcBackButtonBounds), L"rcBackButtonBounds")) {
				return false;
			}
			cv::Mat cutImage2(srcImage, cvRectFromCRect(rcBackButtonBounds));

			std::wstring cutImage2Text = TextFromImage(cutImage2);
			if (cutImage2Text == L"戻る") {
				m_bTrainingMenu = true;
			}
		}
		INFO_LOG << L"・トレーニング " << timer.format();;
	}
	{	// 育成トップ
		m_bIkuseiTop = false;
		CRect rcAbilityDetailBounds = _AdjustBounds(srcImage, m_testBounds[kAbilityDetailBounds]);
		if (!CheckCutBounds(srcImage, cvRectFromCRect(rcAbilityDetailBounds), L"rcAbilityDetailBounds")) {
			return false;
		}
		cv::Mat cutImage(srcImage, cvRectFromCRect(rcAbilityDetailBounds));
		cv::Mat textImage = _InRangeHSVTextColorBounds(cutImage);
		const double whiteRatio = ImageWhiteRatio(textImage);
		if (whiteRatio > kAbilityDetailThreshold) {
			m_bIkuseiTop = true;
		}
	}
	{
		// レース詳細
		Utility::timer timer;
		m_entryRaceDistance = 0;
		CRect rcRaceDetailBounds = _AdjustBounds(srcImage, m_testBounds[kRaceDetailBounds]);
		if (!CheckCutBounds(srcImage, cvRectFromCRect(rcRaceDetailBounds), L"rcRaceDetailBounds")) {
			return false;
		}
		cv::Mat cutImage(srcImage, cvRectFromCRect(rcRaceDetailBounds));
		cv::Mat textImage = _InRangeHSVTextColorBounds(cutImage);

		std::wstring invertedText;
		// 画像における白文字率を確認して、一定比率以下のときは無視する
		const double whiteRatio = ImageWhiteRatio(textImage);
		if (whiteRatio > kMinWhiteTextRatioThreshold) {
			cv::Mat invertedTextImage;
			cv::bitwise_not(textImage, invertedTextImage);	// 白背景化

			invertedText = TextFromImage(invertedTextImage);
		}
		if (invertedText.length()) {
			// [レース予約完了]ダイアログかどうか調べる
			CRect rcBackButtonBounds = _AdjustBounds(srcImage, m_testBounds[kBackButtonBounds]);
			if (!CheckCutBounds(srcImage, cvRectFromCRect(rcBackButtonBounds), L"rcBackButtonBounds")) {
				return false;
			}
			cv::Mat cutImage2(srcImage, cvRectFromCRect(rcBackButtonBounds));

			cv::Mat grayImage2;
			cv::cvtColor(cutImage2, grayImage2, cv::COLOR_RGB2GRAY);

			cv::Mat thresImage2;
			cv::threshold(grayImage2, thresImage2, kBackButtonExistThreshold, 255.0, cv::THRESH_BINARY_INV);

			const double whiteRatio = ImageWhiteRatio(thresImage2);
			// 戻るボタン が見つからなければ[レース予約完了]ダイアログではない
			if (whiteRatio < kMinWhiteTextRatioThreshold) {	
				INFO_LOG << L"・レース詳細 " << timer.format() << L" (" << invertedText << L")";

				std::wregex rx(LR"((\d+)m)");
				std::wsmatch result;
				if (std::regex_search(invertedText, result, rx)) {
					m_entryRaceDistance = std::stoi(result[1].str());
				}
			}
		}		
	}

	// text を適当に変換してから listへ追加する
	auto funcPushBackImageText = [this](std::wstring text, std::vector<std::wstring>& list) {

		// typo を正誤表で変換
		auto itFound = m_typoDictionary.find(text);
		if (itFound != m_typoDictionary.end()) {
			text = itFound->second;
		}
		// '！' が '7' に誤認識されてしまうっぽいので置換して候補に追加しておく
		if (text.find(L"7") != std::wstring::npos) {
			std::wstring replacedText = boost::algorithm::replace_all_copy(text, L"7", L"！");
			list.emplace_back(replacedText);
		}
		// '！' が '/' に誤認識されてしまうっぽいので置換して候補に追加しておく
		if (text.find(L"/") != std::wstring::npos) {
			std::wstring replacedText = boost::algorithm::replace_all_copy(text, L"/", L"！");
			list.emplace_back(replacedText);
		}
		// '?'を正規化
		if (text.find(L"?") != std::wstring::npos) {
			std::wstring replacedText = boost::algorithm::replace_all_copy(text, L"?", L"？");
			list.emplace_back(replacedText);
		}
		list.emplace_back(text);
	};

	// イベント名
	// async から戻ってきた値を取得する
	for (auto& future : TextFromImageFutureList) {
		funcPushBackImageText(future.get(), m_eventName);
	}
	// イベント選択肢
	for (auto& future : eventBottomOptionFutureList) {
		funcPushBackImageText(future.get(), m_eventBottomOption);
	}

	INFO_LOG << L"TextRecognizer finish! " << timer.format() << L"\n";
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
	if (kBlueBackgroundThreshold < blueRatio) {
		return false;	// サポートカードイベント
	}

	cv::Mat grayImage;
	cv::cvtColor(cutImage, grayImage, cv::COLOR_RGB2GRAY);

	cv::Mat thresImage;
	cv::threshold(grayImage, thresImage, kEventNameIconThreshold, 255.0, cv::THRESH_BINARY);

	const double whiteRatio = ImageWhiteRatio(thresImage);
	bool isIcon = whiteRatio > kEventNameIconWhiteRatioThreshold;	// 白の比率が一定以上ならアイコンとみなす
	return isIcon;
}

cv::Mat UmaTextRecognizer::_InRangeHSVTextColorBounds(cv::Mat cutImage)
{
	cv::Mat hsvImage;
	cv::cvtColor(cutImage, hsvImage, cv::COLOR_BGR2HSV);

	cv::Mat resizedImage;
	cv::resize(hsvImage, resizedImage, cv::Size(), kResizeScale, kResizeScale, cv::INTER_LINEAR/*INTER_CUBIC*/);

	// 色による二値化
	cv::Mat textImage;
	cv::inRange(resizedImage,
		cv::Scalar(m_kHSVTextBounds.h_min, m_kHSVTextBounds.s_min, m_kHSVTextBounds.v_min),
		cv::Scalar(m_kHSVTextBounds.h_max, m_kHSVTextBounds.s_max, m_kHSVTextBounds.v_max), textImage);
	return textImage;
}
