#include "ClockRenderer.h"
#include <cmath>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "gdiplus.lib")

ULONG_PTR ClockRenderer::s_gdiplusToken = 0;

void ClockRenderer::InitializeGDIPlus() {
    Gdiplus::GdiplusStartupInput input;
    Gdiplus::GdiplusStartup(&s_gdiplusToken, &input, NULL);
}

void ClockRenderer::ShutdownGDIPlus() {
    if (s_gdiplusToken != 0) {
        Gdiplus::GdiplusShutdown(s_gdiplusToken);
        s_gdiplusToken = 0;
    }
}

ClockRenderer::ClockRenderer() {}
ClockRenderer::~ClockRenderer() {}

ClockRenderer::ThemeColors ClockRenderer::GetThemeColors(ClockTheme theme) const {
    ThemeColors c;
    switch (theme) {
    case ClockTheme::LightElegant:
        c.outerBezel1   = Gdiplus::Color(255, 230, 233, 238);
        c.outerBezel2   = Gdiplus::Color(255, 175, 182, 192);
        c.dialBg1       = Gdiplus::Color(255, 255, 255, 255);
        c.dialBg2       = Gdiplus::Color(255, 240, 243, 246);
        c.tickMinute    = Gdiplus::Color(140, 110, 120, 135);
        c.tickHour      = Gdiplus::Color(255, 25, 35, 48);
        c.numberText    = Gdiplus::Color(255, 20, 28, 38);
        c.infoText      = Gdiplus::Color(200, 70, 80, 95);
        c.hourHand      = Gdiplus::Color(255, 22, 36, 56);
        c.minuteHand    = Gdiplus::Color(255, 22, 36, 56);
        c.secondHand    = Gdiplus::Color(255, 217, 72, 15);
        c.centerCap     = Gdiplus::Color(255, 30, 45, 65);
        c.dateBoxBg     = Gdiplus::Color(255, 245, 247, 250);
        c.dateBoxBorder = Gdiplus::Color(255, 190, 198, 208);
        break;

    case ClockTheme::ClassicGold:
        c.outerBezel1   = Gdiplus::Color(255, 218, 185, 120);
        c.outerBezel2   = Gdiplus::Color(255, 125, 95, 45);
        c.dialBg1       = Gdiplus::Color(255, 20, 32, 28);
        c.dialBg2       = Gdiplus::Color(255, 10, 18, 15);
        c.tickMinute    = Gdiplus::Color(150, 185, 155, 95);
        c.tickHour      = Gdiplus::Color(255, 235, 200, 130);
        c.numberText    = Gdiplus::Color(255, 240, 210, 145);
        c.infoText      = Gdiplus::Color(200, 190, 165, 110);
        c.hourHand      = Gdiplus::Color(255, 235, 205, 135);
        c.minuteHand    = Gdiplus::Color(255, 235, 205, 135);
        c.secondHand    = Gdiplus::Color(255, 230, 65, 40);
        c.centerCap     = Gdiplus::Color(255, 240, 210, 145);
        c.dateBoxBg     = Gdiplus::Color(255, 15, 25, 22);
        c.dateBoxBorder = Gdiplus::Color(255, 160, 130, 75);
        break;

    case ClockTheme::CyberCyan:
        c.outerBezel1   = Gdiplus::Color(255, 0, 229, 255);
        c.outerBezel2   = Gdiplus::Color(255, 10, 25, 45);
        c.dialBg1       = Gdiplus::Color(255, 8, 12, 18);
        c.dialBg2       = Gdiplus::Color(255, 4, 6, 10);
        c.tickMinute    = Gdiplus::Color(120, 0, 180, 216);
        c.tickHour      = Gdiplus::Color(255, 0, 240, 255);
        c.numberText    = Gdiplus::Color(255, 0, 245, 255);
        c.infoText      = Gdiplus::Color(200, 0, 180, 216);
        c.hourHand      = Gdiplus::Color(255, 0, 230, 255);
        c.minuteHand    = Gdiplus::Color(255, 0, 200, 240);
        c.secondHand    = Gdiplus::Color(255, 255, 0, 110);
        c.centerCap     = Gdiplus::Color(255, 0, 245, 255);
        c.dateBoxBg     = Gdiplus::Color(255, 12, 20, 30);
        c.dateBoxBorder = Gdiplus::Color(255, 0, 150, 190);
        break;

    case ClockTheme::DarkModern:
    default:
        c.outerBezel1   = Gdiplus::Color(255, 75, 80, 88);
        c.outerBezel2   = Gdiplus::Color(255, 30, 32, 36);
        c.dialBg1       = Gdiplus::Color(255, 32, 34, 38);
        c.dialBg2       = Gdiplus::Color(255, 20, 21, 24);
        c.tickMinute    = Gdiplus::Color(100, 160, 165, 175);
        c.tickHour      = Gdiplus::Color(255, 230, 235, 240);
        c.numberText    = Gdiplus::Color(255, 240, 243, 246);
        c.infoText      = Gdiplus::Color(180, 150, 156, 166);
        c.hourHand      = Gdiplus::Color(255, 235, 238, 242);
        c.minuteHand    = Gdiplus::Color(255, 220, 224, 230);
        c.secondHand    = Gdiplus::Color(255, 230, 57, 70);
        c.centerCap     = Gdiplus::Color(255, 230, 235, 240);
        c.dateBoxBg     = Gdiplus::Color(255, 28, 30, 34);
        c.dateBoxBorder = Gdiplus::Color(255, 60, 64, 72);
        break;
    }
    return c;
}

