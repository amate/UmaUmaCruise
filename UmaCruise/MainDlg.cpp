
#include "stdafx.h"
#include "MainDlg.h"

#include <tesseract\baseapi.h>
#include <leptonica\allheaders.h>

#include <opencv2\opencv.hpp>

#include "Utility\CodeConvert.h"
#include "Utility\CommonUtility.h"
#include "Utility\json.hpp"

#include "ConfigDlg.h"

using json = nlohmann::json;
using namespace CodeConvert;
using namespace cv;


bool	SaveWindowScreenShot(HWND hWndTarget, const std::wstring& filePath)
{
	CWindowDC dc(NULL/*hWndTarget*/);

	CRect rcWindow;
	::GetWindowRect(hWndTarget, &rcWindow);

	CRect rcClient;
	::GetClientRect(hWndTarget, rcClient);

	CRect rcAdjustClient = rcWindow;
	const int topMargin = (rcWindow.Height() - rcClient.Height() - GetSystemMetrics(SM_CXFRAME) * 2 - GetSystemMetrics(SM_CYCAPTION)) / 2;
	rcAdjustClient.top += GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CYCAPTION) + topMargin;
	rcAdjustClient.left += (rcWindow.Width() - rcClient.Width()) / 2;
	rcAdjustClient.right = rcAdjustClient.left + rcClient.right;
	rcAdjustClient.bottom = rcAdjustClient.top + rcClient.bottom;

	CDC dcMemory;
	dcMemory.CreateCompatibleDC(dc);
	CBitmap hbmp = ::CreateCompatibleBitmap(dc, rcAdjustClient.Width(), rcAdjustClient.Height());
	auto prevhbmp = dcMemory.SelectBitmap(hbmp);

	//dcMemory.BitBlt(0, 0, rcWindow.Width(), rcWindow.Height(), dc, 0, 0, SRCCOPY);
	dcMemory.BitBlt(0, 0, rcAdjustClient.Width(), rcAdjustClient.Height(), dc, rcAdjustClient.left, rcAdjustClient.top, SRCCOPY);
	dcMemory.SelectBitmap(prevhbmp);

	Gdiplus::Bitmap bmp(hbmp, NULL);
	auto pngEncoder = GetEncoderByMimeType(L"image/png");
	auto ret = bmp.Save(filePath.c_str(), &pngEncoder->Clsid);
	bool success = ret == Gdiplus::Ok;
	return success;
}


bool SaveScreenShot(const std::wstring& device, const std::wstring& filePath)
{
	auto adbPath = GetExeDirectory() / L"platform-tools" / L"adb.exe";
	std::wstring targetDevice;
	if (device.length() > 0) {
		targetDevice = L" -s " + device;
	}

	std::wstring deviceSSPath = L"/sdcard/screen.png";
	//if (g_targetDevice.substr(0, 3) != L"127") {
	//	deviceSSPath = L"/sdcard/Screenshots/screen.png";
	//}

	std::wstring commandLine = targetDevice + L" shell screencap -p " + deviceSSPath;
	DWORD ret = StartProcess(adbPath, commandLine);
	if (ret != 0) {
		return false;
		//throw std::runtime_error("shell screencap failed");
	}

	std::wstring ssPath = filePath;
	commandLine = std::wstring(targetDevice + L" pull " + deviceSSPath + L" \"") + ssPath + L"\"";
	ret = StartProcess(adbPath, commandLine);
	if (ret != 0) {
		return false;
		//throw std::runtime_error("pull /sdcard/screen.png failed");
	}
	return true;
}



/////////////////////////////////////////////////////////////////////////////

void CMainDlg::ChangeWindowTitle(const std::wstring& title)
{
	CString str = L"UmaUmaCruise - ";
	str += title.c_str();
	SetWindowText(str);
}

DWORD CMainDlg::OnPrePaint(int idCtrl, LPNMCUSTOMDRAW)
{
	if (idCtrl == IDC_LIST_RACE) {
		return CDRF_NOTIFYITEMDRAW;
	}
	return CDRF_DODEFAULT;
}

