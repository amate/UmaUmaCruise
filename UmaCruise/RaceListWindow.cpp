#include "stdafx.h"
#include "RaceListWindow.h"

#include <random>
#include <thread>

#include "Utility\CodeConvert.h"
#include "Utility\CommonUtility.h"
#include "Utility\Logger.h"
#include "Utility\json.hpp"

#include "ConfigDlg.h"

#pragma comment(lib, "Winmm.lib")	// for PlaySound

using json = nlohmann::json;
using namespace CodeConvert;


void RaceListWindow::ShowWindow(bool bShow)
{
	//INFO_LOG << L"RaceListWindow::ShowWindow: " << bShow;

	json jsonSetting;
	std::ifstream fs((GetExeDirectory() / "setting.json").wstring());
	if (fs) {
		fs >> jsonSetting;
	}
	fs.close();

	if (bShow) {
		// 表示位置復元
		auto& windowRect = jsonSetting["RaceListWindow"]["WindowRect"];
		if (windowRect.is_null() == false && !(::GetKeyState(VK_CONTROL) < 0)) {
			CRect rc(windowRect[0], windowRect[1], windowRect[2], windowRect[3]);
			SetWindowPos(NULL, rc.left, rc.top, rc.Width(), rc.Height(), SWP_NOOWNERZORDER | SWP_NOSIZE);
		} else {
			CenterWindow(GetParent());
		}
	} else {	
		// 表示位置保存
		if (IsWindowVisible()) {
			CRect rcWindow;
			GetWindowRect(&rcWindow);
			jsonSetting["RaceListWindow"]["WindowRect"] =
				nlohmann::json::array({ rcWindow.left, rcWindow.top, rcWindow.right, rcWindow.bottom });

			//INFO_LOG << L"SaveWindowPos x: " << rcWindow.left << L" y:" << rcWindow.top;

			std::ofstream ofs((GetExeDirectory() / "setting.json").wstring());
			ofs << jsonSetting.dump(4);
		}
	}
	__super::ShowWindow(bShow);
}

void RaceListWindow::AnbigiousChangeCurrentTurn(const std::vector<std::wstring>& ambiguousCurrentTurn)
{
	std::wstring currentTurn = m_raceDateLibrary.AnbigiousChangeCurrentTurn(ambiguousCurrentTurn);
	if (currentTurn.length() && m_currentTurn != currentTurn.c_str()) {

		if (m_config.notifyFavoriteRaceHold && _IsFavoriteRaceTurn(currentTurn)) {	// 通知する
			// ウィンドウを振動させる
			std::thread([this]() {
				auto sePath = GetExeDirectory() / L"se" / L"se.wav";
				if (fs::exists(sePath)) {
					BOOL bRet = sndPlaySoundW(sePath.c_str(), SND_ASYNC | SND_NODEFAULT);
					ATLASSERT(bRet);
				}

				CWindow wndTop = GetTopLevelWindow();
				CRect rcOriginal;
				wndTop.GetWindowRect(&rcOriginal);
				enum {
					kShakeDistance = 2,
					kShakeCount = 10,
					kShakeInterval = 30,//20,
				};
				std::uniform_int_distribution<> dist(-kShakeDistance, kShakeDistance);
				for (int i = 0; i < kShakeCount; ++i) {
					CPoint ptShake = rcOriginal.TopLeft();
					ptShake.x += dist(std::random_device());
					ptShake.y += dist(std::random_device());
					wndTop.SetWindowPos(NULL, ptShake.x, ptShake.y, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_ASYNCWINDOWPOS);
					::Sleep(kShakeInterval);
				}

				// 元の位置に戻す
				wndTop.SetWindowPos(NULL, rcOriginal.left, rcOriginal.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_ASYNCWINDOWPOS);

			}).detach();
		}

		_UpdateRaceList(currentTurn);
	}
}

