#include "TimeZoneManager.h"
#include <algorithm>
#include <map>
#include <sstream>
#include <iomanip>

TimeZoneManager& TimeZoneManager::Instance() {
    static TimeZoneManager instance;
    return instance;
}

std::wstring TimeZoneManager::GetRegistryDisplayName(const std::wstring& keyName) const {
    std::wstring subKey = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones\\" + keyName;
    HKEY hKey = NULL;
    std::wstring result = L"";

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, subKey.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        wchar_t buf[512] = { 0 };
        DWORD bufSize = sizeof(buf);
        DWORD type = REG_SZ;
        if (RegQueryValueExW(hKey, L"Display", NULL, &type, (LPBYTE)buf, &bufSize) == ERROR_SUCCESS) {
            result = buf;
        }
        RegCloseKey(hKey);
    }
    return result;
}

std::wstring TimeZoneManager::DetermineShortName(const std::wstring& keyName, int biasMinutes) const {
    // 主要なタイムゾーンの略称マッピング
    static const std::map<std::wstring, std::wstring> knownShortNames = {
        { L"Tokyo Standard Time", L"JST" },
        { L"UTC", L"UTC" },
        { L"GMT Standard Time", L"GMT/BST" },
        { L"Greenwich Standard Time", L"GMT" },
        { L"W. Europe Standard Time", L"CET/CEST" },
        { L"Central Europe Standard Time", L"CET" },
        { L"Romance Standard Time", L"PARIS" },
        { L"Eastern Standard Time", L"EST/EDT" },
        { L"Central Standard Time", L"CST/CDT" },
        { L"Mountain Standard Time", L"MST/MDT" },
        { L"Pacific Standard Time", L"PST/PDT" },
        { L"Alaskan Standard Time", L"AKST" },
        { L"Hawaiian Standard Time", L"HST" },
        { L"China Standard Time", L"CST" },
        { L"Singapore Standard Time", L"SGT" },
        { L"Korea Standard Time", L"KST" },
        { L"India Standard Time", L"IST" },
        { L"AUS Eastern Standard Time", L"AEST" },
        { L"Cen. Australia Standard Time", L"ACST" },
        { L"W. Australia Standard Time", L"AWST" },
        { L"New Zealand Standard Time", L"NZST" },
        { L"Taipei Standard Time", L"CST" },
        { L"Hong Kong Standard Time", L"HKT" },
        { L"Russian Standard Time", L"MSK" },
        { L"Arab Standard Time", L"AST" },
        { L"E. South America Standard Time", L"BRT" }
    };

    auto it = knownShortNames.find(keyName);
    if (it != knownShortNames.end()) {
        return it->second;
    }

    // マップにない場合、UTC オフセット文字列（例: "UTC+9", "UTC-5"）
    int totalMinutes = biasMinutes;
    int hours = totalMinutes / 60;
    int mins = std::abs(totalMinutes % 60);

    std::wostringstream woss;
    woss << L"UTC";
    if (hours >= 0) {
        woss << L"+" << hours;
    } else {
        woss << hours;
    }
    if (mins != 0) {
        woss << L":" << std::setw(2) << std::setfill(L'0') << mins;
    }
    return woss.str();
}