DWORD CMainDlg::OnItemPrePaint(int, LPNMCUSTOMDRAW lpNMCustomDraw)
{
	// 前半と後半でカラムの色を色分けする
	auto pCustomDraw = (LPNMLVCUSTOMDRAW)lpNMCustomDraw;
#if 0
	CString date;
	m_raceListView.GetItemText(static_cast<int>(pCustomDraw->nmcd.dwItemSpec), 0, date);
	const bool first = date.Right(2) == L"前半";
#endif
	const bool first = pCustomDraw->nmcd.lItemlParam != 0;
	
	pCustomDraw->clrTextBk = first ? RGB(230, 231, 255) : RGB(252, 228, 214);// : RGB(241, 246, 252);
	return 0;
}

LRESULT CMainDlg::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&)
{
	// center the dialog on the screen
	CenterWindow();

	// set icons
	HICON hIcon = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
	SetIcon(hIconSmall, FALSE);

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	UIAddChildWindowContainer(m_hWnd);

	m_config.LoadConfig();

	DoDataExchange(DDX_LOAD);

	// 選択肢エディットの背景色を設定
	m_optionBkColor[0] = RGB(203, 247, 148);
	m_optionBkColor[1] = RGB(255, 236, 150);
	m_optionBkColor[2] = RGB(255, 203, 228);
	for (int i = 0; i < std::size(m_brsOptions); ++i) {
		m_brsOptions[i].CreateSolidBrush(m_optionBkColor[i]);
	}

	m_previewWindow.Create(m_hWnd);
	m_previewWindow.SetNotifyDragdropBounds([this](const CRect& rcBounds) {
		m_rcBounds = rcBounds;
	});

	if (!m_umaEventLibrary.LoadUmaMusumeLibrary()) {
		ERROR_LOG << L"LoadUmaMusumeLibrary failed";
		ATLASSERT(FALSE);
	} else {
		// 育成ウマ娘のリストをコンボボックスに追加
		CString currentProperty;
		for (const auto& uma : m_umaEventLibrary.GetIkuseiUmaMusumeEventList()) {
			if (currentProperty != uma->property.c_str()) {
				currentProperty = uma->property.c_str();
				m_cmbUmaMusume.AddString(currentProperty);
			}
			m_cmbUmaMusume.AddString(uma->name.c_str());
		}
	}

	if (!m_raceDateLibrary.LoadRaceDataLibrary()) {
		ERROR_LOG << L"LoadRaceDataLibrary failed";
		ATLASSERT(FALSE);
	}

	if (!m_umaTextRecoginzer.LoadSetting()) {
		ERROR_LOG << L"m_umaTextRecoginzer.LoadSetting failed";
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

	try {
		{
			std::ifstream ifs((GetExeDirectory() / "Common.json").string());
			ATLASSERT(ifs);
			if (!ifs) {
				ERROR_LOG << L"Common.json が存在しません...";
				ChangeWindowTitle(L"Common.json が存在しません...");
			} else {
				json jsonCommon;
				ifs >> jsonCommon;

				m_targetWindowName = UTF16fromUTF8(jsonCommon["Common"]["TargetWindow"]["WindowName"].get<std::string>()).c_str();
				m_targetClassName = UTF16fromUTF8(jsonCommon["Common"]["TargetWindow"]["ClassName"].get<std::string>()).c_str();
			}
		}

		std::ifstream fs((GetExeDirectory() / "setting.json").string());
		if (fs) {
			json jsonSetting;
			fs >> jsonSetting;

			{
				auto& windowRect = jsonSetting["MainDlg"]["WindowRect"];
				if (windowRect.is_null() == false) {
					CRect rc(windowRect[0], windowRect[1], windowRect[2], windowRect[3]);
					SetWindowPos(NULL, rc.left, rc.top, 0, 0, SWP_NOSIZE | SWP_NOOWNERZORDER);
				}

				m_bShowRaceList = !jsonSetting["MainDlg"].value<bool>("ShowRaceList", m_bShowRaceList);
				OnShowHideRaceList(0, 0, NULL);
			}

			{
				auto& windowRect = jsonSetting["PreviewWindow"]["WindowRect"];
				if (windowRect.is_null() == false) {
					CRect rc(windowRect[0], windowRect[1], windowRect[2], windowRect[3]);
					m_previewWindow.SetWindowPos(NULL, rc.left, rc.top, rc.Width(), rc.Height(), SWP_NOOWNERZORDER);
				}
				bool showWindow = jsonSetting["PreviewWindow"].value<bool>("ShowWindow", false);
				if (showWindow) {
					m_previewWindow.ShowWindow(SW_NORMAL);
				}
			}
			// Race
			m_showRaceAfterCurrentDate = jsonSetting["MainDlg"].value<bool>("ShowRaceAfterCurrentDate", m_showRaceAfterCurrentDate);
			const int32_t state = jsonSetting["MainDlg"].value<int32_t>("RaceMatchState", -1);
			_SetRaceMatchState(state);
		} else {
			_SetRaceMatchState(-1);
		}
		_UpdateRaceList(L"");

		DoDataExchange(DDX_LOAD);

	} catch (std::exception& e)
	{
		ATLTRACE(L"%s\n", (LPCWSTR)(CA2W(e.what())));
		ERROR_LOG << L"LoadConfig failed: " << (LPCWSTR)(CA2W(e.what()));
		ATLASSERT(FALSE);
	}
	ChangeWindowTitle(L"init suscess!");

	if (m_config.autoStart) {
		CButton(GetDlgItem(IDC_CHECK_START)).SetCheck(BST_CHECKED);
		OnStart(0, 0, NULL);
	}
	return TRUE;
}

LRESULT CMainDlg::OnDestroy(UINT, WPARAM, LPARAM, BOOL&)
{
	// unregister message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->RemoveMessageFilter(this);
	pLoop->RemoveIdleHandler(this);

	if (m_threadAutoDetect.joinable()) {
		m_cancelAutoDetect = true;
		m_threadAutoDetect.detach();
		::Sleep(2 * 1000);
	}

	return 0;
}

LRESULT CMainDlg::OnAppAbout(WORD, WORD, HWND, BOOL&)
{
	CAboutDlg dlg(m_previewWindow);
	dlg.DoModal();
	return 0;
}

// レースリストの表示を切り替え
void CMainDlg::OnShowHideRaceList(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	CRect rcWindow;
	GetWindowRect(&rcWindow);

	enum { kRightMargin = 23 };

	const int targetID = m_bShowRaceList ? IDC_BUTTON_SHOWHIDE_RACELIST : IDC_STATIC_RACELIST_GROUP;
	CRect rcCtrl;
	GetDlgItem(targetID).GetClientRect(&rcCtrl);
	GetDlgItem(targetID).MapWindowPoints(m_hWnd, &rcCtrl);
	//AdjustWindowRectEx(&rcCtrl, GetStyle(), FALSE, GetExStyle());
	rcCtrl.right += kRightMargin;
	SetWindowPos(NULL, 0, 0, rcCtrl.right, rcWindow.Height(), SWP_NOMOVE | SWP_NOZORDER);

	m_bShowRaceList = !m_bShowRaceList;
}

LRESULT CMainDlg::OnCancel(WORD, WORD wID, HWND, BOOL&)
{
	DoDataExchange(DDX_SAVE);

	json jsonSetting;
	std::ifstream fs((GetExeDirectory() / "setting.json").string());
	if (fs) {
		fs >> jsonSetting;
	}
	fs.close();

	if (IsIconic() == FALSE) {
		{
			CRect rcWindow;
			GetWindowRect(&rcWindow);
			jsonSetting["MainDlg"]["WindowRect"] =
				nlohmann::json::array({ rcWindow.left, rcWindow.top, rcWindow.right, rcWindow.bottom });

			jsonSetting["MainDlg"]["ShowRaceList"] = m_bShowRaceList;
		}
		{
			CRect rcWindow;
			m_previewWindow.GetWindowRect(&rcWindow);
			jsonSetting["PreviewWindow"]["WindowRect"] =
				nlohmann::json::array({ rcWindow.left, rcWindow.top, rcWindow.right, rcWindow.bottom });
			bool showWindow = m_previewWindow.IsWindowVisible() != 0;
			jsonSetting["PreviewWindow"]["ShowWindow"] = showWindow;
		}
	}
	// Race
	jsonSetting["MainDlg"]["ShowRaceAfterCurrentDate"] = m_showRaceAfterCurrentDate;
	jsonSetting["MainDlg"]["RaceMatchState"] = _GetRaceMatchState();

	std::ofstream ofs((GetExeDirectory() / "setting.json").string());
	ofs << jsonSetting;

	DestroyWindow();
	::PostQuitMessage(0);
	return 0;
}

void CMainDlg::OnShowConfigDlg(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	ConfigDlg dlg(m_config);
	dlg.DoModal(m_hWnd);
}

void CMainDlg::OnShowPreviewWindow(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	m_previewWindow.ShowWindow(SW_NORMAL);
}

void CMainDlg::OnTimer(UINT_PTR nIDEvent)
{
}

void CMainDlg::OnSelChangeUmaMusume(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	const int index = m_cmbUmaMusume.GetCurSel();
	if (index == -1) {
		return;
	}
	CString umaName;
	m_cmbUmaMusume.GetLBText(index, umaName);
	if (umaName.Left(1) == L"☆") {
		return;
	}
	m_umaEventLibrary.ChangeIkuseiUmaMusume((LPCWSTR)umaName);
}

// ダイアログの背景色を白に変更
HBRUSH CMainDlg::OnCtlColorDlg(CDCHandle dc, CWindow wnd)
{
	// 選択肢エディットの背景色を設定
	const int ctrlID = wnd.GetDlgCtrlID();
	if (IDC_EDIT_OPTION1 <= ctrlID && ctrlID <= IDC_EDIT_OPTION3) {
		int i = ctrlID - IDC_EDIT_OPTION1;
		dc.SetBkMode(OPAQUE);
		dc.SetBkColor(m_optionBkColor[i]);
		return m_brsOptions[i];
	}

	return (HBRUSH)::GetStockObject(WHITE_BRUSH);
}

void CMainDlg::OnScreenShot(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	HWND hwndTarget = ::FindWindow(m_targetClassName, m_targetWindowName);
	if (!hwndTarget) {
		ChangeWindowTitle(L"ウマ娘のウィンドウが見つかりません。。。");
		return ;
	}
	auto ssFolderPath = GetExeDirectory() / L"screenshot";
	if (!fs::is_directory(ssFolderPath)) {
		fs::create_directory(ssFolderPath);
	}

	auto ssPath = ssFolderPath / (L"screenshot_" + std::to_wstring(std::time(nullptr)) + L".png");
	if (GetKeyState(VK_CONTROL) < 0) {
		ssPath = ssFolderPath / L"screenshot.png";
	}
	// 
	bool success = SaveWindowScreenShot(hwndTarget, ssPath.wstring());
	//bool success = SaveScreenShot(L"", ssPath.wstring());
	ATLASSERT(success);

	m_previewWindow.UpdateImage(ssPath.wstring());
	return;
}

void CMainDlg::OnStart(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	CButton btnStart = GetDlgItem(IDC_CHECK_START);
	bool bChecked = btnStart.GetCheck() == BST_CHECKED;
	if (bChecked) {
		ATLASSERT(!m_threadAutoDetect.joinable());
		btnStart.SetWindowText(L"ストップ");
		m_cancelAutoDetect = false;
		m_threadAutoDetect = std::thread([this]()
		{
			int count = 0;
			while (!m_cancelAutoDetect.load()) {
				auto ssImage = m_umaTextRecoginzer.ScreenShot();
				bool success = m_umaTextRecoginzer.TextRecognizer(ssImage.get());
				if (success) {
					bool updateImage = true;
					if (m_config.stopUpdatePreviewOnTraining && !m_umaTextRecoginzer.IsTrainingMenu()) {
						updateImage = false;
					}
					if (updateImage) {
						m_previewWindow.UpdateImage(ssImage.release());
					}

					// 育成ウマ娘名
					std::wstring prevUmaName = m_umaEventLibrary.GetCurrentIkuseiUmaMusume();
					m_umaEventLibrary.AnbigiousChangeIkuseImaMusume(m_umaTextRecoginzer.GetUmaMusumeName());
					std::wstring nowUmaName = m_umaEventLibrary.GetCurrentIkuseiUmaMusume();
					if (prevUmaName != nowUmaName) {
						// コンボボックスを変更
						const int count = m_cmbUmaMusume.GetCount();
						for (int i = 0; i < count; ++i) {
							CString name;
							m_cmbUmaMusume.GetLBText(i, name);
							if (name == nowUmaName.c_str()) {
								m_cmbUmaMusume.SetCurSel(i);
								break;
							}
						}
					}

					// イベント検索
					auto optUmaEvent = m_umaEventLibrary.AmbiguousSearchEvent(m_umaTextRecoginzer.GetEventName());
					if (optUmaEvent) {
						m_eventName = optUmaEvent->eventName.c_str();
						DoDataExchange(DDX_LOAD, IDC_EDIT_EVENTNAME);

						_UpdateEventOptions(*optUmaEvent);
					}


					// 現在ターン
					std::wstring currentTurn = m_raceDateLibrary.AnbigiousChangeCurrentTurn(m_umaTextRecoginzer.GetCurrentTurn());
					if (currentTurn.length() && m_currentTurn != currentTurn.c_str()) {
						_UpdateRaceList(currentTurn);
					}

					++count;
					CString title;
					title.Format(L"Scan count: %d", count);
					ChangeWindowTitle((LPCWSTR)title);

					for (int i = 0; i < m_config.refreshInterval; ++i) {
						::Sleep(1000);	// wait
					}
				} else {
					if (!ssImage) {
						ChangeWindowTitle(L"ウマ娘のウィンドウが見つかりません...");
					} else {
						ChangeWindowTitle(L"failed...");
					}
					int sleepCount = 0;
					enum { kMaxSleepCount = 30 };
					while (!m_cancelAutoDetect.load() && sleepCount < kMaxSleepCount) {
						::Sleep(1000);
						++sleepCount;
					}
				}
			}
			// finish
			if (m_threadAutoDetect.joinable()) {
				CButton btnStart = GetDlgItem(IDC_CHECK_START);
				btnStart.SetWindowText(L"スタート");
				btnStart.EnableWindow(TRUE);
				m_threadAutoDetect.detach();
			}
		});
		//OnTimer(kAutoOCRTimerID);
		//SetTimer(kAutoOCRTimerID, kAutoOCRTimerInterval);
	} else {
		if (m_threadAutoDetect.joinable()) {
			btnStart.SetWindowText(L"停止中...");
			btnStart.EnableWindow(FALSE);
			m_cancelAutoDetect = true;
		}
		//KillTimer(kAutoOCRTimerID);
	}
}

void CMainDlg::OnEventNameChanged(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	DoDataExchange(DDX_SAVE, IDC_EDIT_EVENTNAME);
	if (m_eventName.IsEmpty()) {
		return;
	}
	std::vector<std::wstring> eventNames;
	eventNames.emplace_back((LPCWSTR)m_eventName);
	auto optUmaEvent = m_umaEventLibrary.AmbiguousSearchEvent(eventNames);
	if (optUmaEvent) {
		ChangeWindowTitle(optUmaEvent->eventName);
		_UpdateEventOptions(*optUmaEvent);

		m_eventSource = m_umaEventLibrary.GetLastEventSource().c_str();
		DoDataExchange(DDX_LOAD, IDC_EDIT_EVENT_SOURCE);
	}
	
}

// イベント選択肢の効果を修正する
void CMainDlg::OnEventRevision(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	UmaEventLibrary::UmaEvent umaEvent;
	DoDataExchange(DDX_SAVE, IDC_EDIT_EVENTNAME);
	DoDataExchange(DDX_SAVE, IDC_EDIT_EVENT_SOURCE);
	if (m_eventName.IsEmpty()) {
		MessageBox(L"イベント名 が空です。");
		return;
	}
	if (m_eventSource.IsEmpty()) {
		MessageBox(L"ソース が空です");
		return;
	}
	
	json jsonOptionsArray = json::array();
	const size_t count = umaEvent.eventOptions.size();
	for (size_t i = 0; i < count; ++i) {
		const int IDC_OPTION = IDC_EDIT_OPTION1 + i;
		const int IDC_EFFECT = IDC_EDIT_EFFECT1 + i;
		CString text;
		GetDlgItem(IDC_OPTION).GetWindowText(text);
		umaEvent.eventOptions[i].option = (LPCWSTR)text;
		GetDlgItem(IDC_EFFECT).GetWindowText(text);
		umaEvent.eventOptions[i].effect = (LPCWSTR)text;

		if (umaEvent.eventOptions[i].option.empty()) {
			break;
		}
		json jsonOption = {
			{"Option", UTF8fromUTF16(umaEvent.eventOptions[i].option) },
			{"Effect", UTF8fromUTF16(umaEvent.eventOptions[i].effect) }
		};
		jsonOptionsArray.push_back(jsonOption);
	}
	if (jsonOptionsArray.empty()) {
		MessageBox(L"選択肢 が空です");
		return;
	}

	CString msg;
	msg.Format(L"イベント名 [%s] の選択肢を修正します。\nよろしいですか？", (LPCWSTR)m_eventName);
	if (MessageBox(msg, L"確認", MB_YESNO) == IDNO) {
		return;
	}
	{
		std::ifstream ifs((GetExeDirectory() / "UmaMusumeLibraryRevision.json").string());
		ATLASSERT(ifs);
		if (!ifs) {
			MessageBox(L"UmaMusumeLibraryRevision.json の読み込みに失敗");
			return;
		}
		json jsonRevisionLibrary;
		ifs >> jsonRevisionLibrary;
		ifs.close();

		std::string source = UTF8fromUTF16((LPCWSTR)m_eventSource);
		std::string eventName = UTF8fromUTF16((LPCWSTR)m_eventName);

		bool update = false;
		json& jsonEventList = jsonRevisionLibrary[source]["Event"];
		if (jsonEventList.is_array()) {
			// 更新
			for (json& jsonEvent : jsonEventList) {
				auto eventElm = *jsonEvent.items().begin();
				std::string orgEventName = eventElm.key();
				if (orgEventName == eventName) {
					json& jsonOptions = eventElm.value();
					jsonOptions.clear();		// 選択肢を一旦全部消す
					jsonOptions = jsonOptionsArray;
					update = true;
					break;
				}
			}
		}
		// 追加
		if (!update) {
			json jsonEvent;
			jsonEvent[eventName] = jsonOptionsArray;
			jsonEventList.push_back(jsonEvent);
		}

		// 保存
		std::ofstream ofs((GetExeDirectory() / "UmaMusumeLibraryRevision.json").string());
		ATLASSERT(ofs);
		if (!ofs) {
			MessageBox(L"UmaMusumeLibraryRevision.json のオープンに失敗");
			return;
		}
		ofs << jsonRevisionLibrary.dump(2);
		ofs.close();

		m_umaEventLibrary.LoadUmaMusumeLibrary();
		MessageBox(L"修正完了", L"成功");
	}
}

void CMainDlg::OnShowRaceAfterCurrentDate(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	DoDataExchange(DDX_SAVE);
	_UpdateRaceList((LPCWSTR)m_currentTurn);
}

void CMainDlg::OnRaceFilterChanged(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	DoDataExchange(DDX_SAVE);
	_UpdateRaceList((LPCWSTR)m_currentTurn);
}

void CMainDlg::_UpdateEventOptions(const UmaEventLibrary::UmaEvent& umaEvent)
{
	const size_t count = umaEvent.eventOptions.size();
	for (size_t i = 0; i < count; ++i) {
		const int IDC_OPTION = IDC_EDIT_OPTION1 + i;
		const int IDC_EFFECT = IDC_EDIT_EFFECT1 + i;
		GetDlgItem(IDC_OPTION).SetWindowText(umaEvent.eventOptions[i].option.c_str());
		GetDlgItem(IDC_EFFECT).SetWindowText(umaEvent.eventOptions[i].effect.c_str());
	}
}

int32_t CMainDlg::_GetRaceMatchState()
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

void CMainDlg::_SetRaceMatchState(int32_t state)
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

void CMainDlg::_UpdateRaceList(const std::wstring& turn)
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
			if (race->IsMatchState(state)) {
				if (!insert) {
					insert = !insert;
					alter = !alter;
				}
				int pos = m_raceListView.GetItemCount();
				m_raceListView.InsertItem(pos, date.c_str());
				m_raceListView.SetItemText(pos, 1, race->RaceName().c_str());
				m_raceListView.SetItemText(pos, 2, race->DistanceText().c_str());
				m_raceListView.SetItemText(pos, 3, race->GroundConditionText().c_str());
				m_raceListView.SetItemText(pos, 4, race->RotationText().c_str());
				m_raceListView.SetItemText(pos, 5, race->location.c_str());
				m_raceListView.SetItemData(pos, alter);

			}
		}
	}
	m_raceListView.SetRedraw(TRUE);
}