void RaceListWindow::EntryRaceDistance(int distance)
{
	if (distance == 0 || m_currentTurn.IsEmpty() || m_currentTurn == L"ファイナルズ開催中") {
		return;
	}
	
	const int currentTurn = m_raceDateLibrary.GetTurnNumberFromTurnName((LPCWSTR)m_currentTurn);
	ATLASSERT(currentTurn != -1);

	// distance == -1 の時は URA予想の更新だけする
	if (distance != -1) {
		// distance から距離を分類する
		RaceDateLibrary::Race::DistanceClass distanceClass;
		if (kMinSprint <= distance && distance <= kMaxSprint) {
			distanceClass = RaceDateLibrary::Race::DistanceClass::kSprint;
		} else if (kMinMile <= distance && distance <= kMaxMile) {
			distanceClass = RaceDateLibrary::Race::DistanceClass::kMile;
		} else if (kMinMiddle <= distance && distance <= kMaxMiddle) {
			distanceClass = RaceDateLibrary::Race::DistanceClass::kMiddle;
		} else if (kMinLong <= distance && distance <= kMaxLong) {
			distanceClass = RaceDateLibrary::Race::DistanceClass::kLong;
		} else {
			ATLASSERT(FALSE);
			return;
		}

		if (m_entryRaceDistanceList.size()) {
			if (m_entryRaceDistanceList.back().turn == currentTurn) {
				return;	// 同ターン数なら更新なし
			}
			// 最後に登録されたターンより若いターンを登録しようとしたときは、新規周回されたと判断する
			if (currentTurn < m_entryRaceDistanceList.back().turn) {
				m_entryRaceDistanceList.clear();
			}
		}
		m_entryRaceDistanceList.emplace_back(currentTurn, distanceClass);
	}

	auto funcIndexFromDistanceClass = [](int distanceClass) -> int {
		switch (distanceClass) {
		case RaceDateLibrary::Race::DistanceClass::kSprint:
			return 0;
		case RaceDateLibrary::Race::DistanceClass::kMile:
			return 1;
		case RaceDateLibrary::Race::DistanceClass::kMiddle:
			return 2;
		case RaceDateLibrary::Race::DistanceClass::kLong:
			return 3;
		default:
			ATLASSERT(FALSE);
			return 0;
		}
	};

	// 出場レースの距離ごとにカウントする
	std::array<int, 4>	entryRaceClassCount = {};
	for (const auto& raceData : m_entryRaceDistanceList) {
		entryRaceClassCount[funcIndexFromDistanceClass(raceData.distanceClass)]++;
	}

	// お気に入りレースの距離ごとにカウントする
	std::array<int, 4>	favoriteRaceClassCount = {};
	for (const std::string& favoriteRace : m_currentFavoriteRaceList) {
		auto sepPos = favoriteRace.find('_');
		ATLASSERT(sepPos != std::string::npos);
		std::wstring date = UTF16fromUTF8(favoriteRace.substr(0, sepPos));
		std::wstring raceName = UTF16fromUTF8(favoriteRace.substr(sepPos + 1));

		const int raceTurn = m_raceDateLibrary.GetTurnNumberFromTurnName(date);
		if (raceTurn <= currentTurn) {
			continue;
		}

		// レース名からレース分類を逆引き
		const auto& turnOrderedRaceList = m_raceDateLibrary.GetTurnOrderedRaceList();
		for (const auto& race : turnOrderedRaceList[raceTurn]) {
			if (race->RaceName() == raceName) {
				favoriteRaceClassCount[funcIndexFromDistanceClass(race->distanceClass)]++;
			}
		}
	}
	CString text;
	std::pair<int, CString> expectURA;
	LPCWSTR kDisntanceClassNames[4] = { L"短距離", L"マイル", L"中距離", L"長距離" };
	int appendCount = 0;
	for (int i = 0; i < 4; ++i) {
		int a = entryRaceClassCount[i];
		int b = favoriteRaceClassCount[i];
		if (a > 0 || b > 0) {
			++appendCount;
			if (appendCount == 3) {
				text += L"\r\n";
			}
			text.AppendFormat(L"%s: %d/%d ", kDisntanceClassNames[i], a, b);

			int ab = a + b;
			if (expectURA.first < ab) {	// 超えたら上書き
				expectURA.first = ab;
				expectURA.second = kDisntanceClassNames[i];
			} else if (expectURA.first == ab) {	// 同値なら追加
				expectURA.second.AppendFormat(L"/%s", kDisntanceClassNames[i]);
			}
		}
	}
	if (text.GetLength()) {
		text += L"\r\nURA予想: " + expectURA.second;
		m_editExpectURA.SetWindowText(text);
	}
}

