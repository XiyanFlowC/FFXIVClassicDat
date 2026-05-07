#include "ContentPanel.h"

bool ContentPanel::s_registered = false;

const wchar_t* ContentPanel::ClassName()
{
	return L"FFXIVClassicDatExplorer_ContentPanel";
}

ContentPanel::ContentPanel() = default;
ContentPanel::~ContentPanel() = default;

bool ContentPanel::Create(HWND parent, int id)
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
		wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
		wc.lpszClassName = ClassName();
		RegisterClassExW(&wc);
		s_registered = true;
	}

	m_hwnd = CreateWindowExW(0, ClassName(), L"ContentPanel",
		WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
		0, 0, 100, 100, parent, (HMENU)(INT_PTR)id,
		GetModuleHandle(nullptr), this);

	if (!m_hwnd) return false;

	m_hwndTab = CreateWindowExW(0, WC_TABCONTROLW, L"",
		WS_CHILD | WS_VISIBLE | TCS_FIXEDWIDTH,
		0, 0, 100, 24, m_hwnd, nullptr, GetModuleHandle(nullptr), nullptr);

	HFONT font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage(m_hwndTab, WM_SETFONT, (WPARAM)font, TRUE);

	m_hwndContent = CreateWindowExW(0, L"STATIC", L"",
		WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
		0, 24, 100, 76, m_hwnd, nullptr, GetModuleHandle(nullptr), nullptr);

	return true;
}

int ContentPanel::AddTab(const std::wstring& title, HWND viewHwnd)
{
	TCITEMW item = {};
	item.mask = TCIF_TEXT;
	item.pszText = const_cast<LPWSTR>(title.c_str());

	int index = static_cast<int>(m_tabs.size());
	TabCtrl_InsertItem(m_hwndTab, index, &item);

	ContentTab tab;
	tab.title = title;
	tab.hwndView = viewHwnd;

	if (viewHwnd)
	{
		SetParent(viewHwnd, m_hwndContent);
	}

	m_tabs.push_back(tab);

	if (m_activeTab < 0)
		SetActiveTab(0);

	return index;
}

void ContentPanel::SetActiveTab(int index)
{
	if (index < 0 || index >= static_cast<int>(m_tabs.size()))
		return;

	HideAllViews();

	m_activeTab = index;
	TabCtrl_SetCurSel(m_hwndTab, index);

	ShowView(index);
}

void ContentPanel::RemoveAllTabs()
{
	TabCtrl_DeleteAllItems(m_hwndTab);
	m_tabs.clear();
	m_activeTab = -1;
}

void ContentPanel::ShowView(int index)
{
	if (index >= 0 && index < static_cast<int>(m_tabs.size()))
	{
		HWND view = m_tabs[index].hwndView;
		if (view)
		{
			ShowWindow(view, SW_SHOW);
			RECT rc;
			GetClientRect(m_hwndContent, &rc);
			SetWindowPos(view, nullptr, 0, 0, rc.right, rc.bottom, SWP_NOZORDER);
		}
	}
}

void ContentPanel::HideAllViews()
{
	for (auto& tab : m_tabs)
	{
		if (tab.hwndView)
			ShowWindow(tab.hwndView, SW_HIDE);
	}
}

void ContentPanel::Resize(int w, int h)
{
	if (m_hwnd)
	{
		SetWindowPos(m_hwnd, nullptr, 0, 0, w, h, SWP_NOZORDER | SWP_NOMOVE);
		OnSize(w, h);
	}
}

LRESULT CALLBACK ContentPanel::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	ContentPanel* panel = nullptr;

	if (msg == WM_NCCREATE)
	{
		auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
		panel = static_cast<ContentPanel*>(cs->lpCreateParams);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(panel));
		panel->m_hwnd = hwnd;
	}
	else
	{
		panel = reinterpret_cast<ContentPanel*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
	}

	if (panel)
		return panel->HandleMessage(msg, wParam, lParam);

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT ContentPanel::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_SIZE:
		OnSize(LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_NOTIFY:
	{
		NMHDR* nmh = reinterpret_cast<NMHDR*>(lParam);
		if (nmh->idFrom == 0 && nmh->code == TCN_SELCHANGE)
		{
			int sel = TabCtrl_GetCurSel(m_hwndTab);
			SetActiveTab(sel);
		}
	}
	break;

	default:
		return DefWindowProc(m_hwnd, msg, wParam, lParam);
	}
	return 0;
}

void ContentPanel::OnSize(int w, int h)
{
	if (m_hwndTab)
		SetWindowPos(m_hwndTab, nullptr, 0, 0, w, 24, SWP_NOZORDER);

	if (m_hwndContent)
	{
		SetWindowPos(m_hwndContent, nullptr, 0, 24, w, h - 24, SWP_NOZORDER);

		if (m_activeTab >= 0 && m_activeTab < static_cast<int>(m_tabs.size()))
		{
			HWND view = m_tabs[m_activeTab].hwndView;
			if (view)
			{
				SetWindowPos(view, nullptr, 0, 0, w, h - 24, SWP_NOZORDER);
			}
		}
	}
}
