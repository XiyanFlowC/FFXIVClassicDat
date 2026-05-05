#include "FileTreePanel.h"

#include <fstream>
#include <sstream>
#include <filesystem>
#include <map>

bool FileTreePanel::s_registered = false;

const wchar_t* FileTreePanel::ClassName()
{
    return L"FFXIVClassicDatExplorer_FileTreePanel";
}

FileTreePanel::FileTreePanel() = default;
FileTreePanel::~FileTreePanel() = default;

bool FileTreePanel::Create(HWND parent, int x, int y, int w, int h)
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

    m_hwnd = CreateWindowExW(0, ClassName(), L"FileTree",
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
        x, y, w, h, parent, nullptr, GetModuleHandle(nullptr), this);

    if (!m_hwnd) return false;

    HWND hTree = CreateWindowExW(0, WC_TREEVIEWW, L"",
        WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_LINESATROOT |
        TVS_HASBUTTONS | TVS_SHOWSELALWAYS | WS_BORDER,
        0, 0, w, h, m_hwnd, (HMENU)1, GetModuleHandle(nullptr), nullptr);

    HFONT font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    SendMessage(hTree, WM_SETFONT, (WPARAM)font, TRUE);

    return true;
}

void FileTreePanel::Populate(const std::wstring& gameDataPath)
{
    HWND hTree = GetWindow(m_hwnd, GW_CHILD);
    if (!hTree) return;

    TreeView_DeleteAllItems(hTree);

    std::wstring typeFilePath = gameDataPath.empty()
        ? L"type.txt"
        : gameDataPath + L"/type.txt";

    std::ifstream file(typeFilePath);
    if (!file)
    {
        HTREEITEM hRoot = AddCategory(L"No type.txt found");
        AddFileEntry(hRoot, L"Run File→Scan from the console tool first", TreeFileType::Unknown);
        return;
    }

    std::map<std::wstring, HTREEITEM> categoryNodes;

    auto ensureCategory = [&](const std::wstring& name) -> HTREEITEM {
        auto it = categoryNodes.find(name);
        if (it != categoryNodes.end()) return it->second;
        HTREEITEM h = AddCategory(name);
        categoryNodes[name] = h;
        return h;
    };

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string typeStr, pathStr;
        if (!std::getline(iss, typeStr, '\t')) continue;
        if (!std::getline(iss, pathStr)) continue;

        std::wstring wtype(typeStr.begin(), typeStr.end());
        std::wstring wpath(pathStr.begin(), pathStr.end());

        TreeFileType ft = PathToType(wtype);

        std::wstring catName;
        switch (ft)
        {
            case TreeFileType::FDT:     catName = L"Fonts (FDT)"; break;
            case TreeFileType::GTEX:    catName = L"Textures (GTEX)"; break;
            case TreeFileType::SSD:     catName = L"Spreadsheets (SSD)"; break;
            case TreeFileType::XML:     catName = L"XML Files"; break;
            case TreeFileType::DDS:     catName = L"DDS Textures"; break;
            case TreeFileType::SEDB:    catName = L"Sound DB (SEDB)"; break;
            case TreeFileType::SQWT:    catName = L"Encrypted UI (SQWT)"; break;
            case TreeFileType::VGRD:    catName = L"VFX Graph (VGRD)"; break;
            case TreeFileType::MLRD:    catName = L"Map Layout (MLRD)"; break;
            case TreeFileType::CFB:     catName = L"Compound Files (CFB)"; break;
            default:                    catName = L"Other"; break;
        }

        HTREEITEM hCat = ensureCategory(catName);
        AddFileEntry(hCat, wpath, ft);
    }
}

void FileTreePanel::Clear()
{
    HWND hTree = GetWindow(m_hwnd, GW_CHILD);
    if (hTree)
        TreeView_DeleteAllItems(hTree);
}