void RaceListWindow::ChangeIkuseiUmaMusume(const std::wstring& umaName)
{
	if (m_currentIkuseUmaMusume != umaName) {
		m_currentFavoriteRaceList.clear();
		if (umaName.length()) {
			// お気に入りレースを切り替え
			const json& jCharaFavoriteRaceList = m_jsonCharaFavoriteRaceList[UTF8fromUTF16(umaName)];
			if (jCharaFavoriteRaceList.is_array()) {
				m_currentFavoriteRaceList = jCharaFavoriteRaceList.get<std::unordered_set<std::string>>();
			}
		}
		m_currentIkuseUmaMusume = umaName;
		_UpdateRaceList((LPCWSTR)m_currentTurn);
	}

}


DWORD RaceListWindow::OnPrePaint(int idCtrl, LPNMCUSTOMDRAW)
{
	if (idCtrl == IDC_LIST_RACE) {
		return CDRF_NOTIFYITEMDRAW;
	}
	return CDRF_DODEFAULT;
}

DWORD RaceListWindow::OnItemPrePaint(int, LPNMCUSTOMDRAW lpNMCustomDraw)
{
	// 前半と後半でカラムの色を色分けする
	auto pCustomDraw = (LPNMLVCUSTOMDRAW)lpNMCustomDraw;
#if 0
	CString date;
	m_raceListView.GetItemText(static_cast<int>(pCustomDraw->nmcd.dwItemSpec), 0, date);
	const bool first = date.Right(2) == L"前半";
#endif
	const bool alter = (pCustomDraw->nmcd.lItemlParam & kAlter) != 0;
	const bool favorite = (pCustomDraw->nmcd.lItemlParam & kFavorite) != 0;
	if (favorite) {
		pCustomDraw->clrTextBk = RGB(255, 215, 0);
	} else {
		pCustomDraw->clrTextBk = alter ? RGB(230, 231, 255) : RGB(252, 228, 214);// : RGB(241, 246, 252);
	}
	return 0;
}

LRESULT RaceListWindow::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&)
{
	// set icons
	HICON hIcon = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
	SetIcon(hIconSmall, FALSE);

	DoDataExchange(DDX_LOAD);

	// 全レースデータを読み込み
	if (!m_raceDateLibrary.LoadRaceDataLibrary()) {
		ERROR_LOG << L"LoadRaceDataLibrary failed";
		ATLASSERT(FALSE);
	}

	// Race List
	m_raceListView.SetExtendedListViewStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

	auto funcAddColumn = [this](LPCWSTR columnName, int nItem, int columnWidth) {
		LVCOLUMN lvc = {};
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		lvc.fmt = LVCFMT_LEFT;
		lvc.pszText = (LPTSTR)columnName;
		lvc.cx = columnWidth;
		lvc.iSubItem = nItem;
		m_raceListView.InsertColumn(nItem, &lvc);
	};
	funcAddColumn(L"開催日", 0, 120);
	funcAddColumn(L"レース名", 1, 144);
	funcAddColumn(L"距離", 2, 110);
	funcAddColumn(L"コース", 3, 42);
	funcAddColumn(L"方向", 4, 38);
	funcAddColumn(L"レース場", 5, 58);

	// 設定読み込み
	{
		std::ifstream fs((GetExeDirectory() / "setting.json").wstring());
		if (fs) {
			json jsonSetting;
			fs >> jsonSetting;

			// Race
			if (jsonSetting["MainDlg"].is_object()) {
				m_showRaceAfterCurrentDate = jsonSetting["MainDlg"].value<bool>("ShowRaceAfterCurrentDate", m_showRaceAfterCurrentDate);
				const int32_t state = jsonSetting["MainDlg"].value<int32_t>("RaceMatchState", -1);
				_SetRaceMatchState(state);
			}
		} else {
			_SetRaceMatchState(-1);
		}
	}
	{
		std::ifstream fs((GetExeDirectory() / "CharaFavoriteRaceList.json").wstring());
		if (fs) {
			json jsonCharaFavoriteRaceList;
			fs >> m_jsonCharaFavoriteRaceList;
		}
	}


	_UpdateRaceList(L"");

	DoDataExchange(DDX_LOAD);

	return TRUE;
}