void ClockRenderer::DrawBezelAndDial(
    Gdiplus::Graphics& g,
    float cx, float cy, float radius,
    const ThemeColors& colors)
{
    using namespace Gdiplus;

    // 1. 外周ベゼル（二重リング・グラデーション）
    RectF outerRect(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
    LinearGradientBrush bezelBrush(
        PointF(cx - radius, cy - radius),
        PointF(cx + radius, cy + radius),
        colors.outerBezel1,
        colors.outerBezel2
    );
    g.FillEllipse(&bezelBrush, outerRect);

    // ベゼルと文字盤の間のインナーシャドウリング
    float innerR = radius * 0.94f;
    RectF innerBezelRect(cx - innerR, cy - innerR, innerR * 2.0f, innerR * 2.0f);
    Pen innerRimPen(Color(100, 0, 0, 0), radius * 0.02f);
    g.DrawEllipse(&innerRimPen, innerBezelRect);

    // 2. 文字盤本体（円形グラデーション背景）
    float dialR = radius * 0.92f;
    RectF dialRect(cx - dialR, cy - dialR, dialR * 2.0f, dialR * 2.0f);
    LinearGradientBrush dialBrush(
        PointF(cx, cy - dialR),
        PointF(cx, cy + dialR),
        colors.dialBg1,
        colors.dialBg2
    );
    g.FillEllipse(&dialBrush, dialRect);

    // 文字盤外周の微細なサークルライン
    Pen dialBorderPen(Color(40, 255, 255, 255), 1.0f);
    g.DrawEllipse(&dialBorderPen, dialRect);
}

void ClockRenderer::DrawTicksAndNumbers(
    Gdiplus::Graphics& g,
    float cx, float cy, float radius,
    const ThemeColors& colors)
{
    using namespace Gdiplus;

    float dialR = radius * 0.92f;
    float tickOuterR = dialR * 0.96f;
    float tickMinInnerR = dialR * 0.91f;
    float tickHourInnerR = dialR * 0.85f;

    Pen minPen(colors.tickMinute, radius * 0.012f);
    Pen hourPen(colors.tickHour, radius * 0.026f);
    hourPen.SetStartCap(LineCapRound);
    hourPen.SetEndCap(LineCapRound);

    const double PI = 3.14159265358979323846;

    // 60本の目盛り
    for (int i = 0; i < 60; ++i) {
        double angleRad = (i * 6.0) * (PI / 180.0);
        float sinA = static_cast<float>(sin(angleRad));
        float cosA = static_cast<float>(cos(angleRad));

        float x1 = cx + tickOuterR * sinA;
        float y1 = cy - tickOuterR * cosA;

        if (i % 5 == 0) {
            // 5分毎の太い目盛り
            float x2 = cx + tickHourInnerR * sinA;
            float y2 = cy - tickHourInnerR * cosA;
            g.DrawLine(&hourPen, x1, y1, x2, y2);
        } else {
            // 1分毎の細い目盛り
            float x2 = cx + tickMinInnerR * sinA;
            float y2 = cy - tickMinInnerR * cosA;
            g.DrawLine(&minPen, x1, y1, x2, y2);
        }
    }

    // 数字の描画 (1 ～ 12)
    float numberR = dialR * 0.74f;
    float fontSize = radius * 0.16f;
    if (fontSize < 8.0f) fontSize = 8.0f;

    FontFamily fontFamily(L"Segoe UI");
    Font font(&fontFamily, fontSize, FontStyleBold, UnitPixel);
    SolidBrush textBrush(colors.numberText);
    StringFormat sf;
    sf.SetAlignment(StringAlignmentCenter);
    sf.SetLineAlignment(StringAlignmentCenter);

    for (int num = 1; num <= 12; ++num) {
        double angleRad = (num * 30.0) * (PI / 180.0);
        float sinA = static_cast<float>(sin(angleRad));
        float cosA = static_cast<float>(cos(angleRad));

        float nx = cx + numberR * sinA;
        float ny = cy - numberR * cosA;

        std::wstring numStr = std::to_wstring(num);
        RectF numBox(nx - fontSize, ny - fontSize, fontSize * 2.0f, fontSize * 2.0f);
        g.DrawString(numStr.c_str(), -1, &font, numBox, &sf, &textBrush);
    }
}

void ClockRenderer::DrawDialInfo(
    Gdiplus::Graphics& g,
    float cx, float cy, float radius,
    const SYSTEMTIME& st,
    const std::wstring& zoneLabel,
    const Settings& settings,
    const ThemeColors& colors)
{
    using namespace Gdiplus;

    float dialR = radius * 0.92f;
    FontFamily fontFamily(L"Segoe UI");
    StringFormat sf;
    sf.SetAlignment(StringAlignmentCenter);
    sf.SetLineAlignment(StringAlignmentCenter);

    // 1. タイムゾーン表示（文字盤上部 12時の少し下）
    if (!zoneLabel.empty()) {
        float zoneFontSize = radius * 0.09f;
        if (zoneFontSize < 7.0f) zoneFontSize = 7.0f;
        Font zoneFont(&fontFamily, zoneFontSize, FontStyleBold, UnitPixel);
        SolidBrush zoneBrush(colors.infoText);

        RectF zoneBox(cx - dialR * 0.4f, cy - dialR * 0.38f, dialR * 0.8f, zoneFontSize * 1.6f);
        g.DrawString(zoneLabel.c_str(), -1, &zoneFont, zoneBox, &sf, &zoneBrush);
    }

    // 2. 日付表示（文字盤下部 または 3時位置の小窓）
    if (settings.showDate) {
        static const wchar_t* const dayNames[] = {
            L"SUN", L"MON", L"TUE", L"WED", L"THU", L"FRI", L"SAT"
        };
        const wchar_t* wdayStr = (st.wDayOfWeek <= 6) ? dayNames[st.wDayOfWeek] : L"";

        std::wostringstream woss;
        woss << st.wDay << L" " << wdayStr;
        std::wstring dateStr = woss.str();

        float dateFontSize = radius * 0.08f;
        if (dateFontSize < 7.0f) dateFontSize = 7.0f;
        Font dateFont(&fontFamily, dateFontSize, FontStyleBold, UnitPixel);

        // 3時位置の内側に日付窓を配置
        float boxW = radius * 0.36f;
        float boxH = radius * 0.14f;
        float boxX = cx + dialR * 0.22f;
        float boxY = cy - boxH * 0.5f;

        RectF dateBox(boxX, boxY, boxW, boxH);
        SolidBrush boxBg(colors.dateBoxBg);
        Pen boxBorder(colors.dateBoxBorder, 1.0f);
        g.FillRectangle(&boxBg, dateBox);
        g.DrawRectangle(&boxBorder, dateBox);

        SolidBrush dateTextBrush(colors.numberText);
        g.DrawString(dateStr.c_str(), -1, &dateFont, dateBox, &sf, &dateTextBrush);
    }

    // 3. デジタル時刻表示（設定されている場合、文字盤下部 6時位置の上）
    if (settings.showDigital) {
        std::wostringstream woss;
        woss << std::setfill(L'0')
             << std::setw(2) << st.wHour << L":"
             << std::setw(2) << st.wMinute;
        if (settings.showSeconds) {
            woss << L":" << std::setw(2) << st.wSecond;
        }

        std::wstring digiStr = woss.str();
        float digiFontSize = radius * 0.09f;
        if (digiFontSize < 7.0f) digiFontSize = 7.0f;
        Font digiFont(&fontFamily, digiFontSize, FontStyleRegular, UnitPixel);
        SolidBrush digiBrush(colors.infoText);

        RectF digiBox(cx - dialR * 0.4f, cy + dialR * 0.32f, dialR * 0.8f, digiFontSize * 1.6f);
        g.DrawString(digiStr.c_str(), -1, &digiFont, digiBox, &sf, &digiBrush);
    }
}

void ClockRenderer::DrawHands(
    Gdiplus::Graphics& g,
    float cx, float cy, float radius,
    const SYSTEMTIME& st,
    double fractionalSec,
    const Settings& settings,
    const ThemeColors& colors)
{
    using namespace Gdiplus;

    float dialR = radius * 0.92f;

    // 角度計算
    double secVal = settings.smoothSeconds ? fractionalSec : static_cast<double>(st.wSecond);
    float secAngle = static_cast<float>(secVal * 6.0); // 360 / 60
    float minAngle = static_cast<float>((st.wMinute + secVal / 60.0) * 6.0);
    float hourAngle = static_cast<float>((st.wHour % 12 + st.wMinute / 60.0 + secVal / 3600.0) * 30.0);

    // 針の寸法
    float hourLen = dialR * 0.52f;
    float hourWidth = dialR * 0.065f;
    float hourTail = dialR * 0.12f;

    float minLen = dialR * 0.78f;
    float minWidth = dialR * 0.045f;
    float minTail = dialR * 0.14f;

    float secLen = dialR * 0.84f;
    float secTail = dialR * 0.22f;

    // シャドウ設定
    float shadowDist = radius * 0.02f;
    Color shadowColor(80, 0, 0, 0);
    SolidBrush shadowBrush(shadowColor);
    Pen shadowSecPen(shadowColor, radius * 0.016f);

    // 1. 時針（影 → 本体）
    GraphicsPath hourPath;
    PointF hourPts[] = {
        PointF(-hourWidth * 0.5f, hourTail),
        PointF(hourWidth * 0.5f, hourTail),
        PointF(hourWidth * 0.35f, -hourLen * 0.75f),
        PointF(0.0f, -hourLen),
        PointF(-hourWidth * 0.35f, -hourLen * 0.75f)
    };
    hourPath.AddPolygon(hourPts, 5);

    // 時針の影
    g.TranslateTransform(cx + shadowDist, cy + shadowDist);
    g.RotateTransform(hourAngle);
    g.FillPath(&shadowBrush, &hourPath);
    g.ResetTransform();

    // 時針の本体
    g.TranslateTransform(cx, cy);
    g.RotateTransform(hourAngle);
    SolidBrush hourBrush(colors.hourHand);
    g.FillPath(&hourBrush, &hourPath);
    Pen hourEdgePen(Color(100, 0, 0, 0), 1.0f);
    g.DrawPath(&hourEdgePen, &hourPath);
    g.ResetTransform();

    // 2. 分針（影 → 本体）
    GraphicsPath minPath;
    PointF minPts[] = {
        PointF(-minWidth * 0.5f, minTail),
        PointF(minWidth * 0.5f, minTail),
        PointF(minWidth * 0.3f, -minLen * 0.85f),
        PointF(0.0f, -minLen),
        PointF(-minWidth * 0.3f, -minLen * 0.85f)
    };
    minPath.AddPolygon(minPts, 5);

    // 分針の影
    g.TranslateTransform(cx + shadowDist * 1.5f, cy + shadowDist * 1.5f);
    g.RotateTransform(minAngle);
    g.FillPath(&shadowBrush, &minPath);
    g.ResetTransform();

    // 分針の本体
    g.TranslateTransform(cx, cy);
    g.RotateTransform(minAngle);
    SolidBrush minBrush(colors.minuteHand);
    g.FillPath(&minBrush, &minPath);
    Pen minEdgePen(Color(100, 0, 0, 0), 1.0f);
    g.DrawPath(&minEdgePen, &minPath);
    g.ResetTransform();

    // 3. 秒針（表示する場合）
    if (settings.showSeconds) {
        float secPenWidth = radius * 0.016f;
        if (secPenWidth < 1.5f) secPenWidth = 1.5f;

        // 秒針の影
        g.TranslateTransform(cx + shadowDist * 2.0f, cy + shadowDist * 2.0f);
        g.RotateTransform(secAngle);
        g.DrawLine(&shadowSecPen, 0.0f, secTail, 0.0f, -secLen);
        g.FillEllipse(&shadowBrush, -dialR * 0.035f, secTail * 0.5f, dialR * 0.07f, dialR * 0.07f);
        g.ResetTransform();

        // 秒針の本体
        g.TranslateTransform(cx, cy);
        g.RotateTransform(secAngle);
        Pen secPen(colors.secondHand, secPenWidth);
        secPen.SetStartCap(LineCapRound);
        secPen.SetEndCap(LineCapRound);
        g.DrawLine(&secPen, 0.0f, secTail, 0.0f, -secLen);

        // バランサー（尾部の小さな丸）
        SolidBrush secBrush(colors.secondHand);
        float balR = dialR * 0.035f;
        g.FillEllipse(&secBrush, -balR, secTail * 0.45f, balR * 2.0f, balR * 2.0f);
        g.ResetTransform();
    }

    // 4. センターキャップ（中央のリベット・軸留め）
    float capR = dialR * 0.05f;
    RectF capRect(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);
    SolidBrush capBrush(colors.centerCap);
    g.FillEllipse(&capBrush, capRect);

    Pen capBorder(Color(150, 0, 0, 0), 1.0f);
    g.DrawEllipse(&capBorder, capRect);

    // 小さなセンターピボット点
    float dotR = capR * 0.4f;
    RectF dotRect(cx - dotR, cy - dotR, dotR * 2.0f, dotR * 2.0f);
    SolidBrush dotBrush(colors.secondHand);
    g.FillEllipse(&dotBrush, dotRect);
}

void ClockRenderer::Render(
    HDC hdc,
    int width,
    int height,
    const SYSTEMTIME& st,
    double fractionalSeconds,
    const std::wstring& zoneLabel,
    const Settings& settings)
{
    using namespace Gdiplus;

    if (width <= 0 || height <= 0) return;

    // ダブルバッファリング用オフスクリーン Bitmap
    Bitmap memBitmap(width, height, PixelFormat32bppARGB);
    Graphics g(&memBitmap);

    // 最高画質のアンチエイリアス設定
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    g.SetInterpolationMode(InterpolationModeHighQualityBicubic);

    // 背景クリア（枠なしモードで四隅が透過または背景に馴染むように）
    // ウィンドウ自体を半透明にするか、背景をテーマに合わせた色でクリア
    ThemeColors colors = GetThemeColors(static_cast<ClockTheme>(settings.theme));

    // 背景色（文字盤周囲の余白部）
    // 枠なしウィンドウでデスクトップ上に自然に見えるよう、文字盤外側をソフトに処理
    Color bgClearColor = (settings.theme == static_cast<int>(ClockTheme::LightElegant))
        ? Color(255, 235, 238, 242)
        : Color(255, 18, 20, 24);
    g.Clear(bgClearColor);

    float cx = width / 2.0f;
    float cy = height / 2.0f;
    float radius = min(cx, cy) - 3.0f;

    if (radius > 10.0f) {
        // ベゼルと文字盤背景の描画
        DrawBezelAndDial(g, cx, cy, radius, colors);

        // 目盛りと数字の描画
        DrawTicksAndNumbers(g, cx, cy, radius, colors);

        // タイムゾーン・日付・デジタル時刻の描画
        DrawDialInfo(g, cx, cy, radius, st, zoneLabel, settings, colors);

        // 針（時・分・秒）の描画
        DrawHands(g, cx, cy, radius, st, fractionalSeconds, settings, colors);
    }

    // メモリBitmapを対象HDCへ転送
    Graphics screenGraphics(hdc);
    screenGraphics.DrawImage(&memBitmap, 0, 0, width, height);
}
