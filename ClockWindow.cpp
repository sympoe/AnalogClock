#include "ClockWindow.h"
#include "TimeZoneManager.h"
#include "TimeZoneDialog.h"
#include "resource.h"
#include <windowsx.h>
#include <algorithm>

namespace {
const wchar_t* const WND_CLASS_NAME = L"AnalogClockWindowClass";
const UINT_PTR CLOCK_TIMER_ID = 1001;
const int RESIZE_BORDER_THICKNESS = 8;
}

ClockWindow::ClockWindow()
    : m_hWnd(NULL)
    , m_hInstance(NULL)
    , m_timerId(0)
    , m_isUpdatingRegion(false)
{
}

ClockWindow::~ClockWindow() {
}

bool ClockWindow::Create(HINSTANCE hInstance) {
    m_hInstance = hInstance;

    // 前回の設定を読み込み
    m_settings.Load();

    // ウィンドウクラスの登録
    WNDCLASSEXW wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcex.lpfnWndProc = ClockWindow::WndProc;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON));
    wcex.hIconSm = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON));
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = NULL; // ちらつき防止のため背景ブラシなし
    wcex.lpszClassName = WND_CLASS_NAME;

    ATOM atom = RegisterClassExW(&wcex);
    if (!atom && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        DWORD err = GetLastError();
        wchar_t buf[128];
        wsprintfW(buf, L"RegisterClassEx failed: %u", err);
        OutputDebugStringW(buf);
    }

    DWORD style = 0;
    DWORD exStyle = 0;

    if (m_settings.hasWindowBorder) {
        style = WS_OVERLAPPEDWINDOW;
        exStyle = WS_EX_APPWINDOW;
    } else {
        style = WS_POPUP | WS_VISIBLE;
        exStyle = WS_EX_APPWINDOW;
    }

    if (m_settings.alwaysOnTop) {
        exStyle |= WS_EX_TOPMOST;
    }

    m_hWnd = CreateWindowExW(
        exStyle,
        WND_CLASS_NAME,
        L"アナログ時計",
        style,
        m_settings.windowX,
        m_settings.windowY,
        m_settings.windowWidth,
        m_settings.windowHeight,
        NULL,
        NULL,
        hInstance,
        this
    );

    if (!m_hWnd) {
        DWORD err = GetLastError();
        wchar_t buf[128];
        wsprintfW(buf, L"CreateWindowEx failed: %u", err);
        OutputDebugStringW(buf);
        return false;
    }

    // タイマー設定
    UpdateTimerInterval();

    // 形状の反映（円形リージョンなど）
    UpdateRegion();

    return true;
}

void ClockWindow::Show(int nCmdShow) {
    if (m_hWnd) {
        ShowWindow(m_hWnd, nCmdShow);
        UpdateWindow(m_hWnd);
    }
}

LRESULT CALLBACK ClockWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    ClockWindow* pThis = nullptr;
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
        pThis = reinterpret_cast<ClockWindow*>(pCreate->lpCreateParams);
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        pThis->m_hWnd = hWnd;
    } else {
        pThis = reinterpret_cast<ClockWindow*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    }

    if (pThis) {
        return pThis->HandleMessage(msg, wParam, lParam);
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

LRESULT ClockWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT:
        OnPaint();
        return 0;

    case WM_ERASEBKGND:
        // ちらつき防止のため背景消去は行わず、OnPaint でダブルバッファ描画
        return 1;

    case WM_TIMER:
        if (wParam == CLOCK_TIMER_ID) {
            OnTimer();
            return 0;
        }
        break;

    case WM_SIZING:
        OnSizing(wParam, reinterpret_cast<RECT*>(lParam));
        return TRUE;

    case WM_SIZE:
        OnSize(LOWORD(lParam), HIWORD(lParam));
        return 0;

    case WM_NCHITTEST: {
        POINTS pts = MAKEPOINTS(lParam);
        return OnNcHitTest(pts);
    }

    case WM_NCRBUTTONUP: {
        // HTCAPTION領域で右クリックが離された場合
        POINTS pts = MAKEPOINTS(lParam);
        OnContextMenu(pts.x, pts.y);
        return 0;
    }

    case WM_CONTEXTMENU: {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        if (x == -1 && y == -1) {
            // キーボード操作等によるコンテキストメニュー要求
            RECT rc;
            GetWindowRect(m_hWnd, &rc);
            x = rc.left + (rc.right - rc.left) / 2;
            y = rc.top + (rc.bottom - rc.top) / 2;
        }
        OnContextMenu(x, y);
        return 0;
    }

    case WM_COMMAND:
        OnCommand(LOWORD(wParam));
        return 0;

    case WM_MOUSEWHEEL:
        OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
        return 0;

    case WM_DESTROY:
        OnDestroy();
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(m_hWnd, msg, wParam, lParam);
}