void FileTreePanel::Resize(int w, int h)
{
    if (m_hwnd)
    {
        SetWindowPos(m_hwnd, nullptr, 0, 0, w, h, SWP_NOZORDER | SWP_NOMOVE);
        HWND hTree = GetWindow(m_hwnd, GW_CHILD);
        if (hTree)
            SetWindowPos(hTree, nullptr, 0, 0, w, h, SWP_NOZORDER);
    }
}

HTREEITEM FileTreePanel::AddCategory(const std::wstring& name)
{
    HWND hTree = GetWindow(m_hwnd, GW_CHILD);
    if (!hTree) return nullptr;

    TVINSERTSTRUCT tv = {};
    tv.hInsertAfter = TVI_ROOT;
    tv.item.mask = TVIF_TEXT | TVIF_STATE;
    tv.item.pszText = const_cast<LPWSTR>(name.c_str());
    tv.item.state = TVIS_EXPANDED;
    tv.item.stateMask = TVIS_EXPANDED;

    return TreeView_InsertItem(hTree, &tv);
}

void FileTreePanel::AddFileEntry(HTREEITEM parent, const std::wstring& path, TreeFileType type)
{
    HWND hTree = GetWindow(m_hwnd, GW_CHILD);
    if (!hTree) return;

    std::filesystem::path p(path);
    std::wstring display = p.filename().wstring();

    TVINSERTSTRUCT tv = {};
    tv.hParent = parent;
    tv.hInsertAfter = TVI_LAST;
    tv.item.mask = TVIF_TEXT;
    tv.item.pszText = const_cast<LPWSTR>(display.c_str());

    HTREEITEM hItem = TreeView_InsertItem(hTree, &tv);

    (void)type;
    (void)hItem;
}

TreeFileType FileTreePanel::PathToType(const std::wstring& typeStr)
{
    if (typeStr == L"VERS") return TreeFileType::FDT;
    if (typeStr == L"GTEX") return TreeFileType::GTEX;
    if (typeStr == L"SSD")  return TreeFileType::SSD;
    if (typeStr == L"XML")  return TreeFileType::XML;
    if (typeStr == L"DDS")  return TreeFileType::DDS;
    if (typeStr == L"SEDB") return TreeFileType::SEDB;
    if (typeStr == L"VGRD") return TreeFileType::VGRD;
    if (typeStr == L"MLRD") return TreeFileType::MLRD;
    if (typeStr == L"CFB")  return TreeFileType::CFB;
    return TreeFileType::Unknown;
}

LRESULT CALLBACK FileTreePanel::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    FileTreePanel* panel = nullptr;

    if (msg == WM_NCCREATE)
    {
        auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        panel = static_cast<FileTreePanel*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(panel));
        panel->m_hwnd = hwnd;
    }
    else
    {
        panel = reinterpret_cast<FileTreePanel*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (panel)
        return panel->HandleMessage(msg, wParam, lParam);

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT FileTreePanel::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_SIZE:
    {
        HWND hTree = GetWindow(m_hwnd, GW_CHILD);
        if (hTree)
        {
            SetWindowPos(hTree, nullptr, 0, 0, LOWORD(lParam), HIWORD(lParam), SWP_NOZORDER);
        }
        break;
    }

    case WM_NOTIFY:
    {
        NMHDR* nmh = reinterpret_cast<NMHDR*>(lParam);
        if (nmh->idFrom == 1 && nmh->code == NM_DBLCLK)
        {
            OnDblClk();
        }
        break;
    }

    default:
        return DefWindowProc(m_hwnd, msg, wParam, lParam);
    }
    return 0;
}

void FileTreePanel::OnDblClk()
{
    HWND hTree = GetWindow(m_hwnd, GW_CHILD);
    if (!hTree) return;

    HTREEITEM hSel = TreeView_GetSelection(hTree);
    if (!hSel) return;

    TVITEM item = {};
    item.hItem = hSel;
    item.mask = TVIF_HANDLE;

    if (!TreeView_GetItem(hTree, &item)) return;

    if (m_onFileSelect)
    {
        // For now, we don't store the full path in tree items.
        // This will be enhanced when the tree stores proper data.
        m_onFileSelect(L"", TreeFileType::Unknown);
    }
}
