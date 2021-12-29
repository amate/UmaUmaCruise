#pragma once

#include "IScreenShotWindow.h"

#include "Utility\Logger.h"
#include "Utility\CommonUtility.h"

class WindowsGraphicsCaptureWrapper
{
public:

	static bool IsSupported()
	{
		using fnRtlGetNtVersionNumbers = void (WINAPI*)(LPDWORD major, LPDWORD minor, LPDWORD build);
		auto RtlGetNtVersionNumbers =
			reinterpret_cast<fnRtlGetNtVersionNumbers>(
				GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetNtVersionNumbers"));
		ATLASSERT(RtlGetNtVersionNumbers);
		DWORD major, minor, buildNumber;
		RtlGetNtVersionNumbers(&major, &minor, &buildNumber);
		buildNumber &= ~0xF0000000;
		if (major >= 10 && buildNumber >= 19041) {
			//bool b = winrt::GraphicsCaptureSession::IsSupported();
			//return b;
			return true;
		}
		return false;
	}

	static bool	LoadLibrary()
	{
		if (!IsSupported()) {
			return false;
		}

		ATLASSERT(!s_hDll);
		s_hDll = ::LoadLibrary((GetExeDirectory() / L"WindowsGraphicsCapture.dll").wstring().c_str());
		ATLASSERT(s_hDll);
		if (!s_hDll) {
			ERROR_LOG << L"WindowsGraphicsCapture.dll の読み込みに失敗しました";
			return false;

		} else {
			s_func_CreateWindowsGraphicsCapture = (func_CreateWindowsGraphicsCapture)::GetProcAddress(s_hDll, "CreateWindowsGraphicsCapture");
			ATLASSERT(s_func_CreateWindowsGraphicsCapture);
			if (s_func_CreateWindowsGraphicsCapture) {
				return true;
			} else {
				return false;
			}
		}
	}

	static void	FreeLibrary()
	{
		ATLASSERT(IsDllLoaded());
		s_func_CreateWindowsGraphicsCapture = nullptr;
		::FreeLibrary(s_hDll);
		s_hDll = NULL;
	}

	static bool IsDllLoaded()
	{
		bool b = s_hDll && s_func_CreateWindowsGraphicsCapture;
		return b;
	}

	static IScreenShotWindow* CreateWindowsGraphicsCapture()
	{
		ATLASSERT(s_func_CreateWindowsGraphicsCapture);
		return s_func_CreateWindowsGraphicsCapture();
	}

private:
	using func_CreateWindowsGraphicsCapture = IScreenShotWindow * (*)();
	static inline func_CreateWindowsGraphicsCapture	s_func_CreateWindowsGraphicsCapture = nullptr;
	static inline HMODULE s_hDll = NULL;
};
