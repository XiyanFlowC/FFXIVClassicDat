#include "HexView.h"
#include <cstdio>
#include <algorithm>

bool HexView::s_registered = false;

const wchar_t* HexView::ClassName()
{
	return L"FFXIVClassicDatExplorer_HexView";
}

HexView::HexView() = default;

HexView::~HexView()
{
	if (m_font)
		DeleteObject(m_font);
}

bool HexView::Create(HWND parent, int x, int y, int w, int h)
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

	m_hwnd = CreateWindowExW(0, ClassName(), L"HexView",
		WS_CHILD | WS_VISIBLE | WS_VSCROLL,
		x, y, w, h, parent, nullptr, GetModuleHandle(nullptr), this);

	return m_hwnd != nullptr;
}

void HexView::LoadData(const uint8_t* data, size_t size)
{
	m_data.assign(data, data + size);
	m_scrollPos = 0;
	UpdateScrollInfo();
	if (m_hwnd)
		InvalidateRect(m_hwnd, nullptr, TRUE);
}

void HexView::ClearData()
{
	m_data.clear();
	m_scrollPos = 0;
	UpdateScrollInfo();
	if (m_hwnd)
		InvalidateRect(m_hwnd, nullptr, TRUE);
}

void HexView::Show()
{
	if (m_hwnd)
		ShowWindow(m_hwnd, SW_SHOW);
}

void HexView::Hide()
{
	if (m_hwnd)
		ShowWindow(m_hwnd, SW_HIDE);
}

void HexView::Resize(int w, int h)
{
	if (m_hwnd)
	{
		SetWindowPos(m_hwnd, nullptr, 0, 0, w, h, SWP_NOZORDER | SWP_NOMOVE);
		UpdateScrollInfo();
	}
}

