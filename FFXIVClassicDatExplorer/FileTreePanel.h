#pragma once

#include <windows.h>
#undef ERROR
#include <commctrl.h>
#include <string>
#include <vector>
#include <functional>

enum class TreeFileType
{
	FDT, GTEX, SSD, XML, DDS, SEDB, SQWT, VGRD, MLRD, CFB, Unknown
};

struct TreeFileEntry
{
	std::wstring path;
	TreeFileType type;
};

class FileTreePanel
{
public:
	using FileSelectCallback = std::function<void(const std::wstring& path, TreeFileType type)>;

	FileTreePanel();
	~FileTreePanel();

	bool Create(HWND parent, int x, int y, int w, int h);
	HWND GetHwnd() const { return m_hwnd; }

	void Populate(const std::wstring& gameDataPath);
	void Clear();
	void SetOnFileSelect(FileSelectCallback cb) { m_onFileSelect = std::move(cb); }

	void Resize(int w, int h);

private:
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

	void OnDblClk();
	HTREEITEM AddCategory(const std::wstring& name);
	void AddFileEntry(HTREEITEM parent, const std::wstring& path, TreeFileType type);
	TreeFileType PathToType(const std::wstring& typeStr);

	HWND m_hwnd = nullptr;
	HWND m_parent = nullptr;
	FileSelectCallback m_onFileSelect;

	static bool s_registered;
	static const wchar_t* ClassName();
};
