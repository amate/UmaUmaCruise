#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <wtl\atlmisc.h>
#include "Utility\GdiplusUtil.h"

class UmaTextRecognizer
{
	static constexpr double kEventNameIconThreshold = 155.0;
	static constexpr double kEventNameIconWhiteRatioThreshold = 0.5;
	static constexpr double kMinWhiteRatioThreshold = 0.5;	// 白背景か決める閾値
	static constexpr double kMinWhiteTextRatioThreshold = 0.05;	// 画像中の白文字率の閾値
	static constexpr double kBlueBackgroundThreshold = 0.7;	// 青背景率の閾値
	static constexpr int kBackButtonExistThreshold = 200;	// 戻るボタンの文字列を分ける閾値
	
	static constexpr double kResizeScale = 2.0;	// 拡大倍率

public:
	bool	LoadSetting();

	std::unique_ptr<Gdiplus::Bitmap>	ScreenShot();

	std::wstring	GetIkuseiUmaMusumeName();

	bool	TextRecognizer(Gdiplus::Bitmap* image = nullptr);

	const std::vector<std::wstring>&	GetUmaMusumeName() const {
		return m_umaMusumeName;
	}
	const std::vector<std::wstring>&	GetCurrentTurn() const {
		return m_currentTurn;
	}
	const std::vector<std::wstring>&	GetEventName() const {
		return m_eventName;
	}
	const std::vector<std::wstring>& GetEventBottomOption() const {
		return m_eventBottomOption;
	}
	bool	IsTrainingMenu() const {
		return m_bTrainingMenu;
	}
	int		GetEntryRaceDistance() const {
		return m_entryRaceDistance;
	}

private:
	// BaseClientSize を基準として、imageのサイズに合うように boundsを調節する
	CRect	_AdjustBounds(const cv::Mat& srcImage, CRect bounds);

	bool	_IsEventNameIcon(cv::Mat srcImage);

	cv::Mat	_InRangeHSVTextColorBounds(cv::Mat cutImage);

	CString	m_targetWindowName;
	CString m_targetClassName;

	CSize	m_baseClientSize;

	enum TestBounds {
		kUmaMusumeSubNameBounds, kUmaMusumeNameBounds, kCurrentTurnBounds, kEventCategoryBounds, kEventNameBounds, kEventNameIconBounds, kEventBottomOptionBounds, kCurrentMenuBounds, kBackButtonBounds, kRaceDetailBounds, kIkuseiUmaMusumeSubNameBounds, kIkuseiUmaMusumeNameBounds,  kMaxCount
	};
	static constexpr LPCWSTR kTestBoundsName[kMaxCount] = {
		L"UmaMusumeSubNameBounds", L"UmaMusumeNameBounds", L"CurrentTurnBounds", L"EventCategoryBounds", L"EventNameBounds", L"EventNameIconBounds", L"EventBottomOptionBounds", L"CurrentMenuBounds", L"BackButtonBounds", L"RaceDetailBounds", L"IkuseiUmaMusumeSubNameBounds", L"IkuseiUmaMusumeNameBounds"
	};
	std::array<CRect, kMaxCount>	m_testBounds;
	std::unordered_map<std::wstring, std::wstring>	m_typoDictionary;

	struct HSVBounds {
		int	h_min, h_max;
		int s_min, s_max;
		int v_min, v_max;
	};
	HSVBounds	m_kHSVTextBounds;

	std::vector<std::wstring>	m_umaMusumeName;
	std::vector<std::wstring>	m_currentTurn;
	std::vector<std::wstring>	m_eventName;
	std::vector<std::wstring>	m_eventBottomOption;
	bool						m_bTrainingMenu = false;
	int							m_entryRaceDistance = 0;

};

