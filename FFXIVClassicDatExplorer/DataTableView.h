#pragma once

#include <windows.h>

// Undefine Windows macros that conflict with project identifiers
#undef ERROR

#include <cstdint>
#include <vector>
#include <string>

class Sheet;

struct TableColumn
{
    std::wstring name;
    int width = 100;
    int type = 0;
};

class DataTableView
{
public:
    DataTableView();
    ~DataTableView();

    bool Create(HWND parent, int x, int y, int w, int h);
    HWND GetHwnd() const { return m_hwnd; }

    void LoadSheet(Sheet* sheet);

    void ClearData();
    void Show();
    void Hide();
    void Resize(int w, int h);

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    void OnPaint();
    void OnSize(int w, int h);
    void OnHScroll(WPARAM wParam);
    void OnVScroll(WPARAM wParam);
    void UpdateScrollInfo();

    HWND m_hwnd = nullptr;
    HWND m_parent = nullptr;
    HFONT m_font = nullptr;

    std::vector<TableColumn> m_columns;
    std::vector<std::vector<std::wstring>> m_cells;
    int m_rowCount = 0;
    int m_colCount = 0;

    int m_scrollX = 0;
    int m_scrollY = 0;
    int m_charWidth = 1;
    int m_rowHeight = 20;
    int m_headerHeight = 24;
    int m_colWidth = 100;

    static bool s_registered;
    static const wchar_t* ClassName();
};
