#include "InputPrompt.h"
#include <commctrl.h>

INT_PTR CALLBACK InputPrompt::DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static Context* ctx = nullptr;

    switch (msg)
    {
    case WM_INITDIALOG:
        ctx = reinterpret_cast<Context*>(lParam);
        SetWindowTextW(hwnd, L"Input");
        if (ctx)
        {
            CreateWindowExW(0, WC_STATICW, ctx->prompt.c_str(),
                WS_CHILD | WS_VISIBLE, 12, 10, 320, 20,
                hwnd, nullptr, GetModuleHandle(nullptr), nullptr);

            HWND hEdit = CreateWindowExW(0, WC_EDITW, ctx->defaultText.c_str(),
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                12, 32, 320, 22, hwnd, (HMENU)1000,
                GetModuleHandle(nullptr), nullptr);
            SetFocus(hEdit);
            SendMessageW(hEdit, EM_SETSEL, 0, -1);
        }

        CreateWindowExW(0, WC_BUTTONW, L"OK",
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            180, 66, 80, 24, hwnd, (HMENU)IDOK,
            GetModuleHandle(nullptr), nullptr);

        CreateWindowExW(0, WC_BUTTONW, L"Cancel",
            WS_CHILD | WS_VISIBLE,
            268, 66, 80, 24, hwnd, (HMENU)IDCANCEL,
            GetModuleHandle(nullptr), nullptr);
        return TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            if (ctx && ctx->outText)
            {
                HWND hEdit = GetDlgItem(hwnd, 1000);
                if (hEdit)
                {
                    wchar_t buf[1024];
                    GetWindowTextW(hEdit, buf, 1024);
                    *ctx->outText = buf;
                }
            }
            EndDialog(hwnd, IDOK);
            return TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hwnd, IDCANCEL);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

bool InputPrompt::Show(HWND parent, const std::wstring& title,
                       const std::wstring& prompt,
                       std::wstring& outText,
                       const std::wstring& defaultText)
{
    Context ctx;
    ctx.prompt = prompt;
    ctx.outText = &outText;
    ctx.defaultText = defaultText;

    // Build dialog template in memory
    struct
    {
        DLGTEMPLATE tmpl;
        WORD menu;
        WORD classId;
        WORD title[32];
    } dlg = {};

    dlg.tmpl.style = DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU;
    dlg.tmpl.dwExtendedStyle = 0;
    dlg.tmpl.cdit = 0;
    dlg.tmpl.x = 0;
    dlg.tmpl.y = 0;
    dlg.tmpl.cx = 350;
    dlg.tmpl.cy = 100;

    if (title.size() < 31)
    {
        wcscpy_s(reinterpret_cast<wchar_t*>(dlg.title), 32, title.c_str());
    }
    else
    {
        wcscpy_s(reinterpret_cast<wchar_t*>(dlg.title), 32, L"Input");
    }

    INT_PTR result = DialogBoxIndirectParamW(GetModuleHandle(nullptr),
        &dlg.tmpl, parent, DlgProc, reinterpret_cast<LPARAM>(&ctx));

    return result == IDOK;
}
