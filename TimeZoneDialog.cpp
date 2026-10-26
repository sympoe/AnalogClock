#include "TimeZoneDialog.h"
#include "TimeZoneManager.h"
#include "resource.h"
#include <vector>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace {

struct DialogContext {
    std::wstring selectedKey;
    bool useSystemTz;
    std::vector<const TimeZoneInfoItem*> filteredList;
    HWND hDlg;
    HWND hSearch;
    HWND hList;
    HWND hPreview;
    HWND hCheckSystem;
};

// 小文字化ヘルパー
std::wstring ToLowerW(const std::wstring& str) {
    std::wstring result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::towlower);
    return result;
}

void UpdatePreviewText(DialogContext* ctx) {
    if (!ctx || !ctx->hPreview) return;

    SYSTEMTIME st = { 0 };
    double dummySec = 0.0;
    std::wstring label;

    std::wstring targetKey = ctx->useSystemTz ? L"" : ctx->selectedKey;
    TimeZoneManager::Instance().GetCurrentTimeInZone(targetKey, st, dummySec, label);

    std::wostringstream woss;
    woss << L"現地時刻プレビュー: "
         << st.wYear << L"/"
         << std::setw(2) << std::setfill(L'0') << st.wMonth << L"/"
         << std::setw(2) << std::setfill(L'0') << st.wDay << L" "
         << std::setw(2) << std::setfill(L'0') << st.wHour << L":"
         << std::setw(2) << std::setfill(L'0') << st.wMinute << L":"
         << std::setw(2) << std::setfill(L'0') << st.wSecond
         << L" (" << label << L")";

    SetWindowTextW(ctx->hPreview, woss.str().c_str());
}

void PopulateList(DialogContext* ctx) {
    SendMessage(ctx->hList, LB_RESETCONTENT, 0, 0);
    ctx->filteredList.clear();

    wchar_t searchBuf[256] = { 0 };
    GetWindowTextW(ctx->hSearch, searchBuf, 256);
    std::wstring searchWord = ToLowerW(searchBuf);

    const auto& allZones = TimeZoneManager::Instance().GetAllTimeZones();
    int selectIndex = -1;

    for (const auto& tz : allZones) {
        if (!searchWord.empty()) {
            std::wstring lowDisplay = ToLowerW(tz.displayName);
            std::wstring lowKey = ToLowerW(tz.keyName);
            std::wstring lowStd = ToLowerW(tz.standardName);
            if (lowDisplay.find(searchWord) == std::wstring::npos &&
                lowKey.find(searchWord) == std::wstring::npos &&
                lowStd.find(searchWord) == std::wstring::npos) {
                continue;
            }
        }

        ctx->filteredList.push_back(&tz);
        int itemIdx = (int)SendMessageW(ctx->hList, LB_ADDSTRING, 0, (LPARAM)tz.displayName.c_str());

        if (!ctx->useSystemTz && tz.keyName == ctx->selectedKey) {
            selectIndex = itemIdx;
        }
    }

    if (selectIndex >= 0) {
        SendMessage(ctx->hList, LB_SETCURSEL, selectIndex, 0);
    } else if (!ctx->filteredList.empty() && !ctx->useSystemTz) {
        SendMessage(ctx->hList, LB_SETCURSEL, 0, 0);
        ctx->selectedKey = ctx->filteredList[0]->keyName;
    }

    UpdatePreviewText(ctx);
}

INT_PTR CALLBACK DialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    DialogContext* ctx = reinterpret_cast<DialogContext*>(GetWindowLongPtr(hDlg, DWLP_USER));

    switch (msg) {
    case WM_INITDIALOG: {
        ctx = reinterpret_cast<DialogContext*>(lParam);
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)ctx);
        ctx->hDlg = hDlg;
        ctx->hSearch = GetDlgItem(hDlg, IDC_SEARCH_EDIT);
        ctx->hList = GetDlgItem(hDlg, IDC_TZ_LIST);
        ctx->hPreview = GetDlgItem(hDlg, IDC_PREVIEW_TEXT);
        ctx->hCheckSystem = GetDlgItem(hDlg, IDC_USE_SYSTEM_TZ);

        SendMessage(ctx->hCheckSystem, BM_SETCHECK, ctx->useSystemTz ? BST_CHECKED : BST_UNCHECKED, 0);
        EnableWindow(ctx->hList, ctx->useSystemTz ? FALSE : TRUE);
        EnableWindow(ctx->hSearch, ctx->useSystemTz ? FALSE : TRUE);

        PopulateList(ctx);

        // 1秒ごとのプレビュー更新タイマー
        SetTimer(hDlg, 1, 1000, NULL);
        return TRUE;
    }

    case WM_TIMER: {
        if (wParam == 1) {
            UpdatePreviewText(ctx);
        }
        return TRUE;
    }

    case WM_COMMAND: {
        WORD id = LOWORD(wParam);
        WORD code = HIWORD(wParam);

        if (id == IDC_USE_SYSTEM_TZ && code == BN_CLICKED) {
            LRESULT chk = SendMessage(ctx->hCheckSystem, BM_GETCHECK, 0, 0);
            ctx->useSystemTz = (chk == BST_CHECKED);
            EnableWindow(ctx->hList, ctx->useSystemTz ? FALSE : TRUE);
            EnableWindow(ctx->hSearch, ctx->useSystemTz ? FALSE : TRUE);
            UpdatePreviewText(ctx);
            return TRUE;
        }

        if (id == IDC_SEARCH_EDIT && code == EN_CHANGE) {
            PopulateList(ctx);
            return TRUE;
        }

        if (id == IDC_TZ_LIST && code == LBN_SELCHANGE) {
            int sel = (int)SendMessage(ctx->hList, LB_GETCURSEL, 0, 0);
            if (sel >= 0 && sel < (int)ctx->filteredList.size()) {
                ctx->selectedKey = ctx->filteredList[sel]->keyName;
                UpdatePreviewText(ctx);
            }
            return TRUE;
        }

        if (id == IDC_TZ_LIST && code == LBN_DBLCLK) {
            // ダブルクリックで即決定
            int sel = (int)SendMessage(ctx->hList, LB_GETCURSEL, 0, 0);
            if (sel >= 0 && sel < (int)ctx->filteredList.size()) {
                ctx->selectedKey = ctx->filteredList[sel]->keyName;
                ctx->useSystemTz = false;
                KillTimer(hDlg, 1);
                EndDialog(hDlg, IDOK);
            }
            return TRUE;
        }

        if (id == IDOK) {
            KillTimer(hDlg, 1);
            EndDialog(hDlg, IDOK);
            return TRUE;
        }

        if (id == IDCANCEL) {
            KillTimer(hDlg, 1);
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        break;
    }

    case WM_DESTROY: {
        KillTimer(hDlg, 1);
        break;
    }
    }

    return FALSE;
}

} // namespace

bool TimeZoneDialog::Show(HWND hParent, std::wstring& inOutKeyName) {
    DialogContext ctx;
    ctx.useSystemTz = inOutKeyName.empty();
    ctx.selectedKey = inOutKeyName;

    HINSTANCE hInst = GetModuleHandle(NULL);
    INT_PTR ret = DialogBoxParamW(
        hInst,
        MAKEINTRESOURCEW(IDD_TIMEZONE_DIALOG),
        hParent,
        DialogProc,
        (LPARAM)&ctx
    );

    if (ret == IDOK) {
        if (ctx.useSystemTz) {
            inOutKeyName = L"";
        } else {
            inOutKeyName = ctx.selectedKey;
        }
        return true;
    }

    return false;
}