void TimeZoneManager::Initialize() {
    m_timeZones.clear();

    DWORD dwIndex = 0;
    DYNAMIC_TIME_ZONE_INFORMATION dtzi = { 0 };

    while (EnumDynamicTimeZoneInformation(dwIndex, &dtzi) == ERROR_SUCCESS) {
        dwIndex++;

        TimeZoneInfoItem item;
        item.keyName = dtzi.TimeZoneKeyName;
        item.standardName = dtzi.StandardName;
        item.dtzi = dtzi;

        // Windows では UTC = Local + Bias なので、オフセットは -Bias
        item.biasMinutes = -dtzi.Bias;

        // レジストリから表示名を取得
        item.displayName = GetRegistryDisplayName(item.keyName);
        if (item.displayName.empty()) {
            // レジストリから取れない場合は生成
            std::wostringstream woss;
            int h = item.biasMinutes / 60;
            int m = std::abs(item.biasMinutes % 60);
            woss << L"(UTC" << (h >= 0 ? L"+" : L"-")
                 << std::setw(2) << std::setfill(L'0') << std::abs(h) << L":"
                 << std::setw(2) << std::setfill(L'0') << m << L") "
                 << item.standardName;
            item.displayName = woss.str();
        }

        item.shortName = DetermineShortName(item.keyName, item.biasMinutes);

        m_timeZones.push_back(item);
    }

    // オフセット順にソート（西 -12:00 から東 +14:00、同じなら表示名順）
    std::sort(m_timeZones.begin(), m_timeZones.end(), [](const TimeZoneInfoItem& a, const TimeZoneInfoItem& b) {
        if (a.biasMinutes != b.biasMinutes) {
            return a.biasMinutes < b.biasMinutes;
        }
        return a.displayName < b.displayName;
    });
}

const TimeZoneInfoItem* TimeZoneManager::FindTimeZone(const std::wstring& keyName) const {
    if (keyName.empty()) return nullptr;

    for (const auto& item : m_timeZones) {
        if (item.keyName == keyName) {
            return &item;
        }
    }
    return nullptr;
}

TimeZoneInfoItem TimeZoneManager::GetSystemTimeZone() const {
    DYNAMIC_TIME_ZONE_INFORMATION dtzi = { 0 };
    DWORD dwRet = GetDynamicTimeZoneInformation(&dtzi);

    TimeZoneInfoItem item;
    item.keyName = dtzi.TimeZoneKeyName;
    item.standardName = dtzi.StandardName;
    item.dtzi = dtzi;
    item.biasMinutes = -dtzi.Bias;
    item.displayName = GetRegistryDisplayName(item.keyName);
    if (item.displayName.empty()) {
        item.displayName = item.standardName;
    }
    item.shortName = DetermineShortName(item.keyName, item.biasMinutes);
    return item;
}

bool TimeZoneManager::GetCurrentTimeInZone(
    const std::wstring& keyName,
    SYSTEMTIME& outLocal,
    double& outFractionalSeconds,
    std::wstring& outZoneLabel) const
{
    // 高精度な現在時刻を取得
    FILETIME ftPrecise;
    GetSystemTimePreciseAsFileTime(&ftPrecise);

    SYSTEMTIME utcTime;
    FileTimeToSystemTime(&ftPrecise, &utcTime);

    // ミリ秒の端数（マイクロ秒相当も考慮してスムーズな秒針を実現）
    ULARGE_INTEGER uli;
    uli.LowPart = ftPrecise.dwLowDateTime;
    uli.HighPart = ftPrecise.dwHighDateTime;
    // FILETIME は 100ナノ秒刻み。1秒 = 10,000,000カウント
    double subSecond = static_cast<double>(uli.QuadPart % 10000000) / 10000000.0;

    const TimeZoneInfoItem* target = nullptr;
    if (!keyName.empty()) {
        target = FindTimeZone(keyName);
    }

    if (target != nullptr) {
        // 指定されたタイムゾーンに変換
        if (!SystemTimeToTzSpecificLocalTimeEx(&target->dtzi, &utcTime, &outLocal)) {
            // 失敗時は通常のローカルタイム
            GetLocalTime(&outLocal);
            outZoneLabel = L"LOCAL";
        } else {
            outZoneLabel = target->shortName;
        }
    } else {
        // システムの現在タイムゾーン
        TimeZoneInfoItem sysTz = GetSystemTimeZone();
        if (!SystemTimeToTzSpecificLocalTimeEx(&sysTz.dtzi, &utcTime, &outLocal)) {
            GetLocalTime(&outLocal);
        }
        outZoneLabel = sysTz.shortName.empty() ? L"LOCAL" : sysTz.shortName;
    }

    outFractionalSeconds = static_cast<double>(outLocal.wSecond) + subSecond;
    return true;
}