LRESULT CALLBACK HexView::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HexView* view = nullptr;

	if (msg == WM_NCCREATE)
	{
		auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
		view = static_cast<HexView*>(cs->lpCreateParams);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(view));
		view->m_hwnd = hwnd;
	}
	else
	{
		view = reinterpret_cast<HexView*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
	}

	if (view)
		return view->HandleMessage(msg, wParam, lParam);

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT HexView::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
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
		if (m_font)
			SelectObject(hdc, m_font);
		TEXTMETRIC tm;
		GetTextMetrics(hdc, &tm);
		m_charWidth = tm.tmAveCharWidth;
		m_charHeight = tm.tmHeight + tm.tmExternalLeading;
		ReleaseDC(m_hwnd, hdc);
	}
	break;

	case WM_PAINT:
		OnPaint();
		break;

	case WM_SIZE:
		OnSize(LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_VSCROLL:
		OnVScroll(wParam);
		break;

	case WM_MOUSEWHEEL:
	{
		int delta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
		m_scrollPos = (std::max)(0, m_scrollPos - delta * 3);
		m_scrollPos = (std::min)(m_scrollPos, (std::max)(0, m_totalLines - m_linesPerPage));
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

void HexView::OnPaint()
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(m_hwnd, &ps);
	if (!hdc) return;

	RECT client;
	GetClientRect(m_hwnd, &client);
	int totalWidth = 75 * m_charWidth;

	HDC memDC = CreateCompatibleDC(hdc);
	HBITMAP memBmp = CreateCompatibleBitmap(hdc, client.right, client.bottom);
	HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

	HBRUSH bgBrush = (HBRUSH)(COLOR_WINDOW + 1);
	FillRect(memDC, &client, bgBrush);

	if (m_font)
		SelectObject(memDC, m_font);

	SetBkMode(memDC, TRANSPARENT);
	SetTextColor(memDC, GetSysColor(COLOR_WINDOWTEXT));

	int firstLine = m_scrollPos;
	int lastLine = (std::min)(firstLine + m_linesPerPage + 1, m_totalLines);

	wchar_t lineBuf[128];

	for (int i = firstLine; i < lastLine; ++i)
	{
		int y = (i - firstLine) * m_charHeight;
		if (y + m_charHeight > client.bottom) break;

		size_t offset = static_cast<size_t>(i) * 16;
		if (offset >= m_data.size()) break;

		int rem = (std::min)(16, static_cast<int>(m_data.size() - offset));

		int pos = swprintf_s(lineBuf, L"%08zX  ", offset);

		for (int j = 0; j < 16; ++j)
		{
			if (j < rem)
			{
				swprintf_s(lineBuf + pos, 128 - pos, L"%02X ", m_data[offset + j]);
				pos += 3;
			}
			else
			{
				lineBuf[pos++] = L' ';
				lineBuf[pos++] = L' ';
				lineBuf[pos++] = L' ';
			}
			if (j == 7) lineBuf[pos++] = L' ';
		}

		lineBuf[pos++] = L' ';
		for (int j = 0; j < rem; ++j)
		{
			uint8_t c = m_data[offset + j];
			lineBuf[pos++] = (c >= 0x20 && c < 0x7F) ? static_cast<wchar_t>(c) : L'.';
		}
		lineBuf[pos] = L'\0';

		TextOutW(memDC, 0, y, lineBuf, pos);
	}

	if (firstLine != m_scrollPos)
	{
		ScrollDC(memDC, 0, (firstLine - m_scrollPos) * m_charHeight, nullptr, nullptr, nullptr, nullptr);
	}

	BitBlt(hdc, 0, 0, client.right, client.bottom, memDC, 0, 0, SRCCOPY);

	SelectObject(memDC, oldBmp);
	DeleteObject(memBmp);
	DeleteDC(memDC);

	EndPaint(m_hwnd, &ps);
}

void HexView::OnSize(int w, int h)
{
	m_linesPerPage = (std::max)(1, h / (std::max)(1, m_charHeight));
	UpdateScrollInfo();
}

void HexView::OnVScroll(WPARAM wParam)
{
	int action = LOWORD(wParam);
	int newPos = m_scrollPos;

	switch (action)
	{
	case SB_LINEUP:        newPos = (std::max)(0, m_scrollPos - 1); break;
	case SB_LINEDOWN:      newPos = (std::min)(m_totalLines - m_linesPerPage, m_scrollPos + 1); break;
	case SB_PAGEUP:        newPos = (std::max)(0, m_scrollPos - m_linesPerPage); break;
	case SB_PAGEDOWN:      newPos = (std::min)(m_totalLines - m_linesPerPage, m_scrollPos + m_linesPerPage); break;
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

	if (newPos < 0) newPos = 0;
	if (newPos > (std::max)(0, m_totalLines - m_linesPerPage))
		newPos = (std::max)(0, m_totalLines - m_linesPerPage);

	if (newPos != m_scrollPos)
	{
		int delta = m_scrollPos - newPos;
		m_scrollPos = newPos;
		ScrollWindowEx(m_hwnd, 0, delta * m_charHeight, nullptr, nullptr, nullptr, nullptr, SW_INVALIDATE);
		SetScrollPos(m_hwnd, SB_VERT, m_scrollPos, TRUE);
	}
}

void HexView::UpdateScrollInfo()
{
	if (!m_hwnd) return;

	m_totalLines = (static_cast<int>(m_data.size()) + 15) / 16;
	if (m_totalLines == 0) m_totalLines = 1;

	RECT client;
	GetClientRect(m_hwnd, &client);
	int clientHeight = static_cast<int>(client.bottom - client.top);
	m_linesPerPage = (std::max)(1, clientHeight / (std::max)(1, m_charHeight));

	SCROLLINFO si = {};
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	si.nMin = 0;
	si.nMax = (std::max)(0, m_totalLines - 1);
	si.nPage = m_linesPerPage;
	si.nPos = m_scrollPos;
	SetScrollInfo(m_hwnd, SB_VERT, &si, TRUE);
}
