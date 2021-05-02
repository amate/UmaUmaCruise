#include "stdafx.h"
#include "ConfigDlg.h"

#include "Utility\json.hpp"
#include "Utility\CommonUtility.h"
#include "Utility\Logger.h"
#include "Utility\WinHTTPWrapper.h"

using json = nlohmann::json;
using namespace WinHTTPWrapper;

ConfigDlg::ConfigDlg(Config& config) : m_config(config)
{
}

LRESULT ConfigDlg::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&)
{
	DoDataExchange(DDX_LOAD);

	enum { kMaxRefershCount = 10 };
	for (int i = 1; i <= kMaxRefershCount; ++i) {
		m_cmbRefreshInterval.AddString(std::to_wstring(i).c_str());
	}
	m_cmbRefreshInterval.SetCurSel(m_config.refreshInterval - 1);

	m_autoStart = m_config.autoStart;
	m_stopUpdatePreviewOnTraining = m_config.stopUpdatePreviewOnTraining;
	m_popupRaceListWindow = m_config.popupRaceListWindow;
	m_notifyFavoriteRaceHold = m_config.notifyFavoriteRaceHold;
	DoDataExchange(DDX_LOAD);

	return 0;
}

LRESULT ConfigDlg::OnOK(WORD, WORD wID, HWND, BOOL&)
{
	DoDataExchange(DDX_SAVE);

	const int index = m_cmbRefreshInterval.GetCurSel();
	if (index == -1) {
		ATLASSERT(FALSE);
		ERROR_LOG << L"m_cmbRefreshInterval.GetCurSel == -1";
	} else {
		m_config.refreshInterval = index + 1;
	}
	m_config.autoStart = m_autoStart;
	m_config.stopUpdatePreviewOnTraining = m_stopUpdatePreviewOnTraining;
	m_config.popupRaceListWindow = m_popupRaceListWindow;
	m_config.notifyFavoriteRaceHold = m_notifyFavoriteRaceHold;

	m_config.SaveConfig();

	EndDialog(IDOK);
	return 0;
}

LRESULT ConfigDlg::OnCancel(WORD, WORD, HWND, BOOL&)
{
	EndDialog(IDCANCEL);
	return 0;
}

void ConfigDlg::OnCheckUmaLibrary(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	try {
		std::ifstream ifs((GetExeDirectory() / L"UmaLibrary" / "Common.json").wstring());
		ATLASSERT(ifs);
		if (!ifs) {
			MessageBox(L"Common.json の読み込みに失敗");
			return;
		}
		json jsonCommon;
		ifs >> jsonCommon;
		std::string libraryURL = jsonCommon["Common"]["UmaMusumeLibraryURL"];

		// ファイルサイズ取得
		auto umaLibraryPath = GetExeDirectory() / L"UmaLibrary" / L"UmaMusumeLibrary.json";
		const DWORD umaLibraryFileSize = static_cast<DWORD>(fs::file_size(umaLibraryPath));

		CUrl	downloadUrl(libraryURL.c_str());
		auto hConnect = HttpConnect(downloadUrl);
		auto hRequest = HttpOpenRequest(downloadUrl, hConnect, L"HEAD");
		if (HttpSendRequestAndReceiveResponse(hRequest)) {
			int statusCode = HttpQueryStatusCode(hRequest);
			if (statusCode == 200) {
				DWORD contentLength = 0;
				HttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_LENGTH, contentLength);
				if (umaLibraryFileSize != contentLength) {	// ファイルサイズ比較
					// 更新する
					auto optDLData = HttpDownloadData(downloadUrl.GetURL());
					if (optDLData) {
						SaveFile(umaLibraryPath, optDLData.get());
						MessageBox(L"更新しました\n更新後の UmaMusumeLibrary.json は再起動後に有効になります", L"成功");
						GetDlgItem(IDC_BUTTON_CHECK_UMALIBRARY).EnableWindow(FALSE);
						return;
					} else {
						MessageBox(L"ダウンロードに失敗しました...", L"エラー", MB_ICONERROR);
						return;
					}
				} else {
					MessageBox(L"更新は必要ありません", L"成功");
					GetDlgItem(IDC_BUTTON_CHECK_UMALIBRARY).EnableWindow(FALSE);
					return;
				}
			} else {
				CString errorText;
				errorText.Format(L"サーバーからエラーが返されました。\nステータスコード: %d", statusCode);
				MessageBox(errorText, L"エラー", MB_ICONERROR);
				return;
			}
		} else {
			MessageBox(L"リクエストの送信に失敗しました。\n詳細は info.log を参照してください。", L"エラー", MB_ICONERROR);
			return;
		}
	} catch (boost::exception& e) {
		std::string expText = boost::diagnostic_information(e);
		ERROR_LOG << L"OnCheckUmaLibrary exception: " << (LPCWSTR)CA2W(expText.c_str());
		int a = 0;
	}
	ATLASSERT(FALSE);
	MessageBox(L"何かしらのエラーが発生しました...", L"エラー", MB_ICONERROR);
}
