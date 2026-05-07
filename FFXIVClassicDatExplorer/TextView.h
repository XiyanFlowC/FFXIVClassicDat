#pragma once

#include <windows.h>
#undef ERROR
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <cstdint>
#include <vector>
#include <string>

class TextView
{
public:
	TextView();
	~TextView();

	bool Create(HWND parent, int x, int y, int w, int h);
	HWND GetHwnd() const { return m_hwnd; }

	void LoadText(const std::string& text);
	void LoadText(const std::wstring& text);
	void LoadBytes(const uint8_t* data, size_t size, bool forceUtf8 = false);
	void Clear();

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

	std::wstring m_text;
	std::vector<int> m_lineOffsets;

	int m_scrollPos = 0;
	int m_linesPerPage = 1;
	int m_totalLines = 0;
	int m_charWidth = 1;
	int m_charHeight = 16;

	static bool s_registered;
	static const wchar_t* ClassName();
};
