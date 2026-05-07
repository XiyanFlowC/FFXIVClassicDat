#pragma once

#include <windows.h>
#include <string>

class InputPrompt
{
public:
    static bool Show(HWND parent, const std::wstring& title,
                     const std::wstring& prompt,
                     std::wstring& outText,
                     const std::wstring& defaultText = L"");

private:
    static INT_PTR CALLBACK DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    struct Context
    {
        std::wstring prompt;
        std::wstring* outText = nullptr;
        std::wstring defaultText;
    };
};
