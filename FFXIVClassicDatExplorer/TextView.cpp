#include "TextView.h"
#include <algorithm>
#include <cstring>

bool TextView::s_registered = false;

const wchar_t* TextView::ClassName()
{
	return L"FFXIVClassicDatExplorer_TextView";
}

TextView::TextView() = default;

TextView::~TextView()
{
	if (m_font)
		DeleteObject(m_font);
}

bool TextView::Create(HWND parent, int x, int y, int w, int h)
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

	m_hwnd = CreateWindowExW(0, ClassName(), L"TextView",
		WS_CHILD | WS_VISIBLE | WS_VSCROLL,
		x, y, w, h, parent, nullptr, GetModuleHandle(nullptr), this);

	return m_hwnd != nullptr;
}

void TextView::LoadText(const std::string& text)
{
	int len = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
	if (len <= 0)
	{
		LoadText(std::wstring(text.begin(), text.end()));
		return;
	}
	std::wstring wtext(len, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, &wtext[0], len);
	while (!wtext.empty() && wtext.back() == L'\0') wtext.pop_back();
	LoadText(wtext);
}

void TextView::LoadText(const std::wstring& text)
{
	m_text = text;
	m_lineOffsets.clear();
	m_lineOffsets.push_back(0);

	for (size_t i = 0; i < m_text.size(); ++i)
	{
		if (m_text[i] == L'\n')
			m_lineOffsets.push_back(static_cast<int>(i) + 1);
	}

	m_totalLines = static_cast<int>(m_lineOffsets.size());
	if (m_totalLines == 0) m_totalLines = 1;

	m_scrollPos = 0;
	UpdateScrollInfo();
	if (m_hwnd) InvalidateRect(m_hwnd, nullptr, TRUE);
}

void TextView::LoadBytes(const uint8_t* data, size_t size, bool forceUtf8)
{
	if (!data || size == 0)
	{
		Clear();
		return;
	}

	std::string str(reinterpret_cast<const char*>(data), size);

	// Try UTF-8 first, then Shift-JIS
	std::wstring wtext;
	if (forceUtf8)
	{
		LoadText(str);
		return;
	}

	int len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str.c_str(), -1, nullptr, 0);
	if (len > 0)
	{
		wtext.resize(len);
		MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str.c_str(), -1, &wtext[0], len);
	}
	else
	{
		len = MultiByteToWideChar(932, 0, str.c_str(), -1, nullptr, 0);
		if (len > 0)
		{
			wtext.resize(len);
			MultiByteToWideChar(932, 0, str.c_str(), -1, &wtext[0], len);
		}
		else
		{
			wtext.assign(str.begin(), str.end());
		}
	}

	LoadText(wtext);
}

void TextView::Clear()
{
	m_text.clear();
	m_lineOffsets.clear();
	m_totalLines = 0;
	m_scrollPos = 0;
	if (m_hwnd) InvalidateRect(m_hwnd, nullptr, TRUE);
}

void TextView::Show()
{
	if (m_hwnd) ShowWindow(m_hwnd, SW_SHOW);
}

void TextView::Hide()
{
	if (m_hwnd) ShowWindow(m_hwnd, SW_HIDE);
}

void TextView::Resize(int w, int h)
{
	if (m_hwnd)
	{
		SetWindowPos(m_hwnd, nullptr, 0, 0, w, h, SWP_NOZORDER | SWP_NOMOVE);
		UpdateScrollInfo();
	}
}

LRESULT CALLBACK TextView::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	TextView* view = nullptr;

	if (msg == WM_NCCREATE)
	{
		auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
		view = static_cast<TextView*>(cs->lpCreateParams);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(view));
		view->m_hwnd = hwnd;
	}
	else
	{
		view = reinterpret_cast<TextView*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
	}

	if (view)
		return view->HandleMessage(msg, wParam, lParam);

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT TextView::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
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
		m_scrollPos = std::max(0, m_scrollPos - delta * 3);
		m_scrollPos = std::min(m_scrollPos, std::max(0, m_totalLines - m_linesPerPage));
		UpdateScrollInfo();
		InvalidateRect(m_hwnd, nullptr, TRUE);
	}
	break;

	case WM_DESTROY:
		if (m_font) { DeleteObject(m_font); m_font = nullptr; }
		break;

	default:
		return DefWindowProc(m_hwnd, msg, wParam, lParam);
	}
	return 0;
}