void ClockWindow::OnPaint() {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(m_hWnd, &ps);

    RECT clientRect;
    GetClientRect(m_hWnd, &clientRect);
    int w = clientRect.right - clientRect.left;
    int h = clientRect.bottom - clientRect.top;

    SYSTEMTIME st = { 0 };
    double fractionalSec = 0.0;
    std::wstring zoneLabel;

    TimeZoneManager::Instance().GetCurrentTimeInZone(
        m_settings.timeZoneKeyName,
        st,
        fractionalSec,
        zoneLabel
    );

    if (!m_settings.timeZoneCustomName.empty()) {
        zoneLabel = m_settings.timeZoneCustomName;
    }

    m_renderer.Render(hdc, w, h, st, fractionalSec, zoneLabel, m_settings);

    EndPaint(m_hWnd, &ps);
}

void ClockWindow::OnTimer() {
    // 画面の更新（再描画トリガー）
    InvalidateRect(m_hWnd, NULL, FALSE);
}

void ClockWindow::OnSizing(WPARAM edge, RECT* pRect) {
    if (!pRect) return;

    int width = pRect->right - pRect->left;
    int height = pRect->bottom - pRect->top;

    // 縦横比 1:1 に固定
    int size = (std::max)(width, height);
    if (size < 120) size = 120;
    if (size > 1200) size = 1200;

    switch (edge) {
    case WMSZ_LEFT:
    case WMSZ_BOTTOMLEFT:
        pRect->left = pRect->right - size;
        pRect->bottom = pRect->top + size;
        break;

    case WMSZ_TOP:
    case WMSZ_TOPRIGHT:
        pRect->top = pRect->bottom - size;
        pRect->right = pRect->left + size;
        break;

    case WMSZ_TOPLEFT:
        pRect->left = pRect->right - size;
        pRect->top = pRect->bottom - size;
        break;

    case WMSZ_RIGHT:
    case WMSZ_BOTTOM:
    case WMSZ_BOTTOMRIGHT:
    default:
        pRect->right = pRect->left + size;
        pRect->bottom = pRect->top + size;
        break;
    }
}

void ClockWindow::OnSize(int width, int height) {
    if (m_isUpdatingRegion) return;
    UpdateRegion();
    InvalidateRect(m_hWnd, NULL, FALSE);
}

LRESULT ClockWindow::OnNcHitTest(POINTS pt) {
    if (m_settings.hasWindowBorder) {
        // 枠ありモードは通常挙動
        return DefWindowProcW(m_hWnd, WM_NCHITTEST, 0, MAKELPARAM(pt.x, pt.y));
    }

    // 枠なしモード時のリサイズ＆ドラッグ判定
    RECT rc;
    GetWindowRect(m_hWnd, &rc);

    int x = pt.x - rc.left;
    int y = pt.y - rc.top;
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;

    // 外周リサイズボーダーの判定
    bool left   = (x < RESIZE_BORDER_THICKNESS);
    bool right  = (x >= w - RESIZE_BORDER_THICKNESS);
    bool top    = (y < RESIZE_BORDER_THICKNESS);
    bool bottom = (y >= h - RESIZE_BORDER_THICKNESS);

    if (top && left)     return HTTOPLEFT;
    if (top && right)    return HTTOPRIGHT;
    if (bottom && left)  return HTBOTTOMLEFT;
    if (bottom && right) return HTBOTTOMRIGHT;
    if (left)            return HTLEFT;
    if (right)           return HTRIGHT;
    if (top)             return HTTOP;
    if (bottom)          return HTBOTTOM;

    // 内部はすべてドラッグ移動可能（HTCAPTION）にする
    return HTCAPTION;
}

void ClockWindow::OnMouseWheel(short delta) {
    // マウスホイールで時計の拡大・縮小
    int step = (delta > 0) ? 20 : -20;

    RECT rc;
    GetWindowRect(m_hWnd, &rc);
    int curW = rc.right - rc.left;
    int newSize = curW + step;

    if (newSize >= 120 && newSize <= 1000) {
        ResizeWindowCenter(newSize);
    }
}