LRESULT RaceListWindow::OnDestroy(UINT, WPARAM, LPARAM, BOOL&)
{
	DoDataExchange(DDX_SAVE);

	// 設定保存
	{
		json jsonSetting;
		std::ifstream fs((GetExeDirectory() / "setting.json").wstring());
		if (fs) {
			fs >> jsonSetting;
		}
		fs.close();

		// Race
		jsonSetting["MainDlg"]["ShowRaceAfterCurrentDate"] = m_showRaceAfterCurrentDate;
		jsonSetting["MainDlg"]["RaceMatchState"] = _GetRaceMatchState();

		std::ofstream ofs((GetExeDirectory() / "setting.json").wstring());
		ofs << jsonSetting.dump(4);
		ofs.close();
	}
	{
		std::ofstream ofs((GetExeDirectory() / "CharaFavoriteRaceList.json").wstring());
		ofs << m_jsonCharaFavoriteRaceList.dump(4);
		ofs.close();
	}

	return 0;
}


LRESULT RaceListWindow::OnCancel(WORD, WORD wID, HWND, BOOL&)
{
	//DoDataExchange(DDX_SAVE);
	//ShowWindow(false);

	// メインダイアログ通知して隠してもらう
	GetParent().PostMessage(WM_COMMAND, IDC_BUTTON_SHOWHIDE_RACELIST);
	return 0;
}

// ウィンドウの位置移動終了時に呼ばれる
// メインウィンドウとのドッキング動作を行う
void RaceListWindow::OnExitSizeMove()
{
	CRect rcParentWindow;
	GetParent().GetWindowRect(&rcParentWindow);

	CRect rcWindow;
	GetWindowRect(&rcWindow);
	CRect rcClient;
	GetClientRect(&rcClient);

	const int cxPadding = (rcWindow.Width() - rcClient.Width()) - (GetSystemMetrics(SM_CXBORDER) * 2);//GetSystemMetrics(SM_CXSIZEFRAME) * 2;
	const int cyPadding = GetSystemMetrics(SM_CYSIZEFRAME) * 2;

	// メインの右にある
	if (std::abs(rcParentWindow.right - rcWindow.left) <= kDockingMargin) {
		rcWindow.MoveToX(rcParentWindow.right - cxPadding);

		// メインの左にある
	} else if (std::abs(rcParentWindow.left - rcWindow.right) <= kDockingMargin) {
		rcWindow.MoveToX(rcParentWindow.left - rcWindow.Width() + cxPadding);

		// メインの上にある
	} else if (std::abs(rcParentWindow.top - rcWindow.bottom) <= kDockingMargin) {
		rcWindow.MoveToY(rcParentWindow.top - rcWindow.Height() + cyPadding);

		// メインの下にある
	} else if (std::abs(rcParentWindow.bottom - rcWindow.top) <= kDockingMargin) {
		rcWindow.MoveToY(rcParentWindow.bottom - cyPadding);
	}
	MoveWindow(&rcWindow);
}

// ダイアログの背景色を白に変更
HBRUSH RaceListWindow::OnCtlColorDlg(CDCHandle dc, CWindow wnd)
{
	return (HBRUSH)::GetStockObject(WHITE_BRUSH);
}

// レースリストクリック
LRESULT RaceListWindow::OnRaceListClick(LPNMHDR pnmh)
{
	auto pnmitem = (LPNMITEMACTIVATE)pnmh;
	if (!(pnmitem->uKeyFlags == LVKF_CONTROL)) {
		return 0;	// Ctrlキーを押していなければ無視
	}	
	
	_SwitchFavoriteRace(pnmitem->iItem);
	return LRESULT();
}

// レースリスト右クリック
LRESULT RaceListWindow::OnRaceListRClick(LPNMHDR pnmh)
{
	auto pnmitem = (LPNMITEMACTIVATE)pnmh;
	if (pnmitem->iItem == -1) {
		return 0;
	}

	CMenu menu;
	menu.CreatePopupMenu();
	enum { kSwitchFavorite = 1 };
	menu.AppendMenuW(MF_STRING, kSwitchFavorite, L"レース予約を切り替え");

	CPoint pt;
	::GetCursorPos(&pt);
	BOOL ret = menu.TrackPopupMenu(TPM_RETURNCMD, pt.x, pt.y, m_hWnd);
	if (ret == kSwitchFavorite) {
		_SwitchFavoriteRace(pnmitem->iItem);
	}
	return LRESULT();
}

void RaceListWindow::OnShowRaceAfterCurrentDate(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	DoDataExchange(DDX_SAVE);
	_UpdateRaceList((LPCWSTR)m_currentTurn);
}

