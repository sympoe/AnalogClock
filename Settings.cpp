#include "Settings.h"
#include <shlobj.h>
#include <vector>
#include <algorithm>

Settings::Settings() {
    ResetToDefaults();
}

void Settings::ResetToDefaults() {
    windowX = 100;
    windowY = 100;
    windowWidth = 260;
    windowHeight = 260;
    hasWindowBorder = false;
    alwaysOnTop = false;

    timeZoneKeyName = L"";       // デフォルトはシステムローカル
    timeZoneCustomName = L"";
    smoothSeconds = true;
    showSeconds = true;
    showDate = true;
    showDigital = false;
    theme = static_cast<int>(ClockTheme::DarkModern);
}

std::wstring Settings::GetIniFilePath() const {
    // 1. EXEと同じディレクトリの settings.ini を優先して試す（ポータブル動作）
    wchar_t exePath[MAX_PATH] = { 0 };
    if (GetModuleFileNameW(NULL, exePath, MAX_PATH) > 0) {
        wchar_t* lastSlash = wcsrchr(exePath, L'\\');
        if (lastSlash) {
            *lastSlash = L'\0';
            std::wstring localIni = std::wstring(exePath) + L"\\clock_settings.ini";
            // すでに存在するか、書き込み可能かテスト
            DWORD attr = GetFileAttributesW(localIni.c_str());
            if (attr != INVALID_FILE_ATTRIBUTES) {
                return localIni;
            }
            // 新規作成できるかテスト
            HANDLE hTest = CreateFileW(localIni.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL,
                OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hTest != INVALID_HANDLE_VALUE) {
                CloseHandle(hTest);
                return localIni;
            }
        }
    }

    // 2. フォールバック: %APPDATA%\AnalogClock\clock_settings.ini
    wchar_t appDataPath[MAX_PATH] = { 0 };
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
        std::wstring dir = std::wstring(appDataPath) + L"\\AnalogClock";
        CreateDirectoryW(dir.c_str(), NULL);
        return dir + L"\\clock_settings.ini";
    }

    return L".\\clock_settings.ini";
}

void Settings::Load() {
    std::wstring iniPath = GetIniFilePath();

    windowX = GetPrivateProfileIntW(L"Window", L"X", windowX, iniPath.c_str());
    windowY = GetPrivateProfileIntW(L"Window", L"Y", windowY, iniPath.c_str());
    windowWidth = GetPrivateProfileIntW(L"Window", L"Width", windowWidth, iniPath.c_str());
    windowHeight = GetPrivateProfileIntW(L"Window", L"Height", windowHeight, iniPath.c_str());
    hasWindowBorder = GetPrivateProfileIntW(L"Window", L"HasBorder", hasWindowBorder ? 1 : 0, iniPath.c_str()) != 0;
    alwaysOnTop = GetPrivateProfileIntW(L"Window", L"AlwaysOnTop", alwaysOnTop ? 1 : 0, iniPath.c_str()) != 0;

    // サイズの最小・最大値ガード
    if (windowWidth < 120) windowWidth = 120;
    if (windowHeight < 120) windowHeight = 120;
    if (windowWidth > 1200) windowWidth = 1200;
    if (windowHeight > 1200) windowHeight = 1200;

    wchar_t tzBuf[256] = { 0 };
    GetPrivateProfileStringW(L"Clock", L"TimeZoneKey", L"", tzBuf, 256, iniPath.c_str());
    timeZoneKeyName = tzBuf;

    wchar_t tzNameBuf[256] = { 0 };
    GetPrivateProfileStringW(L"Clock", L"TimeZoneCustomName", L"", tzNameBuf, 256, iniPath.c_str());
    timeZoneCustomName = tzNameBuf;

    smoothSeconds = GetPrivateProfileIntW(L"Clock", L"SmoothSeconds", smoothSeconds ? 1 : 0, iniPath.c_str()) != 0;
    showSeconds = GetPrivateProfileIntW(L"Clock", L"ShowSeconds", showSeconds ? 1 : 0, iniPath.c_str()) != 0;
    showDate = GetPrivateProfileIntW(L"Clock", L"ShowDate", showDate ? 1 : 0, iniPath.c_str()) != 0;
    showDigital = GetPrivateProfileIntW(L"Clock", L"ShowDigital", showDigital ? 1 : 0, iniPath.c_str()) != 0;
    theme = GetPrivateProfileIntW(L"Clock", L"Theme", theme, iniPath.c_str());

    EnsureVisibleOnScreen();
}

void Settings::Save() const {
    std::wstring iniPath = GetIniFilePath();

    WritePrivateProfileStringW(L"Window", L"X", std::to_wstring(windowX).c_str(), iniPath.c_str());
    WritePrivateProfileStringW(L"Window", L"Y", std::to_wstring(windowY).c_str(), iniPath.c_str());
    WritePrivateProfileStringW(L"Window", L"Width", std::to_wstring(windowWidth).c_str(), iniPath.c_str());
    WritePrivateProfileStringW(L"Window", L"Height", std::to_wstring(windowHeight).c_str(), iniPath.c_str());
    WritePrivateProfileStringW(L"Window", L"HasBorder", hasWindowBorder ? L"1" : L"0", iniPath.c_str());
    WritePrivateProfileStringW(L"Window", L"AlwaysOnTop", alwaysOnTop ? L"1" : L"0", iniPath.c_str());

    WritePrivateProfileStringW(L"Clock", L"TimeZoneKey", timeZoneKeyName.c_str(), iniPath.c_str());
    WritePrivateProfileStringW(L"Clock", L"TimeZoneCustomName", timeZoneCustomName.c_str(), iniPath.c_str());
    WritePrivateProfileStringW(L"Clock", L"SmoothSeconds", smoothSeconds ? L"1" : L"0", iniPath.c_str());
    WritePrivateProfileStringW(L"Clock", L"ShowSeconds", showSeconds ? L"1" : L"0", iniPath.c_str());
    WritePrivateProfileStringW(L"Clock", L"ShowDate", showDate ? L"1" : L"0", iniPath.c_str());
    WritePrivateProfileStringW(L"Clock", L"ShowDigital", showDigital ? L"1" : L"0", iniPath.c_str());
    WritePrivateProfileStringW(L"Clock", L"Theme", std::to_wstring(theme).c_str(), iniPath.c_str());
}

void Settings::EnsureVisibleOnScreen() {
    RECT rc = { windowX, windowY, windowX + windowWidth, windowY + windowHeight };
    HMONITOR hMon = MonitorFromRect(&rc, MONITOR_DEFAULTTONULL);
    if (hMon == NULL) {
        // どのモニターの範囲にもない場合、プライマリモニターの作業領域内に収める
        RECT workArea;
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);

        windowX = workArea.left + 50;
        windowY = workArea.top + 50;
        if (windowWidth > (workArea.right - workArea.left)) {
            windowWidth = (workArea.right - workArea.left) / 2;
        }
        if (windowHeight > (workArea.bottom - workArea.top)) {
            windowHeight = (workArea.bottom - workArea.top) / 2;
        }
    }
}
