#pragma once
#include <windows.h>
#include <gdiplus.h>
#include <string>
#include "Settings.h"

class ClockRenderer {
public:
    ClockRenderer();
    ~ClockRenderer();

    // GDI+ の初期化と終了（アプリケーションレベル）
    static void InitializeGDIPlus();
    static void ShutdownGDIPlus();

    // 時計の描画
    // hdc: 描画対象のデバイスコンテキスト
    // width, height: クライアント領域のサイズ
    // st: 表示する日時
    // fractionalSeconds: スイープ秒針用の秒（0.000 ～ 59.999...）
    // zoneLabel: 文字盤に表示するタイムゾーン名
    // settings: 現在の設定
    void Render(
        HDC hdc,
        int width,
        int height,
        const SYSTEMTIME& st,
        double fractionalSeconds,
        const std::wstring& zoneLabel,
        const Settings& settings);

private:
    // カラーパレット構造体
    struct ThemeColors {
        Gdiplus::Color outerBezel1;
        Gdiplus::Color outerBezel2;
        Gdiplus::Color dialBg1;
        Gdiplus::Color dialBg2;
        Gdiplus::Color tickMinute;
        Gdiplus::Color tickHour;
        Gdiplus::Color numberText;
        Gdiplus::Color infoText;
        Gdiplus::Color hourHand;
        Gdiplus::Color minuteHand;
        Gdiplus::Color secondHand;
        Gdiplus::Color centerCap;
        Gdiplus::Color dateBoxBg;
        Gdiplus::Color dateBoxBorder;
    };

    ThemeColors GetThemeColors(ClockTheme theme) const;

    // 各要素の描画ヘルパー
    void DrawBezelAndDial(Gdiplus::Graphics& g, float cx, float cy, float radius, const ThemeColors& colors);
    void DrawTicksAndNumbers(Gdiplus::Graphics& g, float cx, float cy, float radius, const ThemeColors& colors);
    void DrawDialInfo(Gdiplus::Graphics& g, float cx, float cy, float radius, const SYSTEMTIME& st, const std::wstring& zoneLabel, const Settings& settings, const ThemeColors& colors);
    void DrawHands(Gdiplus::Graphics& g, float cx, float cy, float radius, const SYSTEMTIME& st, double fractionalSec, const Settings& settings, const ThemeColors& colors);

    static ULONG_PTR s_gdiplusToken;
};
