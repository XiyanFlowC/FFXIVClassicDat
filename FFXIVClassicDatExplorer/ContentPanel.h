#pragma once

#include <windows.h>
#undef ERROR
#include <commctrl.h>
#include <vector>
#include <string>

struct ContentTab
{
	HWND hwndView = nullptr;
	std::wstring title;
};

class ContentPanel
{
public:
	ContentPanel();
	~ContentPanel();

	bool Create(HWND parent, int id);
	HWND GetHwnd() const { return m_hwnd; }
	HWND GetContentArea() const { return m_hwndContent; }

	int AddTab(const std::wstring& title, HWND viewHwnd);
	void SetActiveTab(int index);
	int GetActiveTab() const { return m_activeTab; }
	void RemoveAllTabs();
	void ShowView(int index);
	void HideAllViews();

	void Resize(int w, int h);

private:
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

	void OnSize(int w, int h);

	HWND m_hwnd = nullptr;
	HWND m_hwndTab = nullptr;
	HWND m_hwndContent = nullptr;
	HWND m_parent = nullptr;

	std::vector<ContentTab> m_tabs;
	int m_activeTab = -1;

	static bool s_registered;
	static const wchar_t* ClassName();
};
