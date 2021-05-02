
#include "stdafx.h"
#include "..\stdafx.h"
#include "WinHTTPWrapper.h"

#include "Logger.h"

#pragma comment (lib, "winhttp.lib")

namespace WinHTTPWrapper {

LPCWSTR	kDefaultUserAgent = L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) UmaUmaCruise";

HINTERNET	s_hSession = NULL;

bool	InitWinHTTP(boost::optional<CString> optUserAgent /*= boost::none*/, boost::optional<CString> optProxy /*= boost::none*/)
{
	ATLASSERT( s_hSession == NULL );
	LPCWSTR proxy = optProxy ? (LPCWSTR)optProxy.get() : WINHTTP_NO_PROXY_NAME;
	DWORD accessType = proxy ? WINHTTP_ACCESS_TYPE_NAMED_PROXY : WINHTTP_ACCESS_TYPE_NO_PROXY;
	s_hSession = ::WinHttpOpen(optUserAgent ? (LPCWSTR)optUserAgent.get() : kDefaultUserAgent, 
								accessType,
								proxy,
								WINHTTP_NO_PROXY_BYPASS, 0);
	ATLASSERT( s_hSession );
	if (s_hSession == NULL)
		return false;
	return true;
}

void	TermWinHTTP()
{
	ATLASSERT( s_hSession );
	ATLVERIFY(::WinHttpCloseHandle( s_hSession ));
	s_hSession = NULL;
}

INetHandle	HttpConnect(const CUrl& url)
{
	INTERNET_PORT	port = INTERNET_DEFAULT_PORT;
	if (auto value = url.GetSSLPortNumber())
		port = value.get();
	INetHandle hConnect(::WinHttpConnect(s_hSession, url.GetHost(), port, 0));
	if (hConnect == nullptr)
		BOOST_THROW_EXCEPTION(WinHTTPException());
	return hConnect;
}

INetHandle	HttpOpenRequest(const CUrl& url, const INetHandle& hConnect, LPCWSTR Verb /*= L"GET"*/, const CString& referer /*= CString()*/)
{
	DWORD flags = 0;
	if (url.GetSSLPortNumber())
		flags = WINHTTP_FLAG_SECURE;
	INetHandle hRequest(::WinHttpOpenRequest(hConnect.get(), Verb, url.GetPath(), NULL, 
		referer.IsEmpty() ? nullptr : (LPCWSTR)referer, WINHTTP_DEFAULT_ACCEPT_TYPES, flags));
	if (hRequest == nullptr)
		BOOST_THROW_EXCEPTION(WinHTTPException());
	return hRequest;
}

void	HttpAddRequestHeaders(const INetHandle& hRequest, const CString& addHeaders, DWORD dwModifiers /*= WINHTTP_ADDREQ_FLAG_REPLACE*/)
{
	if (::WinHttpAddRequestHeaders(hRequest.get(), addHeaders, addHeaders.GetLength(), dwModifiers) == FALSE)
		BOOST_THROW_EXCEPTION(WinHTTPException());
}

void		HttpSetOption(const INetHandle& hRequest, DWORD option, DWORD optionValue)
{
	if (::WinHttpSetOption(hRequest.get(), option, &optionValue, sizeof(optionValue)) == FALSE)
		BOOST_THROW_EXCEPTION(WinHTTPException());
}

void		HttpSetProxy(const INetHandle& hRequest, const CString& proxy)
{
	WINHTTP_PROXY_INFO proxyInfo = {};
	proxyInfo.dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
	proxyInfo.lpszProxy	= (LPWSTR)(LPCWSTR)proxy;
	if (::WinHttpSetOption(hRequest.get(), WINHTTP_OPTION_PROXY, &proxyInfo, sizeof(proxyInfo)) == FALSE)
		BOOST_THROW_EXCEPTION(WinHTTPException());
}

bool	HttpSendRequestAndReceiveResponse(const INetHandle& hRequest, const std::string& postData /*= std::string()*/)
{
	LPCWSTR headers = WINHTTP_NO_ADDITIONAL_HEADERS;
	DWORD	headersLength = 0;
	LPVOID	optional = WINHTTP_NO_REQUEST_DATA;
	DWORD	optionalLength = 0;
	if (postData.length() > 0) {
		headers = L"Content-Type: application/x-www-form-urlencoded\r\n";
		headersLength = static_cast<DWORD>(::wcslen(headers));
		optional		= (LPVOID)postData.c_str();
		optionalLength	= static_cast<DWORD>(postData.length());
	}

	BOOL bRet = ::WinHttpSendRequest(hRequest.get(), 
									headers, headersLength,
									optional, optionalLength, optionalLength, 0);
	if (bRet) {
		bRet = ::WinHttpReceiveResponse(hRequest.get(), NULL);
		if (bRet == 0) {
			DWORD dwError = GetLastError();
			ATLTRACE(_T("HttpSendRequestAndReceiveResponse - WinHttpReceiveResponse Error!:%d"), dwError);
			ERROR_LOG << L"HttpSendRequestAndReceiveResponse - WinHttpReceiveResponse Error!: " << dwError;
		}
	} else {
		DWORD dwError = GetLastError();
		ERROR_LOG << L"HttpSendRequestAndReceiveResponse - WinHttpSendRequest Error!: " << dwError;
	}
	return bRet != 0;
}

