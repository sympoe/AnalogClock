#include <windows.h>
#include <commctrl.h>
#include "ClockWindow.h"
#include "TimeZoneManager.h"
#include "ClockRenderer.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "advapi32.lib")

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // コモンコントロール初期化（ダイアログ等のビジュアルスタイル用）
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);

    // GDI+ 初期化
    ClockRenderer::InitializeGDIPlus();

    // タイムゾーンマネージャ初期化
    TimeZoneManager::Instance().Initialize();

    // 時計ウィンドウ作成と表示
    ClockWindow clockWin;
    if (!clockWin.Create(hInstance)) {
        ClockRenderer::ShutdownGDIPlus();
        return 0;
    }

    clockWin.Show(nCmdShow);

    // メッセージループ
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // GDI+ 終了処理
    ClockRenderer::ShutdownGDIPlus();

    return static_cast<int>(msg.wParam);
}
