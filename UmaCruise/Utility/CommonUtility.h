
// CommonUtility.h

#pragma once

#include <boost\filesystem.hpp>
#include <unordered_map>
#include <Windows.h>
#include <boost\optional.hpp>

namespace fs = boost::filesystem;

/// 現在実行中の exeのあるフォルダのパスを返す
fs::path GetExeDirectory();

/// 例外を発生させる
#define THROWEXCEPTION(error)	FatalErrorOccur(error, __FILE__,__LINE__)

void	FatalErrorOccur(const std::string& error, const char* fileName, const int line);
void	FatalErrorOccur(const std::wstring& error, const char* fileName, const int line);

/// プロセスを起動し、終了まで待つ
DWORD	StartProcess(const fs::path& exePath, const std::wstring& commandLine);

DWORD	StartProcessGetStdOut(const fs::path& exePath, const std::wstring& commandLine, std::string& stdoutText);

fs::path	SearchFirstFile(const fs::path& search);

std::string	LoadFile(const fs::path& filePath);
CComPtr<IStream>	LoadFileToMemoryStream(const fs::path& filePath);

void		SaveFile(const fs::path& filePath, const std::string& content);










