#pragma once
#include <windows.h>
#include <string>
#include "Settings.h"
#include "ClockRenderer.h"

class ClockWindow {
public:
    ClockWindow();
    ~ClockWindow();

    // ウィンドウクラスの登録と作成
    bool Create(HINSTANCE hInstance);

    // ウィンドウの表示
    void Show(int nCmdShow);

    // ウィンドウハンドル取得
    HWND GetHwnd() const { return m_hWnd; }

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    // イベントハンドラ
    void OnPaint();
    void OnTimer();
    void OnSizing(WPARAM edge, RECT* pRect);
    void OnSize(int width, int height);
    LRESULT OnNcHitTest(POINTS pt);
    void OnContextMenu(int screenX, int screenY);
    void OnCommand(WORD id);
    void OnMouseWheel(short delta);
    void OnDestroy();

    // ヘルパー
    void UpdateTimerInterval();
    void UpdateRegion();
    void ApplyWindowStyles();
    void ResizeWindowCenter(int newSize);
    void SetCurrentTimeZone(const std::wstring& keyName);

    HWND m_hWnd;
    HINSTANCE m_hInstance;
    Settings m_settings;
    ClockRenderer m_renderer;

    UINT_PTR m_timerId;
    bool m_isUpdatingRegion;
};
