
#include "stdafx.h"
#include "CommonUtility.h"
#include <fstream>
#include <regex>
#include <Windows.h>
#include <atlbase.h>
#include "Logger.h"
#include "CodeConvert.h"

/// 現在実行中の exeのあるフォルダのパスを返す
fs::path GetExeDirectory()
{
	WCHAR exePath[MAX_PATH] = L"";
	GetModuleFileName(NULL, exePath, MAX_PATH);
	fs::path exeFolder = exePath;
	return exeFolder.parent_path();
}


void	FatalErrorOccur(const std::string& error, const char* fileName, const int line)
{
	ERROR_LOG << L"fileName: " << fileName << L" line: " << line << L"\n" << error;
	throw std::exception(error.c_str());
}

void	FatalErrorOccur(const std::wstring& error, const char* fileName, const int line)
{
	ERROR_LOG << L"fileName: " << fileName << L" line: " << line << L"\n" << error;
	throw std::exception(CodeConvert::ShiftJISfromUTF16(error).c_str());
}

/// プロセスを起動し、終了まで待つ
DWORD	StartProcess(const fs::path& exePath, const std::wstring& commandLine)
{
	//INFO_LOG << L"StartProcess\n" << L"\"" << exePath << L"\" " << commandLine;

	STARTUPINFO startUpInfo = { sizeof(STARTUPINFO) };
	startUpInfo.dwFlags = STARTF_USESHOWWINDOW;
	startUpInfo.wShowWindow = SW_HIDE;
	PROCESS_INFORMATION processInfo = {};
	SECURITY_ATTRIBUTES securityAttributes = { sizeof(SECURITY_ATTRIBUTES) };
	BOOL bRet = ::CreateProcess(exePath.native().c_str(), (LPWSTR)commandLine.data(),
		nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startUpInfo, &processInfo);
	if (bRet == 0) {
		THROWEXCEPTION(L"StartProcess");
	}
	//ATLASSERT(bRet);
	::WaitForSingleObject(processInfo.hProcess, INFINITE);
	DWORD dwExitCode = 0;
	GetExitCodeProcess(processInfo.hProcess, &dwExitCode);

	::CloseHandle(processInfo.hThread);
	::CloseHandle(processInfo.hProcess);

	return dwExitCode;
}

DWORD	StartProcessGetStdOut(const fs::path& exePath, const std::wstring& commandLine, std::string& stdoutText)
{
	//INFO_LOG << L"StartProcessGetStdOut\n" << L"\"" << exePath << L"\" " << commandLine;

	try {
		CHandle hReadPipe;
		CHandle hWritePipe;
		if (!::CreatePipe(&hReadPipe.m_h, &hWritePipe.m_h, nullptr, 0))
			throw std::runtime_error("CreatePipe failed");

		CHandle hStdOutput;
		if (!::DuplicateHandle(GetCurrentProcess(), hWritePipe, GetCurrentProcess(), &hStdOutput.m_h, 0, TRUE, DUPLICATE_SAME_ACCESS))
			throw std::runtime_error("DuplicateHandle failed");

		hWritePipe.Close();

		STARTUPINFO startUpInfo = { sizeof(STARTUPINFO) };
		startUpInfo.dwFlags = STARTF_USESTDHANDLES;
		startUpInfo.hStdOutput = hStdOutput;
		//startUpInfo.hStdError = hStdOutput;
		PROCESS_INFORMATION processInfo = {};

		BOOL bRet = ::CreateProcess(exePath.c_str(), (LPWSTR)commandLine.data(),
			nullptr, nullptr, TRUE, 0, nullptr, nullptr, &startUpInfo, &processInfo);
		ATLASSERT(bRet);

		hStdOutput.Close();

		std::string outresult;

		for (;;) {
			enum { kBufferSize = 512 };
			char buffer[kBufferSize + 1] = "";
			DWORD readSize = 0;
			bRet = ::ReadFile(hReadPipe, (LPVOID)buffer, kBufferSize, &readSize, nullptr);
			if (bRet && readSize == 0) { // EOF
				break;
			}

			if (bRet == 0) {
				DWORD err = ::GetLastError();
				if (err == ERROR_BROKEN_PIPE) {
					break;
				}
			}
			outresult.append(buffer, readSize);
		}
		stdoutText = outresult;

		hReadPipe.Close();

		DWORD dwExitCode = 0;
		GetExitCodeProcess(processInfo.hProcess, &dwExitCode);

		::WaitForSingleObject(processInfo.hProcess, INFINITE);
		::CloseHandle(processInfo.hThread);
		::CloseHandle(processInfo.hProcess);

		return dwExitCode;
	}
	catch (std::exception& e)
	{
		THROWEXCEPTION(e.what());
		return -1;
	}
}

fs::path	SearchFirstFile(const fs::path& search)
{
	WIN32_FIND_DATA fd = {};
	HANDLE hFind = ::FindFirstFileW(search.c_str(), &fd);
	if (hFind == INVALID_HANDLE_VALUE)
		return L"";

	::FindClose(hFind);

	fs::path firstFilePath = search.parent_path() / fd.cFileName;
	return firstFilePath;
}


std::string	LoadFile(const fs::path& filePath)
{
	std::ifstream fs(filePath.wstring(), std::ios::in | std::ios::binary);
	if (!fs)
		return "";

	fs.seekg(0, std::ios::end);
	std::string buff;
	auto fileSize = fs.tellg();
	buff.resize(fileSize);
	fs.seekg(std::ios::beg);
	fs.clear();
	fs.read(const_cast<char*>(buff.data()), fileSize);
	return buff;
}

CComPtr<IStream>	LoadFileToMemoryStream(const fs::path& filePath)
{
	std::ifstream fs(filePath.wstring(), std::ios::in | std::ios::binary);
	if (!fs)
		return nullptr;

	fs.seekg(0, std::ios::end);
	std::string buff;
	auto fileSize = fs.tellg();
	buff.resize(fileSize);
	fs.seekg(std::ios::beg);
	fs.clear();

	HGLOBAL	hMem = ::GlobalAlloc(GMEM_FIXED, fileSize);
	if (!hMem) {
		return nullptr;
	}
	fs.read(static_cast<char*>(hMem), fileSize);

	CComPtr<IStream> spStream;
	HRESULT hr = ::CreateStreamOnHGlobal(hMem, TRUE, &spStream);
	return spStream;
}

void	SaveFile(const fs::path& filePath, const std::string& content)
{
	std::ofstream fs(filePath.wstring(), std::ios::out | std::ios::binary);
	if (!fs) {
		THROWEXCEPTION("SaveFile open failed");
	}
	fs.write(content.c_str(), content.length());
}

