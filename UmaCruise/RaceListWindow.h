#pragma once

#include <unordered_set>
#include <atldlgs.h>
#include <atlddx.h>

#include "RaceDateLibrary.h"
#include "Config.h"

#include "Utility\json.hpp"

#include "resource.h"

class RaceListWindow : 
	public CDialogImpl<RaceListWindow>,
	public CWinDataExchange<RaceListWindow>,
	public CCustomDraw<RaceListWindow>
{
public:
	enum { IDD = IDD_RACELIST };

	enum {
		kDockingMargin = 15
	};

	RaceListWindow(const Config& config) : m_config(config) {}

	void	ShowWindow(bool bShow);

	void	AnbigiousChangeCurrentTurn(const std::vector<std::wstring>& ambiguousCurrentTurn);

	void	EntryRaceDistance(int distance);

	void	ChangeIkuseiUmaMusume(const std::wstring& umaName);

	// overrides
	DWORD OnPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/);
	DWORD OnItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/);

	BEGIN_DDX_MAP(RaceListWindow)
		// Race
		DDX_TEXT(IDC_EDIT_NOWDATE, m_currentTurn)
		DDX_CHECK(IDC_CHECK_SHOWRACE_AFTERCURRENTDATE, m_showRaceAfterCurrentDate)

		DDX_CHECK(IDC_CHECK_G1, m_gradeG1)
		DDX_CHECK(IDC_CHECK_G2, m_gradeG2)
		DDX_CHECK(IDC_CHECK_G3, m_gradeG3)

		DDX_CHECK(IDC_CHECK_SPRINT, m_sprint)
		DDX_CHECK(IDC_CHECK_MILE, m_mile)
		DDX_CHECK(IDC_CHECK_MIDDLE, m_middle)
		DDX_CHECK(IDC_CHECK_LONG, m_long)

		DDX_CHECK(IDC_CHECK_GRASS, m_grass)
		DDX_CHECK(IDC_CHECK_DART, m_dart)

		DDX_CHECK(IDC_CHECK_RIGHT, m_right)
		DDX_CHECK(IDC_CHECK_LEFT, m_left)
		DDX_CHECK(IDC_CHECK_LINE, m_line)

		DDX_CONTROL_HANDLE(IDC_LIST_RACE, m_raceListView)
		DDX_CONTROL_HANDLE(IDC_EDIT_EXPECT_URA, m_editExpectURA)
	END_DDX_MAP()

	BEGIN_MSG_MAP_EX(RaceListWindow)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)

		MSG_WM_EXITSIZEMOVE(OnExitSizeMove)

		MSG_WM_CTLCOLORDLG(OnCtlColorDlg)
		MSG_WM_CTLCOLORSTATIC(OnCtlColorDlg)
		MSG_WM_CTLCOLORBTN(OnCtlColorDlg)
		MSG_WM_CTLCOLOREDIT(OnCtlColorDlg)

		NOTIFY_HANDLER_EX(IDC_LIST_RACE, NM_CLICK, OnRaceListClick)
		NOTIFY_HANDLER_EX(IDC_LIST_RACE, NM_RCLICK, OnRaceListRClick)

		// Race List
		COMMAND_ID_HANDLER_EX(IDC_CHECK_SHOWRACE_AFTERCURRENTDATE, OnShowRaceAfterCurrentDate)
		COMMAND_RANGE_HANDLER_EX(IDC_CHECK_G1, IDC_CHECK_LOCATION_OOI, OnRaceFilterChanged)

		CHAIN_MSG_MAP(CCustomDraw<RaceListWindow>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void OnExitSizeMove();

	HBRUSH OnCtlColorDlg(CDCHandle dc, CWindow wnd);

	LRESULT OnRaceListClick(LPNMHDR pnmh);
	LRESULT OnRaceListRClick(LPNMHDR pnmh);

	void OnShowRaceAfterCurrentDate(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnRaceFilterChanged(UINT uNotifyCode, int nID, CWindow wndCtl);

private:
	void _UpdateRaceList(const std::wstring& turn);

	int32_t _GetRaceMatchState();
	void _SetRaceMatchState(int32_t state);

	void	_SwitchFavoriteRace(int index);
	bool	_IsFavoriteRaceTurn(const std::wstring& turn);

	enum RaceHighlightFlag {
		kAlter = 1 << 0,
		kFavorite = 1 << 1,
	};

	const Config&	m_config;
	RaceDateLibrary	m_raceDateLibrary;

	CString	m_currentTurn;

	bool	m_showRaceAfterCurrentDate = true;

	bool	m_gradeG1 = true;
	bool	m_gradeG2 = true;
	bool	m_gradeG3 = true;

	bool	m_sprint = true;
	bool	m_mile = true;
	bool	m_middle = true;
	bool	m_long = true;

	bool	m_grass = true;
	bool	m_dart = true;

	bool	m_right = true;
	bool	m_left = true;
	bool	m_line = true;

	bool	m_raceLocation[RaceDateLibrary::Race::Location::kMaxLocationCount];

	CListViewCtrl	m_raceListView;
	CEdit			m_editExpectURA;

	std::wstring	m_currentIkuseUmaMusume;
	nlohmann::json	m_jsonCharaFavoriteRaceList;
	std::unordered_set<std::string>	m_currentFavoriteRaceList;

	enum ClassifyDistanceClass {
		kMinSprint = 1000, kMaxSprint = 1599,
		kMinMile = 1600, kMaxMile = 1999,
		kMinMiddle = 2000, kMaxMiddle = 2499,
		kMinLong = 2500, kMaxLong = 4000,
	};
	struct RaceDistanceData {
		int turn;
		int	distanceClass;

		RaceDistanceData(int turn, int distanceClass) : turn(turn), distanceClass(distanceClass) {}
	};
	std::vector<RaceDistanceData>	m_entryRaceDistanceList;
};