void RaceListWindow::OnRaceFilterChanged(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	DoDataExchange(DDX_SAVE);
	_UpdateRaceList((LPCWSTR)m_currentTurn);
}


void RaceListWindow::_UpdateRaceList(const std::wstring& turn)
{
	m_currentTurn = turn.c_str();
	DoDataExchange(DDX_LOAD, IDC_EDIT_NOWDATE);

	m_raceListView.SetRedraw(FALSE);
	m_raceListView.DeleteAllItems();

	size_t i = 0;
	if (turn.length() && m_showRaceAfterCurrentDate) {
		i = m_raceDateLibrary.GetTurnNumberFromTurnName(turn);
	}
	const int32_t state = _GetRaceMatchState();

	auto funcIsFavoriteRace = [this](const std::wstring& date, const std::wstring& raceName) -> bool {
		std::string date_race = UTF8fromUTF16(date + L"_" + raceName);
		auto itfound = m_currentFavoriteRaceList.find(date_race);
		if (itfound != m_currentFavoriteRaceList.end()) {
			return true;
		} else {
			return false;
		}
	};

	const auto& turnOrderedRaceList = m_raceDateLibrary.GetTurnOrderedRaceList();
	const auto& allTurnList = m_raceDateLibrary.GetAllTurnList();
	const size_t turnLength = allTurnList.size();
	bool	alter = false;
	for (; i < turnLength; ++i) {
		if (turnOrderedRaceList[i].empty()) {
			continue;
		}
		std::wstring date = allTurnList[i];	// 開催日
		bool insert = false;
		for (const auto& race : turnOrderedRaceList[i]) {
			const bool bFavoriteRace = funcIsFavoriteRace(date, race->RaceName());
			if (race->IsMatchState(state) || bFavoriteRace) {
				if (!insert) {
					insert = !insert;
					alter = !alter;
				}
				int flags = alter;
				std::wstring raceName;
				if (bFavoriteRace) {
					raceName = L"★";
					flags |= kFavorite;
				}
				raceName += race->RaceName();

				int pos = m_raceListView.GetItemCount();
				m_raceListView.InsertItem(pos, date.c_str());
				m_raceListView.SetItemText(pos, 1, raceName.c_str());
				m_raceListView.SetItemText(pos, 2, race->DistanceText().c_str());
				m_raceListView.SetItemText(pos, 3, race->GroundConditionText().c_str());
				m_raceListView.SetItemText(pos, 4, race->RotationText().c_str());
				m_raceListView.SetItemText(pos, 5, race->location.c_str());
				m_raceListView.SetItemData(pos, flags);

			}
		}
	}
	m_raceListView.SetRedraw(TRUE);
}

int32_t RaceListWindow::_GetRaceMatchState()
{
	int32_t state = 0;
	state |= m_gradeG1 ? RaceDateLibrary::Race::Grade::kG1 : 0;
	state |= m_gradeG2 ? RaceDateLibrary::Race::Grade::kG2 : 0;
	state |= m_gradeG3 ? RaceDateLibrary::Race::Grade::kG3 : 0;

	state |= m_sprint ? RaceDateLibrary::Race::DistanceClass::kSprint : 0;
	state |= m_mile ? RaceDateLibrary::Race::DistanceClass::kMile : 0;
	state |= m_middle ? RaceDateLibrary::Race::DistanceClass::kMiddle : 0;
	state |= m_long ? RaceDateLibrary::Race::DistanceClass::kLong : 0;

	state |= m_grass ? RaceDateLibrary::Race::GroundCondition::kGrass : 0;
	state |= m_dart ? RaceDateLibrary::Race::GroundCondition::kDart : 0;

	state |= m_right ? RaceDateLibrary::Race::Rotation::kRight : 0;
	state |= m_left ? RaceDateLibrary::Race::Rotation::kLeft : 0;
	state |= m_line ? RaceDateLibrary::Race::Rotation::kLine : 0;

	for (int i = 0; i < RaceDateLibrary::Race::Location::kMaxLocationCount; ++i) {
		const int checkBoxID = IDC_CHECK_LOCATION_SAPPORO + i;
		bool check = CButton(GetDlgItem(checkBoxID)).GetCheck() == BST_CHECKED;
		if (check) {
			state |= RaceDateLibrary::Race::Location::kSapporo << i;
		}
	}
	return state;
}