void ClockWindow::ResizeWindowCenter(int newSize) {
    RECT rc;
    GetWindowRect(m_hWnd, &rc);
    int curW = rc.right - rc.left;
    int curH = rc.bottom - rc.top;

    int cx = rc.left + curW / 2;
    int cy = rc.top + curH / 2;

    int newX = cx - newSize / 2;
    int newY = cy - newSize / 2;

    SetWindowPos(m_hWnd, NULL, newX, newY, newSize, newSize, SWP_NOZORDER | SWP_NOACTIVATE);
}

void ClockWindow::UpdateRegion() {
    if (m_settings.hasWindowBorder) {
        // 枠ありモードはリージョン解除（四角形通常ウィンドウ）
        SetWindowRgn(m_hWnd, NULL, TRUE);
    } else {
        // 枠なしモードは美しい完全な円形ウィンドウにする
        RECT rc;
        GetWindowRect(m_hWnd, &rc);
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;

        m_isUpdatingRegion = true;
        HRGN hRgn = CreateEllipticRgn(0, 0, w + 1, h + 1);
        SetWindowRgn(m_hWnd, hRgn, TRUE);
        m_isUpdatingRegion = false;
    }
}

void ClockWindow::UpdateTimerInterval() {
    if (m_timerId != 0) {
        KillTimer(m_hWnd, m_timerId);
        m_timerId = 0;
    }

    // スムーズ秒針なら約60fps (16ms)、ステップなら 200ms
    UINT interval = m_settings.smoothSeconds ? 16 : 200;
    m_timerId = SetTimer(m_hWnd, CLOCK_TIMER_ID, interval, NULL);
}

void ClockWindow::SetCurrentTimeZone(const std::wstring& keyName) {
    m_settings.timeZoneKeyName = keyName;
    m_settings.timeZoneCustomName = L"";
    m_settings.Save();
    InvalidateRect(m_hWnd, NULL, FALSE);
}

