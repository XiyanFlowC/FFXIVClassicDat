#include "DataTableView.h"

#include <algorithm>

// Windows defines ERROR as a macro which conflicts with XmlNode
#undef ERROR
#include "../FFXIVClassicDat/Sheet.h"

bool DataTableView::s_registered = false;

const wchar_t* DataTableView::ClassName()
{
    return L"FFXIVClassicDatExplorer_DataTableView";
}

DataTableView::DataTableView() = default;

DataTableView::~DataTableView()
{
    if (m_font)
        DeleteObject(m_font);
}

bool DataTableView::Create(HWND parent, int x, int y, int w, int h)
{
    m_parent = parent;

    if (!s_registered)
    {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = ClassName();
        RegisterClassExW(&wc);
        s_registered = true;
    }

    m_hwnd = CreateWindowExW(0, ClassName(), L"DataTable",
        WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL,
        x, y, w, h, parent, nullptr, GetModuleHandle(nullptr), this);

    return m_hwnd != nullptr;
}

void DataTableView::LoadSheet(Sheet* sheet)
{
    if (!sheet) return;
    ClearData();

    sheet->LoadAll();

    auto& schema = sheet->GetSchema();
    const auto& defs = schema.GetSchemaDefinition();

    m_colCount = static_cast<int>(defs.size());
    m_columns.resize(m_colCount);

    int idx = 0;
    for (auto type : defs)
    {
        m_columns[idx].type = type;
        m_columns[idx].width = m_colWidth;

        wchar_t buf[32];
        swprintf_s(buf, L"Col %d", idx);
        m_columns[idx].name = buf;
        ++idx;
    }

    m_rowCount = 0;
    while (true)
    {
        try
        {
            auto& row = sheet->GetRow(m_rowCount);
            m_rowCount++;
        }
        catch (...)
        {
            break;
        }
    }

    m_cells.resize(m_rowCount);
    for (int r = 0; r < m_rowCount; ++r)
    {
        m_cells[r].resize(m_colCount);
        try
        {
            auto& row = sheet->GetRow(r);
            for (int c = 0; c < m_colCount; ++c)
            {
                try
                {
                    auto& cell = row[c];
                    std::string str = cell.ToString();
                    m_cells[r][c] = std::wstring(str.begin(), str.end());
                }
                catch (...)
                {
                    m_cells[r][c] = L"";
                }
            }
        }
        catch (...)
        {
            for (int c = 0; c < m_colCount; ++c)
                m_cells[r][c] = L"?";
        }
    }

    UpdateScrollInfo();
    if (m_hwnd)
        InvalidateRect(m_hwnd, nullptr, TRUE);
}

void DataTableView::ClearData()
{
    m_columns.clear();
    m_cells.clear();
    m_rowCount = 0;
    m_colCount = 0;
    m_scrollX = 0;
    m_scrollY = 0;
    if (m_hwnd)
        InvalidateRect(m_hwnd, nullptr, TRUE);
}

void DataTableView::Show()
{
    if (m_hwnd) ShowWindow(m_hwnd, SW_SHOW);
}

void DataTableView::Hide()
{
    if (m_hwnd) ShowWindow(m_hwnd, SW_HIDE);
}

void DataTableView::Resize(int w, int h)
{
    if (m_hwnd)
    {
        SetWindowPos(m_hwnd, nullptr, 0, 0, w, h, SWP_NOZORDER | SWP_NOMOVE);
        UpdateScrollInfo();
    }
}

