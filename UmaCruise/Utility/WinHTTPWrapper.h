
#pragma once

#include <memory>
#include <winhttp.h>
#include <boost/exception/all.hpp>
#include <boost/optional.hpp>

namespace WinHTTPWrapper {

bool	InitWinHTTP(boost::optional<CString> optUserAgent = boost::none, boost::optional<CString> optProxy = boost::none);
void	TermWinHTTP();

struct HINTERNET_deleter
{
	typedef HINTERNET	pointer;
	void operator () (HINTERNET handle) {
		ATLVERIFY(::WinHttpCloseHandle(handle));
	}
};

typedef std::unique_ptr<HINTERNET, HINTERNET_deleter>	INetHandle;


typedef boost::error_info<struct err_info, DWORD> ex_add_info;

struct WinHTTPException : virtual boost::exception, virtual std::exception
{
	WinHTTPException() 
	{
		dwGetLastError = ::GetLastError();
		*this << ex_add_info(dwGetLastError);
	}

	DWORD	dwGetLastError;
};

class CUrl
{
public:
	CUrl() { }

	CUrl(const CString& url) : m_url(url)
	{
		Set(url);
	}

	CUrl(const std::wstring& url) : m_url(url.c_str())
	{
		Set(url.c_str());
	}

	void	Set(const CString& url)
	{
		URL_COMPONENTS urlComp = { sizeof(URL_COMPONENTS) };
		urlComp.dwHostNameLength	= -1;
		urlComp.dwUrlPathLength		= -1;
		if (::WinHttpCrackUrl(url, url.GetLength(), 0, &urlComp) == FALSE) 
			BOOST_THROW_EXCEPTION(WinHTTPException());
		m_hostName	= CString(urlComp.lpszHostName, urlComp.dwHostNameLength);
		m_path		= CString(urlComp.lpszUrlPath, urlComp.dwUrlPathLength);
		if (urlComp.nScheme == INTERNET_SCHEME_HTTPS)
			m_optSSLPort = urlComp.nPort;
	}

	const CString&	GetURL() const { return m_url; }
	const CString&	GetHost() const { return m_hostName; }
	const CString&	GetPath() const { return m_path; }
	boost::optional<int>	GetSSLPortNumber() const { return m_optSSLPort; }

private:
	CString	m_url;
	CString	m_hostName;
	CString	m_path;
	boost::optional<int>	m_optSSLPort;
};

/// urlで指定したサーバーとのコネクションを作成する
INetHandle	HttpConnect(const CUrl& url);

/// コネクションから実際のリソースに対するリクエストを作成する
INetHandle	HttpOpenRequest(const CUrl& url, const INetHandle& hConnect, LPCWSTR Verb = L"GET", const CString& referer = CString());

/// リクエストヘッダを追加する
void		HttpAddRequestHeaders(const INetHandle& hRequest, const CString& addHeaders, DWORD dwModifiers = WINHTTP_ADDREQ_FLAG_ADD);

/// リクエストの設定を変更する
void		HttpSetOption(const INetHandle& hRequest, DWORD option, DWORD optionValue);

/// リクエストにプロクシを設定する
void		HttpSetProxy(const INetHandle& hRequest, const CString& proxy);

/// リクエストを送信した後、応答があるまで待つ
bool		HttpSendRequestAndReceiveResponse(const INetHandle& hRequest, const std::string& postData = std::string());

/// リクエスト結果のステータスコードを返す
DWORD		HttpQueryStatusCode(const INetHandle& hRequest);

/// InfoLevelで指定したレスポンスヘッダを返す
bool		HttpQueryHeaders(const INetHandle& hRequest, DWORD InfoLevel, CString& headerContents);

/// InfoLevelで指定したレスポンスヘッダを返す
bool		HttpQueryHeaders(const INetHandle& hRequest, DWORD InfoLevel, DWORD& headerContents);

// レスポンスヘッダを生のまま返す
bool		HttpQueryRawHeaders(const INetHandle& hRequest, CString& rawHeaderContents);

/// レスポンスからボディ部分を取得する
std::string HttpReadData(const INetHandle& hRequest);

/// urlからダウンロードする
boost::optional<std::string>	HttpDownloadData(const CString& url);


}	// namespace WinHTTPWrapper