void ClockWindow::OnContextMenu(int screenX, int screenY) {
    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return;

    // 1. タイムゾーンサブメニュー
    HMENU hTzSub = CreatePopupMenu();
    AppendMenuW(hTzSub, MF_STRING, ID_MENU_TZ_SYSTEM, L"システムのタイムゾーン（自動追従）");
    AppendMenuW(hTzSub, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hTzSub, MF_STRING, ID_MENU_TZ_TOKYO, L"東京 (JST, UTC+9)");
    AppendMenuW(hTzSub, MF_STRING, ID_MENU_TZ_UTC, L"世界協定時 (UTC)");
    AppendMenuW(hTzSub, MF_STRING, ID_MENU_TZ_LONDON, L"ロンドン (GMT/BST)");
    AppendMenuW(hTzSub, MF_STRING, ID_MENU_TZ_PARIS, L"パリ (CET/CEST, UTC+1)");
    AppendMenuW(hTzSub, MF_STRING, ID_MENU_TZ_NEWYORK, L"ニューヨーク (EST/EDT, UTC-5)");
    AppendMenuW(hTzSub, MF_STRING, ID_MENU_TZ_LOSANGELES, L"ロサンゼルス (PST/PDT, UTC-8)");
    AppendMenuW(hTzSub, MF_STRING, ID_MENU_TZ_SYDNEY, L"シドニー (AEST/AEDT, UTC+10)");
    AppendMenuW(hTzSub, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hTzSub, MF_STRING, ID_MENU_TZ_MORE, L"その他のタイムゾーンを選択...");

    // チェック状態反映
    UINT checkedTz = ID_MENU_TZ_MORE;
    if (m_settings.timeZoneKeyName.empty()) checkedTz = ID_MENU_TZ_SYSTEM;
    else if (m_settings.timeZoneKeyName == L"Tokyo Standard Time") checkedTz = ID_MENU_TZ_TOKYO;
    else if (m_settings.timeZoneKeyName == L"UTC") checkedTz = ID_MENU_TZ_UTC;
    else if (m_settings.timeZoneKeyName == L"GMT Standard Time") checkedTz = ID_MENU_TZ_LONDON;
    else if (m_settings.timeZoneKeyName == L"Romance Standard Time" || m_settings.timeZoneKeyName == L"W. Europe Standard Time") checkedTz = ID_MENU_TZ_PARIS;
    else if (m_settings.timeZoneKeyName == L"Eastern Standard Time") checkedTz = ID_MENU_TZ_NEWYORK;
    else if (m_settings.timeZoneKeyName == L"Pacific Standard Time") checkedTz = ID_MENU_TZ_LOSANGELES;
    else if (m_settings.timeZoneKeyName == L"AUS Eastern Standard Time") checkedTz = ID_MENU_TZ_SYDNEY;

    CheckMenuRadioItem(hTzSub, ID_MENU_TZ_SYSTEM, ID_MENU_TZ_MORE, checkedTz, MF_BYCOMMAND);
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hTzSub, L"タイムゾーン (&T)");

    // 2. テーマサブメニュー
    HMENU hThemeSub = CreatePopupMenu();
    AppendMenuW(hThemeSub, MF_STRING, ID_MENU_THEME_DARK, L"ダークモダン (Dark Modern)");
    AppendMenuW(hThemeSub, MF_STRING, ID_MENU_THEME_LIGHT, L"ライトエレガント (Light Elegant)");
    AppendMenuW(hThemeSub, MF_STRING, ID_MENU_THEME_GOLD, L"クラシックゴールド (Classic Gold)");
    AppendMenuW(hThemeSub, MF_STRING, ID_MENU_THEME_CYBER, L"サイバーシアン (Cyber Cyan)");

    UINT checkedTheme = ID_MENU_THEME_DARK + m_settings.theme;
    CheckMenuRadioItem(hThemeSub, ID_MENU_THEME_DARK, ID_MENU_THEME_CYBER, checkedTheme, MF_BYCOMMAND);
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hThemeSub, L"テーマ (&C)");

    // 3. サイズ変更サブメニュー
    HMENU hSizeSub = CreatePopupMenu();
    AppendMenuW(hSizeSub, MF_STRING, ID_MENU_SIZE_SMALL, L"小 (160 × 160)");
    AppendMenuW(hSizeSub, MF_STRING, ID_MENU_SIZE_MEDIUM, L"中 (240 × 240)");
    AppendMenuW(hSizeSub, MF_STRING, ID_MENU_SIZE_LARGE, L"大 (360 × 360)");
    AppendMenuW(hSizeSub, MF_STRING, ID_MENU_SIZE_XLARGE, L"特大 (480 × 480)");
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hSizeSub, L"サイズプリセット (&S)");

    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

    // 4. トグルオプション
    AppendMenuW(hMenu, MF_STRING | (m_settings.smoothSeconds ? MF_CHECKED : MF_UNCHECKED),
        ID_MENU_TOGGLE_SMOOTH, L"スムーズ秒針（スイープ運針）");
    AppendMenuW(hMenu, MF_STRING | (m_settings.showSeconds ? MF_CHECKED : MF_UNCHECKED),
        ID_MENU_TOGGLE_SECONDS, L"秒針を表示");
    AppendMenuW(hMenu, MF_STRING | (m_settings.showDate ? MF_CHECKED : MF_UNCHECKED),
        ID_MENU_TOGGLE_DATE, L"日付を表示");
    AppendMenuW(hMenu, MF_STRING | (m_settings.showDigital ? MF_CHECKED : MF_UNCHECKED),
        ID_MENU_TOGGLE_DIGITAL, L"デジタル時刻を表示");
    AppendMenuW(hMenu, MF_STRING | (m_settings.alwaysOnTop ? MF_CHECKED : MF_UNCHECKED),
        ID_MENU_TOGGLE_ONTOP, L"常に最前面に表示");
    AppendMenuW(hMenu, MF_STRING | (m_settings.hasWindowBorder ? MF_CHECKED : MF_UNCHECKED),
        ID_MENU_TOGGLE_BORDER, L"ウィンドウ枠を表示");

    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

    AppendMenuW(hMenu, MF_STRING, ID_MENU_RESET_POS, L"位置とサイズを初期化");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, ID_MENU_EXIT, L"終了 (&X)");

    SetForegroundWindow(m_hWnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, screenX, screenY, 0, m_hWnd, NULL);
    DestroyMenu(hMenu);
}

void ClockWindow::ApplyWindowStyles() {
    DWORD style = 0;
    DWORD exStyle = 0;

    if (m_settings.hasWindowBorder) {
        style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
        exStyle = WS_EX_APPWINDOW;
    } else {
        style = WS_POPUP | WS_VISIBLE;
        exStyle = WS_EX_TOOLWINDOW;
    }

    if (m_settings.alwaysOnTop) {
        exStyle |= WS_EX_TOPMOST;
    }

    SetWindowLongW(m_hWnd, GWL_STYLE, style);
    SetWindowLongW(m_hWnd, GWL_EXSTYLE, exStyle);

    UpdateRegion();

    HWND hWndInsertAfter = m_settings.alwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST;
    SetWindowPos(m_hWnd, hWndInsertAfter, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);

    InvalidateRect(m_hWnd, NULL, TRUE);
}