LRESULT CALLBACK DataTableView::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    DataTableView* view = nullptr;

    if (msg == WM_NCCREATE)
    {
        auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        view = static_cast<DataTableView*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(view));
        view->m_hwnd = hwnd;
    }
    else
    {
        view = reinterpret_cast<DataTableView*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (view)
        return view->HandleMessage(msg, wParam, lParam);

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT DataTableView::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        NONCLIENTMETRICS ncm = {};
        ncm.cbSize = sizeof(ncm);
        SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
        m_font = CreateFontIndirectW(&ncm.lfMessageFont);

        HDC hdc = GetDC(m_hwnd);
        if (m_font) SelectObject(hdc, m_font);
        TEXTMETRIC tm;
        GetTextMetrics(hdc, &tm);
        m_charWidth = tm.tmAveCharWidth;
        m_rowHeight = tm.tmHeight + tm.tmExternalLeading + 4;
        ReleaseDC(m_hwnd, hdc);
    }
    break;

    case WM_PAINT:
        OnPaint();
        break;

    case WM_SIZE:
        OnSize(LOWORD(lParam), HIWORD(lParam));
        break;

    case WM_HSCROLL:
        OnHScroll(wParam);
        break;

    case WM_VSCROLL:
        OnVScroll(wParam);
        break;

    case WM_MOUSEWHEEL:
    {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
        m_scrollY = (std::max)(0, m_scrollY - delta);
        UpdateScrollInfo();
        InvalidateRect(m_hwnd, nullptr, TRUE);
    }
    break;

    case WM_DESTROY:
        if (m_font)
        {
            DeleteObject(m_font);
            m_font = nullptr;
        }
        break;

    default:
        return DefWindowProc(m_hwnd, msg, wParam, lParam);
    }
    return 0;
}