void RaceListWindow::_SetRaceMatchState(int32_t state)
{
	m_gradeG1 = (state & RaceDateLibrary::Race::kG1) != 0;
	m_gradeG2 = (state & RaceDateLibrary::Race::kG2) != 0;
	m_gradeG3 = (state & RaceDateLibrary::Race::kG3) != 0;

	m_sprint = (state & RaceDateLibrary::Race::kSprint) != 0;
	m_mile = (state & RaceDateLibrary::Race::kMile) != 0;
	m_middle = (state & RaceDateLibrary::Race::kMiddle) != 0;
	m_long = (state & RaceDateLibrary::Race::kLong) != 0;

	m_grass = (state & RaceDateLibrary::Race::kGrass) != 0;
	m_dart = (state & RaceDateLibrary::Race::kDart) != 0;

	m_right = (state & RaceDateLibrary::Race::kRight) != 0;
	m_left = (state & RaceDateLibrary::Race::kLeft) != 0;
	m_line = (state & RaceDateLibrary::Race::kLine) != 0;

	for (int i = 0; i < RaceDateLibrary::Race::Location::kMaxLocationCount; ++i) {
		RaceDateLibrary::Race::Location location =
			static_cast<RaceDateLibrary::Race::Location>(RaceDateLibrary::Race::Location::kSapporo << i);
		bool check = (state & location) != 0;
		const int checkBoxID = IDC_CHECK_LOCATION_SAPPORO + i;
		CButton(GetDlgItem(checkBoxID)).SetCheck(check);
	}
}

void RaceListWindow::_SwitchFavoriteRace(int index)
{
	ATLASSERT(index != -1 && index < m_raceListView.GetItemCount());
	auto userData = m_raceListView.GetItemData(index);
	const bool nowFavorite = (userData & kFavorite) != 0;

	CString date;
	CString raceName;
	m_raceListView.GetItemText(index, 0, date);
	m_raceListView.GetItemText(index, 1, raceName);
	if (nowFavorite) {
		raceName = raceName.Mid(1);	// 前の'☆'を切り捨てる
	}
	std::string date_race = UTF8fromUTF16(static_cast<LPCWSTR>(date + L"_" + raceName));

	// お気に入り登録をスイッチさせる
	if (nowFavorite) {
		m_currentFavoriteRaceList.erase(date_race);
	} else {
		m_currentFavoriteRaceList.insert(date_race);
	}
	if (m_currentIkuseUmaMusume.length()) {
		// jsonへ保存
		m_jsonCharaFavoriteRaceList[UTF8fromUTF16(m_currentIkuseUmaMusume)] = m_currentFavoriteRaceList;
	}

	const int top = m_raceListView.GetTopIndex();
	_UpdateRaceList((LPCWSTR)m_currentTurn);	// 更新
	const int page = m_raceListView.GetCountPerPage();
	// スクロール位置の復元
	m_raceListView.EnsureVisible(top + page - 1, FALSE);

	// URA予想更新
	EntryRaceDistance(-1);
}

bool RaceListWindow::_IsFavoriteRaceTurn(const std::wstring& turn)
{
	if (turn.empty()) {
		return false;
	}
	size_t i = m_raceDateLibrary.GetTurnNumberFromTurnName(turn);
	ATLASSERT(i != -1);

	auto funcIsFavoriteRace = [this](const std::wstring& date, const std::wstring& raceName) -> bool {
		std::string date_race = UTF8fromUTF16(date + L"_" + raceName);
		auto itfound = m_currentFavoriteRaceList.find(date_race);
		if (itfound != m_currentFavoriteRaceList.end()) {
			return true;
		} else {
			return false;
		}
	};

	const auto& turnOrderedRaceList = m_raceDateLibrary.GetTurnOrderedRaceList();
	if (turnOrderedRaceList[i].empty()) {
		return false;	// このターンにレースはない
	}

	const auto& allTurnList = m_raceDateLibrary.GetAllTurnList();
	std::wstring date = allTurnList[i];	// 開催日
	bool insert = false;
	for (const auto& race : turnOrderedRaceList[i]) {
		const bool bFavoriteRace = funcIsFavoriteRace(date, race->RaceName());
		if (bFavoriteRace) {
			return true;	// 見つかった
		}
	}
	return false;
}


