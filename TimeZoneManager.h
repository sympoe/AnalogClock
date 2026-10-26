#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <memory>

struct TimeZoneInfoItem {
    std::wstring keyName;       // 内部識別子 (例: "Tokyo Standard Time")
    std::wstring displayName;   // レジストリの Display 名 (例: "(UTC+09:00) 大阪、札幌、東京")
    std::wstring standardName;  // 標準名 (例: "東京 (標準時)")
    std::wstring shortName;     // 略称 (例: "JST", "UTC", "EST" 等)
    int biasMinutes;            // UTC からのオフセット（分）。正が東、負が西 (例: +540 は UTC+9)
    DYNAMIC_TIME_ZONE_INFORMATION dtzi;
};

class TimeZoneManager {
public:
    static TimeZoneManager& Instance();

    // タイムゾーン一覧を初期化・読み込み
    void Initialize();

    // 全タイムゾーンリスト取得（オフセット順）
    const std::vector<TimeZoneInfoItem>& GetAllTimeZones() const { return m_timeZones; }

    // キー名からタイムゾーン情報を検索（見つからなければnullptr）
    const TimeZoneInfoItem* FindTimeZone(const std::wstring& keyName) const;

    // システム（ローカルPC）のタイムゾーン情報を取得
    TimeZoneInfoItem GetSystemTimeZone() const;

    // 現在時刻を指定タイムゾーンで取得
    // outLocal: 変換後の現地時刻
    // outFractionalSeconds: 秒 + ミリ秒/1000 (0.000 ～ 59.999...) スイープ秒針用
    // outZoneLabel: 文字盤に表示するラベル (例: "JST", "UTC+9", "NEW YORK" など)
    bool GetCurrentTimeInZone(
        const std::wstring& keyName,
        SYSTEMTIME& outLocal,
        double& outFractionalSeconds,
        std::wstring& outZoneLabel) const;

private:
    TimeZoneManager() = default;
    std::vector<TimeZoneInfoItem> m_timeZones;

    // レジストリから Display 名を取得
    std::wstring GetRegistryDisplayName(const std::wstring& keyName) const;

    // キー名から一般的な略称（JST, EST, UTC等）を推定
    std::wstring DetermineShortName(const std::wstring& keyName, int biasMinutes) const;
};