void DataTableView::OnPaint()
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(m_hwnd, &ps);
    if (!hdc) return;

    RECT client;
    GetClientRect(m_hwnd, &client);

    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBmp = CreateCompatibleBitmap(hdc, client.right, client.bottom);
    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

    HBRUSH bgBrush = (HBRUSH)(COLOR_WINDOW + 1);
    FillRect(memDC, &client, bgBrush);

    if (m_font)
        SelectObject(memDC, m_font);

    SetBkMode(memDC, OPAQUE);

    int colCount = (std::max)(0, m_colCount);
    int headerHeight = m_headerHeight;

    HPEN gridPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DLIGHT));
    HPEN oldPen = (HPEN)SelectObject(memDC, gridPen);

    HBRUSH hdrBrush = GetSysColorBrush(COLOR_BTNFACE);
    SetTextColor(memDC, GetSysColor(COLOR_BTNTEXT));

    for (int c = 0; c < colCount; ++c)
    {
        int x = c * m_colWidth - m_scrollX;
        if (x + m_colWidth < 0) continue;
        if (x > client.right) break;

        RECT rc = { x, 0, x + m_colWidth, headerHeight };
        FillRect(memDC, &rc, hdrBrush);
        MoveToEx(memDC, x, 0, nullptr);
        LineTo(memDC, x, headerHeight);
        MoveToEx(memDC, x + m_colWidth - 1, 0, nullptr);
        LineTo(memDC, x + m_colWidth - 1, headerHeight);

        if (!m_columns.empty() && c < static_cast<int>(m_columns.size()))
        {
            RECT textRc = { x + 4, 2, x + m_colWidth - 4, headerHeight - 2 };
            DrawTextW(memDC, m_columns[c].name.c_str(), -1, &textRc,
                DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        }
    }

    MoveToEx(memDC, 0, headerHeight - 1, nullptr);
    LineTo(memDC, client.right, headerHeight - 1);

    SetBkMode(memDC, TRANSPARENT);
    SetTextColor(memDC, GetSysColor(COLOR_WINDOWTEXT));

    int firstRow = m_scrollY / m_rowHeight;
    int visibleRows = (client.bottom - headerHeight) / m_rowHeight + 1;
    int lastRow = (std::min)(m_rowCount, firstRow + visibleRows);

    for (int r = firstRow; r < lastRow; ++r)
    {
        int y = headerHeight + r * m_rowHeight - m_scrollY;

        for (int c = 0; c < colCount; ++c)
        {
            int x = c * m_colWidth - m_scrollX;
            if (x + m_colWidth < 0) continue;
            if (x > client.right) break;

            RECT rc = { x, y, x + m_colWidth, y + m_rowHeight };

            std::wstring text;
            if (r < static_cast<int>(m_cells.size()) &&
                c < static_cast<int>(m_cells[r].size()))
            {
                text = m_cells[r][c];
            }

            RECT textRc = { x + 4, y + 2, x + m_colWidth - 4, y + m_rowHeight - 2 };
            DrawTextW(memDC, text.c_str(), -1, &textRc,
                DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

            MoveToEx(memDC, x, y, nullptr);
            LineTo(memDC, x, y + m_rowHeight);
            MoveToEx(memDC, x + m_colWidth - 1, y, nullptr);
            LineTo(memDC, x + m_colWidth - 1, y + m_rowHeight);
            MoveToEx(memDC, x, y + m_rowHeight - 1, nullptr);
            LineTo(memDC, x + m_colWidth, y + m_rowHeight - 1);
        }
    }

    SelectObject(memDC, oldPen);
    DeleteObject(gridPen);

    BitBlt(hdc, 0, 0, client.right, client.bottom, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldBmp);
    DeleteObject(memBmp);
    DeleteDC(memDC);

    EndPaint(m_hwnd, &ps);
}

void DataTableView::OnSize(int w, int h)
{
    UpdateScrollInfo();
}

void DataTableView::OnHScroll(WPARAM wParam)
{
    int action = LOWORD(wParam);
    int newPos = m_scrollX;

    switch (action)
    {
    case SB_LINELEFT:  newPos = (std::max)(0, m_scrollX - m_colWidth / 2); break;
    case SB_LINERIGHT: newPos = m_scrollX + m_colWidth / 2; break;
    case SB_PAGELEFT:  newPos = (std::max)(0, m_scrollX - m_colWidth * 3); break;
    case SB_PAGERIGHT: newPos = m_scrollX + m_colWidth * 3; break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
    {
        SCROLLINFO si = {};
        si.cbSize = sizeof(si);
        si.fMask = SIF_TRACKPOS;
        GetScrollInfo(m_hwnd, SB_HORZ, &si);
        newPos = si.nTrackPos;
        break;
    }
    default: return;
    }

    int maxScroll = (std::max)(0, m_colCount * m_colWidth);
    if (newPos < 0) newPos = 0;
    if (newPos > maxScroll) newPos = maxScroll;

    if (newPos != m_scrollX)
    {
        m_scrollX = newPos;
        UpdateScrollInfo();
        InvalidateRect(m_hwnd, nullptr, TRUE);
    }
}

void DataTableView::OnVScroll(WPARAM wParam)
{
    int action = LOWORD(wParam);
    int newPos = m_scrollY;

    switch (action)
    {
    case SB_LINEUP:   newPos = (std::max)(0, m_scrollY - m_rowHeight); break;
    case SB_LINEDOWN: newPos = m_scrollY + m_rowHeight; break;
    case SB_PAGEUP:   newPos = (std::max)(0, m_scrollY - m_rowHeight * 10); break;
    case SB_PAGEDOWN: newPos = m_scrollY + m_rowHeight * 10; break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
    {
        SCROLLINFO si = {};
        si.cbSize = sizeof(si);
        si.fMask = SIF_TRACKPOS;
        GetScrollInfo(m_hwnd, SB_VERT, &si);
        newPos = si.nTrackPos;
        break;
    }
    default: return;
    }

    int maxScroll = (std::max)(0, m_rowCount * m_rowHeight);
    if (newPos < 0) newPos = 0;
    if (newPos > maxScroll) newPos = maxScroll;

    if (newPos != m_scrollY)
    {
        m_scrollY = newPos;
        UpdateScrollInfo();
        InvalidateRect(m_hwnd, nullptr, TRUE);
    }
}

void DataTableView::UpdateScrollInfo()
{
    if (!m_hwnd) return;

    RECT client;
    GetClientRect(m_hwnd, &client);

    int totalW = (std::max)(0, m_colCount * m_colWidth);
    int totalH = m_headerHeight + (std::max)(0, m_rowCount * m_rowHeight);

    SCROLLINFO si = {};
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    si.nMin = 0;
    si.nMax = (std::max)(0, totalW - 1);
    si.nPage = client.right;
    si.nPos = m_scrollX;
    SetScrollInfo(m_hwnd, SB_HORZ, &si, TRUE);

    si.nMax = (std::max)(0, totalH - 1);
    si.nPage = client.bottom;
    si.nPos = m_scrollY;
    SetScrollInfo(m_hwnd, SB_VERT, &si, TRUE);
}
