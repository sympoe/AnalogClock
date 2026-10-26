#pragma once
#include <windows.h>
#include <string>

enum class ClockTheme {
    DarkModern = 0,
    LightElegant = 1,
    ClassicGold = 2,
    CyberCyan = 3
};

class Settings {
public:
    Settings();

    // ウィンドウ状態
    int windowX;
    int windowY;
    int windowWidth;
    int windowHeight;
    bool hasWindowBorder;
    bool alwaysOnTop;

    // 時計の表示・動作設定
    std::wstring timeZoneKeyName;    // 空ならシステムのタイムゾーン
    std::wstring timeZoneCustomName; // 表示用カスタム名（空なら標準名）
    bool smoothSeconds;              // 滑らかな秒針（スイープ運針）
    bool showSeconds;                // 秒針の表示
    bool showDate;                   // 日付表示
    bool showDigital;                // デジタル時刻表示
    int theme;                       // ClockTheme

    // 設定ファイルパス
    std::wstring GetIniFilePath() const;

    // 設定の読み込みと保存
    void Load();
    void Save() const;

    // 画面外に出ていないか確認し補正
    void EnsureVisibleOnScreen();

    // デフォルト値にリセット
    void ResetToDefaults();
};