void ClockWindow::OnCommand(WORD id) {
    switch (id) {
    case ID_MENU_TZ_SYSTEM:
        SetCurrentTimeZone(L"");
        break;
    case ID_MENU_TZ_TOKYO:
        SetCurrentTimeZone(L"Tokyo Standard Time");
        break;
    case ID_MENU_TZ_UTC:
        SetCurrentTimeZone(L"UTC");
        break;
    case ID_MENU_TZ_LONDON:
        SetCurrentTimeZone(L"GMT Standard Time");
        break;
    case ID_MENU_TZ_PARIS:
        SetCurrentTimeZone(L"Romance Standard Time");
        break;
    case ID_MENU_TZ_NEWYORK:
        SetCurrentTimeZone(L"Eastern Standard Time");
        break;
    case ID_MENU_TZ_LOSANGELES:
        SetCurrentTimeZone(L"Pacific Standard Time");
        break;
    case ID_MENU_TZ_SYDNEY:
        SetCurrentTimeZone(L"AUS Eastern Standard Time");
        break;

    case ID_MENU_TZ_MORE: {
        std::wstring selectedKey = m_settings.timeZoneKeyName;
        if (TimeZoneDialog::Show(m_hWnd, selectedKey)) {
            SetCurrentTimeZone(selectedKey);
        }
        break;
    }

    case ID_MENU_THEME_DARK:
    case ID_MENU_THEME_LIGHT:
    case ID_MENU_THEME_GOLD:
    case ID_MENU_THEME_CYBER:
        m_settings.theme = id - ID_MENU_THEME_DARK;
        m_settings.Save();
        InvalidateRect(m_hWnd, NULL, TRUE);
        break;

    case ID_MENU_SIZE_SMALL:
        ResizeWindowCenter(160);
        break;
    case ID_MENU_SIZE_MEDIUM:
        ResizeWindowCenter(240);
        break;
    case ID_MENU_SIZE_LARGE:
        ResizeWindowCenter(360);
        break;
    case ID_MENU_SIZE_XLARGE:
        ResizeWindowCenter(480);
        break;

    case ID_MENU_TOGGLE_SMOOTH:
        m_settings.smoothSeconds = !m_settings.smoothSeconds;
        m_settings.Save();
        UpdateTimerInterval();
        InvalidateRect(m_hWnd, NULL, FALSE);
        break;

    case ID_MENU_TOGGLE_SECONDS:
        m_settings.showSeconds = !m_settings.showSeconds;
        m_settings.Save();
        InvalidateRect(m_hWnd, NULL, FALSE);
        break;

    case ID_MENU_TOGGLE_DATE:
        m_settings.showDate = !m_settings.showDate;
        m_settings.Save();
        InvalidateRect(m_hWnd, NULL, FALSE);
        break;

    case ID_MENU_TOGGLE_DIGITAL:
        m_settings.showDigital = !m_settings.showDigital;
        m_settings.Save();
        InvalidateRect(m_hWnd, NULL, FALSE);
        break;

    case ID_MENU_TOGGLE_ONTOP:
        m_settings.alwaysOnTop = !m_settings.alwaysOnTop;
        m_settings.Save();
        ApplyWindowStyles();
        break;

    case ID_MENU_TOGGLE_BORDER:
        m_settings.hasWindowBorder = !m_settings.hasWindowBorder;
        m_settings.Save();
        ApplyWindowStyles();
        break;

    case ID_MENU_RESET_POS: {
        RECT workArea;
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
        int defSize = 260;
        int defX = workArea.left + 80;
        int defY = workArea.top + 80;
        SetWindowPos(m_hWnd, NULL, defX, defY, defSize, defSize, SWP_NOZORDER);
        break;
    }

    case ID_MENU_EXIT:
        DestroyWindow(m_hWnd);
        break;
    }
}

void ClockWindow::OnDestroy() {
    // 終了時に現在のウィンドウ位置とサイズを保存！
    RECT rc;
    GetWindowRect(m_hWnd, &rc);
    m_settings.windowX = rc.left;
    m_settings.windowY = rc.top;
    m_settings.windowWidth = rc.right - rc.left;
    m_settings.windowHeight = rc.bottom - rc.top;
    m_settings.Save();

    if (m_timerId != 0) {
        KillTimer(m_hWnd, m_timerId);
        m_timerId = 0;
    }
}