DWORD		HttpQueryStatusCode(const INetHandle& hRequest)
{
	DWORD	status = 0;
	DWORD	dwSize = sizeof(status);
	if (::WinHttpQueryHeaders(hRequest.get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, (LPVOID)&status, &dwSize, WINHTTP_NO_HEADER_INDEX))
		return status;
	ATLASSERT( FALSE );
	return 0;
}

bool		HttpQueryHeaders(const INetHandle& hRequest, DWORD InfoLevel, CString& headerContents)
{
	DWORD dwBufferSize = 0;
	BOOL bRet = ::WinHttpQueryHeaders(hRequest.get(), InfoLevel, WINHTTP_HEADER_NAME_BY_INDEX, WINHTTP_NO_OUTPUT_BUFFER, &dwBufferSize, WINHTTP_NO_HEADER_INDEX);
	if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
		bRet = ::WinHttpQueryHeaders(hRequest.get(), InfoLevel, WINHTTP_HEADER_NAME_BY_INDEX, headerContents.GetBuffer(dwBufferSize / sizeof(WCHAR)), &dwBufferSize, WINHTTP_NO_HEADER_INDEX);
		headerContents.ReleaseBuffer();
		if (bRet)
			return true;
	}
	return false;
}

bool HttpQueryHeaders(const INetHandle& hRequest, DWORD InfoLevel, DWORD& headerContents)
{
	DWORD dwBufferSize = sizeof(headerContents);
	BOOL bRet = ::WinHttpQueryHeaders(hRequest.get(), InfoLevel | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, (LPVOID)&headerContents, &dwBufferSize, WINHTTP_NO_HEADER_INDEX);
	if (bRet) {
		return true;
	}
	return false;
}

bool HttpQueryRawHeaders(const INetHandle& hRequest, CString& rawHeaderContents)
{
	DWORD dwSize = 0;
	WinHttpQueryHeaders(hRequest.get(), WINHTTP_QUERY_RAW_HEADERS_CRLF,
		WINHTTP_HEADER_NAME_BY_INDEX, NULL,
		&dwSize, WINHTTP_NO_HEADER_INDEX);

	// Allocate memory for the buffer.
	if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
		// Now, use WinHttpQueryHeaders to retrieve the header.
		BOOL bResults = WinHttpQueryHeaders(hRequest.get(),
			WINHTTP_QUERY_RAW_HEADERS_CRLF,
			WINHTTP_HEADER_NAME_BY_INDEX,
			rawHeaderContents.GetBuffer(dwSize / sizeof(WCHAR)), &dwSize,
			WINHTTP_NO_HEADER_INDEX);
		rawHeaderContents.ReleaseBuffer();
		if (bResults) {
			return true;
		}
	}
	return false;
}

std::string HttpReadData(const INetHandle& hRequest)
{
	std::string	downloadData;
	DWORD	dwAvailableSize = 0;	
	do {
		dwAvailableSize = 0;
		if (::WinHttpQueryDataAvailable(hRequest.get(), &dwAvailableSize) == FALSE)
			BOOST_THROW_EXCEPTION(WinHTTPException());

		if (dwAvailableSize == 0)	// もうデータがない
			break;
		CTempBuffer<char>	buff;
		DWORD dwDownloaded = 0;
		if (::WinHttpReadData(hRequest.get(), buff.Allocate(dwAvailableSize + 1), dwAvailableSize, &dwDownloaded) == FALSE)
			BOOST_THROW_EXCEPTION(WinHTTPException());

		downloadData.append(buff, dwDownloaded);
	} while ( dwAvailableSize > 0 );
	return downloadData;
}

boost::optional<std::string>	HttpDownloadData(const CString& url)
{
	try {
		CUrl	downloadUrl(url);
		auto hConnect = HttpConnect(downloadUrl);
		auto hRequest = HttpOpenRequest(downloadUrl, hConnect);
		if (HttpSendRequestAndReceiveResponse(hRequest)) {
			if (HttpQueryStatusCode(hRequest) == 200) {
				return HttpReadData(hRequest);
			} else {
				return boost::optional<std::string>();
			}
		}
	} catch (boost::exception& e) {
		std::string expText = boost::diagnostic_information(e);
		int a = 0;
	}	
	return boost::optional<std::string>();
}


}	// namespace WinHTTPWrapper

