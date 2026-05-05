#pragma once

#include <windows.h>
#undef ERROR
#include <commctrl.h>
#include <shellapi.h>
#include <vector>
#include <string>
#include <memory>

#include "FileTypeDetector.h"

class FileTreePanel;
class ContentPanel;
class HexView;
class DataTableView;
class TextureView;
class PatchSummaryView;

class MainWindow
{
public:
    MainWindow(HINSTANCE hInstance);
    ~MainWindow();

    bool Init(int nCmdShow);
    int Run();
    HWND GetHwnd() const { return m_hwnd; }

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    void OnCreate();
    void OnSize(int w, int h);
    void OnCommand(int id);
    void OnDropFiles(HDROP hDrop);
    void OnDestroy();

    void OpenFile(const std::wstring& path);
    void OpenGameFile(uint32_t fileId);
    void OpenPatchFile(const std::wstring& path);
    void OpenSsdFile(const std::wstring& path);

    void SetStatusText(const std::wstring& text);

    void UpdateLayout();
    void OnSplitterDrag(int mouseY);
    void PopulateFileTree();

    HINSTANCE m_hInstance;
    HWND m_hwnd = nullptr;
    HWND m_hwndStatus = nullptr;
    HWND m_hwndSplitter = nullptr;

    FileTreePanel* m_fileTree = nullptr;
    ContentPanel* m_contentPanel = nullptr;

    // Views — one of each type reused
    HexView* m_hexView = nullptr;
    DataTableView* m_dataTableView = nullptr;
    TextureView* m_textureView = nullptr;
    PatchSummaryView* m_patchView = nullptr;

    // Tab indices
    int m_tabHex = -1;
    int m_tabTable = -1;
    int m_tabTexture = -1;
    int m_tabPatch = -1;

    std::wstring m_currentFilePath;

    int m_splitterPos = 250;
    bool m_draggingSplitter = false;
    int m_minLeftWidth = 150;
    int m_minRightWidth = 200;

    static bool s_registered;
    static const wchar_t* ClassName();
};