void TextView::OnPaint()
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

	if (m_font) SelectObject(memDC, m_font);
	SetBkMode(memDC, TRANSPARENT);
	SetTextColor(memDC, GetSysColor(COLOR_WINDOWTEXT));

	SetBkMode(memDC, OPAQUE);
	SetBkColor(memDC, GetSysColor(COLOR_WINDOW));

	int firstLine = m_scrollPos;
	int lastLine = std::min(firstLine + m_linesPerPage + 1, m_totalLines);

	for (int i = firstLine; i < lastLine && i < static_cast<int>(m_lineOffsets.size()); ++i)
	{
		int y = (i - firstLine) * m_charHeight;
		if (y + m_charHeight > client.bottom) break;

		int start = m_lineOffsets[i];
		int end = (i + 1 < static_cast<int>(m_lineOffsets.size()))
			? m_lineOffsets[i + 1] - 1
			: static_cast<int>(m_text.size());

		if (end <= start)
		{
			TextOutW(memDC, 4, y, L"", 0);
			continue;
		}

		int len = end - start;
		if (len > 2048) len = 2048;

		TextOutW(memDC, 4, y, m_text.c_str() + start, len);
	}

	BitBlt(hdc, 0, 0, client.right, client.bottom, memDC, 0, 0, SRCCOPY);

	SelectObject(memDC, oldBmp);
	DeleteObject(memBmp);
	DeleteDC(memDC);

	EndPaint(m_hwnd, &ps);
}

void TextView::OnSize(int w, int h)
{
	m_linesPerPage = std::max(1, h / std::max(1, m_charHeight));
	UpdateScrollInfo();
}

void TextView::OnVScroll(WPARAM wParam)
{
	int action = LOWORD(wParam);
	int newPos = m_scrollPos;

	switch (action)
	{
	case SB_LINEUP:        newPos = std::max(0, m_scrollPos - 1); break;
	case SB_LINEDOWN:      newPos = std::min(m_totalLines - m_linesPerPage, m_scrollPos + 1); break;
	case SB_PAGEUP:        newPos = std::max(0, m_scrollPos - m_linesPerPage); break;
	case SB_PAGEDOWN:      newPos = std::min(m_totalLines - m_linesPerPage, m_scrollPos + m_linesPerPage); break;
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
	if (newPos > std::max(0, m_totalLines - m_linesPerPage))
		newPos = std::max(0, m_totalLines - m_linesPerPage);

	if (newPos != m_scrollPos)
	{
		int delta = m_scrollPos - newPos;
		m_scrollPos = newPos;
		ScrollWindowEx(m_hwnd, 0, delta * m_charHeight, nullptr, nullptr, nullptr, nullptr, SW_INVALIDATE);
		SetScrollPos(m_hwnd, SB_VERT, m_scrollPos, TRUE);
	}
}

void TextView::UpdateScrollInfo()
{
	if (!m_hwnd) return;

	if (m_totalLines == 0) m_totalLines = 1;

	RECT client;
	GetClientRect(m_hwnd, &client);
	int clientH = static_cast<int>(client.bottom - client.top);
	m_linesPerPage = std::max(1, clientH / std::max(1, m_charHeight));

	SCROLLINFO si = {};
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	si.nMin = 0;
	si.nMax = std::max(0, m_totalLines - 1);
	si.nPage = m_linesPerPage;
	si.nPos = m_scrollPos;
	SetScrollInfo(m_hwnd, SB_VERT, &si, TRUE);
}
