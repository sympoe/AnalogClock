#pragma once
#include <windows.h>
#include <string>

class TimeZoneDialog {
public:
    // ダイアログを表示（モーダル）
    // 成功してOKが押された場合 true を返し、selectedKeyName に選択されたキー名が設定される
    // システムタイムゾーンが選択された場合は selectedKeyName は空文字列
    static bool Show(HWND hParent, std::wstring& inOutKeyName);
};
