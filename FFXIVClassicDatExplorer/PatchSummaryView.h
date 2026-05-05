#pragma once

#include <windows.h>
#include <commctrl.h>
#undef ERROR

#include <vector>
#include <string>
#include <memory>

class ZiPatchFile;

class PatchSummaryView
{
public:
    PatchSummaryView();
    ~PatchSummaryView();

    bool Create(HWND parent, int x, int y, int w, int h);
    HWND GetHwnd() const { return m_hwnd; }

    void LoadPatch(const std::wstring& path);
    void Clear();

    void Show();
    void Hide();
    void Resize(int w, int h);

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    void OnSize(int w, int h);
    void OnCommand(int id);
    void PopulateList();
    void PopulateListAsync();
    void ApplyPatch();

    HWND m_hwnd = nullptr;
    HWND m_hwndInfo = nullptr;
    HWND m_hwndList = nullptr;
    HWND m_hwndApply = nullptr;

    std::wstring m_patchPath;
    std::unique_ptr<ZiPatchFile> m_patch;

    static bool s_registered;
    static const wchar_t* ClassName();
};
