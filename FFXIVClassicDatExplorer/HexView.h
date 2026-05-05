#pragma once

#include <windows.h>
#undef ERROR

#include <cstdint>
#include <vector>

class HexView
{
public:
    HexView();
    ~HexView();

    bool Create(HWND parent, int x, int y, int w, int h);
    HWND GetHwnd() const { return m_hwnd; }

    void LoadData(const uint8_t* data, size_t size);
    void ClearData();
    const std::vector<uint8_t>& GetData() const { return m_data; }

    void Show();
    void Hide();
    void Resize(int w, int h);

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    void OnPaint();
    void OnSize(int w, int h);
    void OnVScroll(WPARAM wParam);
    void UpdateScrollInfo();

    HWND m_hwnd = nullptr;
    HWND m_parent = nullptr;
    HFONT m_font = nullptr;
    std::vector<uint8_t> m_data;

    int m_scrollPos = 0;
    int m_linesPerPage = 1;
    int m_totalLines = 0;
    int m_charWidth = 0;
    int m_charHeight = 0;

    static const wchar_t* ClassName();
    static bool s_registered;
};
