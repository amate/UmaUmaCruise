#pragma once

#include <Windows.h>
#include <Uxtheme.h>

extern bool g_darkModeSupported;
extern bool g_darkModeEnabled;
extern DWORD g_buildNumber;

bool AllowDarkModeForWindow(HWND hWnd, bool allow);

bool IsHighContrast();

void RefreshTitleBarThemeColor(HWND hWnd);

bool IsColorSchemeChangeMessage(LPARAM lParam);

bool IsColorSchemeChangeMessage(UINT message, LPARAM lParam);

void AllowDarkModeForApp(bool allow);

void FixDarkScrollBar();

void InitDarkMode();

bool UpdateDarkModeEnabled();

