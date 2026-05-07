#include "MainWindow.h"

#include <shellapi.h>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>

#include "resource.h"
#include "ContentPanel.h"
#include "FileTreePanel.h"
#include "HexView.h"
#include "DataTableView.h"
#include "TextureView.h"
#include "PatchSummaryView.h"
#include "TextView.h"
#include "FontView.h"
#include "FileTypeDetector.h"
#include "ProgressDialog.h"
#include "InputPrompt.h"

#include "../FFXIVClassicDat/DataManager.h"
#include "../FFXIVClassicDat/BinaryData.h"
#include "../FFXIVClassicDat/SsdData.h"
#include "../FFXIVClassicDat/Sheet.h"
#include "../FFXIVClassicDat/SqwtFile.h"
#include "../FFXIVClassicDatConsole/Config.h"

#undef min

bool MainWindow::s_registered = false;

const wchar_t* MainWindow::ClassName()
{
	return L"FFXIVClassicDatExplorer_MainWindow";
}

MainWindow::MainWindow(HINSTANCE hInstance) : m_hInstance(hInstance) {}

MainWindow::~MainWindow()
{
	delete m_fileTree;
	delete m_contentPanel;
	delete m_hexView;
	delete m_dataTableView;
	delete m_textureView;
	delete m_patchView;
	delete m_textView;
	delete m_fontView;
}

bool MainWindow::Init(int nCmdShow)
{
	if (!s_registered)
	{
		WNDCLASSEXW wc = {};
		wc.cbSize = sizeof(wc);
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = WndProc;
		wc.hInstance = m_hInstance;
		wc.hIcon = LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_FFXIVCLASSICDATEXPLORER));
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
		wc.lpszMenuName = MAKEINTRESOURCEW(IDC_FFXIVCLASSICDATEXPLORER);
		wc.lpszClassName = ClassName();
		wc.hIconSm = LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_SMALL));
		RegisterClassExW(&wc);
		s_registered = true;
	}

	WCHAR title[256];
	LoadStringW(m_hInstance, IDS_APP_TITLE, title, 256);

	m_hwnd = CreateWindowExW(0, ClassName(), title,
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0,
		1200, 800, nullptr, nullptr, m_hInstance, this);

	if (!m_hwnd) return false;

	ShowWindow(m_hwnd, nCmdShow);
	UpdateWindow(m_hwnd);

	return true;
}

int MainWindow::Run()
{
	MSG msg;
	HACCEL hAccel = LoadAccelerators(m_hInstance, MAKEINTRESOURCE(IDC_FFXIVCLASSICDATEXPLORER));

	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccel, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK MainWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	MainWindow* mw = nullptr;

	if (msg == WM_NCCREATE)
	{
		auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
		mw = static_cast<MainWindow*>(cs->lpCreateParams);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(mw));
		mw->m_hwnd = hwnd;
	}
	else
	{
		mw = reinterpret_cast<MainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
	}

	if (mw)
		return mw->HandleMessage(msg, wParam, lParam);

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT MainWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CREATE:
		OnCreate();
		DragAcceptFiles(m_hwnd, TRUE);
		break;

	case WM_SIZE:
		OnSize(LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_COMMAND:
		OnCommand(LOWORD(wParam));
		break;

	case WM_DROPFILES:
		OnDropFiles(reinterpret_cast<HDROP>(wParam));
		break;

	case WM_DESTROY:
		DragAcceptFiles(m_hwnd, FALSE);
		OnDestroy();
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(m_hwnd, msg, wParam, lParam);
	}
	return 0;
}

void MainWindow::PopulateFileTree()
{
	if (!m_fileTree) return;
	std::wstring gamePath = Config::GetInstance().m_ffxivInstallPath;
	std::wstring typePath = gamePath + L"/type.txt";

	if (!std::filesystem::exists(typePath))
	{
		typePath = L"type.txt";
	}

	m_fileTree->Populate(gamePath);
}

void MainWindow::OnCreate()
{
	INITCOMMONCONTROLSEX icc = {};
	icc.dwSize = sizeof(icc);
	icc.dwICC = ICC_TAB_CLASSES | ICC_TREEVIEW_CLASSES | ICC_STANDARD_CLASSES | ICC_BAR_CLASSES;
	InitCommonControlsEx(&icc);

	m_hwndStatus = CreateWindowExW(0, STATUSCLASSNAMEW, L"",
		WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
		0, 0, 0, 0, m_hwnd, nullptr, m_hInstance, nullptr);

	int parts[] = { 400, 600 };
	SendMessage(m_hwndStatus, SB_SETPARTS, 2, reinterpret_cast<LPARAM>(parts));
	SendMessage(m_hwndStatus, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(L"Ready"));

	m_hwndSplitter = CreateWindowExW(0, L"STATIC", L"",
		WS_CHILD | WS_VISIBLE | SS_ETCHEDVERT,
		250, 0, 4, 100, m_hwnd, nullptr, m_hInstance, nullptr);

	m_fileTree = new FileTreePanel();
	m_fileTree->Create(m_hwnd, 0, 0, m_splitterPos, 600);
	m_fileTree->SetOnFileSelect([this](const std::wstring& path, TreeFileType type) {
		if (!path.empty())
			OpenFile(path);
	});

	m_contentPanel = new ContentPanel();
	m_contentPanel->Create(m_hwnd, 2000);

	m_hexView = new HexView();
	m_hexView->Create(m_hwnd, 0, 0, 100, 100);
	m_tabHex = m_contentPanel->AddTab(L"Hex", m_hexView->GetHwnd());

	m_dataTableView = new DataTableView();
	m_dataTableView->Create(m_hwnd, 0, 0, 100, 100);
	m_tabTable = m_contentPanel->AddTab(L"Data Table", m_dataTableView->GetHwnd());

	m_textureView = new TextureView();
	m_textureView->Create(m_hwnd, 0, 0, 100, 100);
	m_tabTexture = m_contentPanel->AddTab(L"Texture", m_textureView->GetHwnd());

	m_patchView = new PatchSummaryView();
	m_patchView->Create(m_hwnd, 0, 0, 100, 100);
	m_tabPatch = m_contentPanel->AddTab(L"Patch", m_patchView->GetHwnd());

	m_textView = new TextView();
	m_textView->Create(m_hwnd, 0, 0, 100, 100);
	m_tabText = m_contentPanel->AddTab(L"Text", m_textView->GetHwnd());

	m_fontView = new FontView();
	m_fontView->Create(m_hwnd, 0, 0, 100, 100);
	m_tabFont = m_contentPanel->AddTab(L"Font", m_fontView->GetHwnd());

	PopulateFileTree();

	SetStatusText(L"Ready — Drag & drop a file or use File→Open");
}

void MainWindow::OnSize(int w, int h)
{
	if (m_hwndStatus)
	{
		SendMessage(m_hwndStatus, WM_SIZE, 0, 0);
	}
	UpdateLayout();
}

void MainWindow::UpdateLayout()
{
	RECT rc;
	GetClientRect(m_hwnd, &rc);
	int w = rc.right;
	int h = rc.bottom;

	RECT rcStatus = {};
	if (m_hwndStatus)
		GetWindowRect(m_hwndStatus, &rcStatus);
	int statusHeight = rcStatus.bottom - rcStatus.top;
	int contentBottom = h - statusHeight;

	int splitterWidth = 4;
	int splitterPos = m_splitterPos;

	if (splitterPos < m_minLeftWidth) splitterPos = m_minLeftWidth;
	if (splitterPos > w - m_minRightWidth - splitterWidth)
		splitterPos = w - m_minRightWidth - splitterWidth;

	m_splitterPos = splitterPos;

	if (m_hwndSplitter)
	{
		SetWindowPos(m_hwndSplitter, nullptr, splitterPos, 0, splitterWidth,
			contentBottom, SWP_NOZORDER);
	}

	int rightX = splitterPos + splitterWidth;
	int rightW = w - rightX;

	if (m_fileTree && m_fileTree->GetHwnd())
	{
		SetWindowPos(m_fileTree->GetHwnd(), nullptr, 0, 0, splitterPos,
			contentBottom, SWP_NOZORDER);
	}

	if (m_contentPanel && m_contentPanel->GetHwnd())
	{
		SetWindowPos(m_contentPanel->GetHwnd(), nullptr, rightX, 0, rightW,
			contentBottom, SWP_NOZORDER);
	}
}

void MainWindow::OnCommand(int id)
{
	switch (id)
	{
	case IDM_FILE_OPEN:
	{
		OPENFILENAMEW ofn = {};
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = m_hwnd;
		ofn.lpstrFilter = L"All Supported Files\0*.dat;*.gtx;*.patch;*.fdt;*.dds\0"
						  L"Data Files (*.dat)\0*.dat\0"
						  L"Patch Files (*.patch)\0*.patch\0"
						  L"All Files (*.*)\0*.*\0";
		wchar_t buf[MAX_PATH] = {};
		ofn.lpstrFile = buf;
		ofn.nMaxFile = MAX_PATH;
		ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

		if (GetOpenFileNameW(&ofn))
		{
			OpenFile(ofn.lpstrFile);
		}
		break;
	}
	case IDM_FILE_OPEN_PATCH:
	{
		OPENFILENAMEW ofn = {};
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = m_hwnd;
		ofn.lpstrFilter = L"Patch Files (*.patch)\0*.patch\0All Files (*.*)\0*.*\0";
		wchar_t buf[MAX_PATH] = {};
		ofn.lpstrFile = buf;
		ofn.nMaxFile = MAX_PATH;
		ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

		if (GetOpenFileNameW(&ofn))
		{
			OpenPatchFile(ofn.lpstrFile);
		}
		break;
	}
    case IDM_TOOLS_CONFIG:
    {
        std::wstring newPath = Config::GetInstance().m_ffxivInstallPath;
        if (InputPrompt::Show(m_hwnd, L"Configure Game Path",
            L"Enter the FFXIV Classic game install path:",
            newPath, Config::GetInstance().m_ffxivInstallPath))
        {
            Config::GetInstance().m_ffxivInstallPath = newPath;
            PopulateFileTree();
            SetStatusText(L"Game path updated: " + newPath);
        }
        break;
    }
	case IDM_ABOUT:
		MessageBoxW(m_hwnd, L"FFXIV Classic Dat Explorer\n\n"
			L"Explore and view FFXIV 1.0 data files.\n"
			L"Supports data tables, textures, and patch files.",
			L"About", MB_OK | MB_ICONINFORMATION);
		break;
	case IDM_EXIT:
		DestroyWindow(m_hwnd);
		break;
	}
}

void MainWindow::OnDropFiles(HDROP hDrop)
{
	UINT fileCount = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);
	if (fileCount > 0)
	{
		wchar_t path[MAX_PATH];
		DragQueryFileW(hDrop, 0, path, MAX_PATH);
		OpenFile(path);
	}
	DragFinish(hDrop);
}

void MainWindow::OpenFile(const std::wstring& path)
{
	m_currentFilePath = path;

	// Check if file exists
	if (!std::filesystem::exists(path))
	{
		MessageBoxW(m_hwnd, (L"File not found:\n" + path).c_str(),
			L"Error", MB_OK | MB_ICONERROR);
		return;
	}

	// Get file size first
	std::error_code ec;
	auto fileSize = std::filesystem::file_size(path, ec);
	if (ec)
	{
		MessageBoxW(m_hwnd, (L"Failed to get file size:\n" + path).c_str(),
			L"Error", MB_OK | MB_ICONERROR);
		return;
	}

	// Show progress dialog for large files (> 1MB)
	bool showProgress = fileSize > 1024 * 1024;
	std::unique_ptr<ProgressDialog> progress;

	if (showProgress)
	{
		progress = std::make_unique<ProgressDialog>(m_hwnd, L"Loading File", L"Reading file...");
		progress->Show();
	}

	std::vector<uint8_t> data;
	bool loadSuccess = false;
	std::wstring errorMsg;

	auto loadTask = [&]() {
		try {
			std::ifstream file(path, std::ios::binary | std::ios::ate);
			if (!file)
			{
				errorMsg = L"Failed to open file";
				return;
			}

			size_t size = static_cast<size_t>(file.tellg());
			file.seekg(0);

			data.resize(size);

			if (showProgress)
			{
				const size_t chunkSize = 1024 * 1024; // 1MB chunks
				size_t bytesRead = 0;
				while (bytesRead < size)
				{
					size_t toRead = std::min(chunkSize, size - bytesRead);
					file.read(reinterpret_cast<char*>(data.data() + bytesRead), toRead);
					bytesRead += toRead;

					if (progress)
					{
						progress->SetProgress(bytesRead, size);
						progress->SetMessage(L"Reading: " + std::to_wstring(bytesRead / 1024) +
							L" KB / " + std::to_wstring(size / 1024) + L" KB");

						if (progress->IsCancelled())
						{
							errorMsg = L"Load cancelled by user";
							return;
						}
					}
				}
			}
			else
			{
				file.read(reinterpret_cast<char*>(data.data()), size);
			}

			file.close();
			loadSuccess = true;
		}
		catch (const std::exception& ex)
		{
			std::string err(ex.what());
			errorMsg = std::wstring(err.begin(), err.end());
		}
	};

	if (showProgress && progress)
	{
		progress->RunTask(loadTask);
		progress->WaitForCompletion();
	}
	else
	{
		loadTask();
	}

	if (!loadSuccess)
	{
		MessageBoxW(m_hwnd, (L"Failed to load file:\n" + path + L"\n\n" + errorMsg).c_str(),
			L"Error", MB_OK | MB_ICONERROR);
		return;
	}

	auto result = DetectFileTypeDetail(data.data(), data.size());
	FileType type = result.type;

	wchar_t statusMsg[256];
	swprintf_s(statusMsg, L"Loaded: %s | Type: %.*s | Size: %zu bytes",
		path.c_str(), static_cast<int>(FileTypeName(type).size()),
		FileTypeName(type).data(), data.size());
	SetStatusText(statusMsg);

	switch (type)
	{
	case FileType::GTEX:
		m_textureView->LoadTexture(data.data(), data.size());
		m_contentPanel->SetActiveTab(m_tabTexture);
		m_hexView->LoadData(data.data(), data.size());
		break;

	case FileType::DDS:
		m_hexView->LoadData(data.data(), data.size());
		m_contentPanel->SetActiveTab(m_tabHex);
		break;

	case FileType::SQWT:
		OpenSqwtFile(path);
		break;

	case FileType::FDT:
		m_fontView->LoadFdt(data.data(), data.size());
		m_contentPanel->SetActiveTab(m_tabFont);
		m_hexView->LoadData(data.data(), data.size());
		break;

	case FileType::SSD:
		OpenSsdFile(path);
		break;

	case FileType::XML:
		m_hexView->LoadData(data.data(), data.size());
		m_contentPanel->SetActiveTab(m_tabHex);
		break;

	case FileType::ZiPatch:
		m_patchView->LoadPatch(path);
		m_contentPanel->SetActiveTab(m_tabPatch);
		SetStatusText(L"Patch loaded: " + path);
		return;

	default:
		m_hexView->LoadData(data.data(), data.size());
		m_contentPanel->SetActiveTab(m_tabHex);
		break;
	}
}

void MainWindow::OpenGameFile(uint32_t fileId)
{
	// TODO: load from DataManager when file tree navigation is implemented
}

void MainWindow::OpenSsdFile(const std::wstring& path)
{
	ProgressDialog progress(m_hwnd, L"Loading SSD", L"Parsing SSD file...");
	progress.Show();

	SsdData* ssd = nullptr;
	std::list<Sheet*> sheets;
	bool loadSuccess = false;
	std::wstring errorMsg;

	progress.RunTask([&]() {
		try
		{
			ssd = new SsdData(path, u8"");
			sheets = ssd->GetAllSheets();
			loadSuccess = true;
		}
		catch (const std::exception& ex)
		{
			std::string err(ex.what());
			errorMsg = std::wstring(err.begin(), err.end());
			if (ssd)
			{
				delete ssd;
				ssd = nullptr;
			}
		}
	});

	progress.WaitForCompletion();

	if (!loadSuccess)
	{
		MessageBoxW(m_hwnd, (L"Failed to parse SSD file:\n" + errorMsg).c_str(),
			L"Error", MB_OK | MB_ICONERROR);
		return;
	}

	if (sheets.empty())
	{
		MessageBoxW(m_hwnd, L"No sheets found in this SSD file.",
			L"SSD Info", MB_OK | MB_ICONINFORMATION);
		if (ssd) delete ssd;
		return;
	}

	Sheet* firstSheet = sheets.front();
	if (firstSheet)
	{
		m_dataTableView->LoadSheet(firstSheet);
		m_contentPanel->SetActiveTab(m_tabTable);
		SetStatusText(L"Loaded SSD: " + path);
	}
}

void MainWindow::OpenSqwtFile(const std::wstring& path)
{
	try
	{
		SqwtFile sqwt(path);
		const uint8_t* decrypted = static_cast<const uint8_t*>(sqwt.m_fileContent.GetData());
		int decryptedSize = sqwt.m_fileContent.GetLength();

		if (decrypted && decryptedSize > 0)
		{
			// Check if decrypted content is XML
			std::string str(reinterpret_cast<const char*>(decrypted), decryptedSize);
			m_textView->LoadBytes(decrypted, decryptedSize);
			m_contentPanel->SetActiveTab(m_tabText);
			SetStatusText(L"Decrypted SQWT: " + path);
		}
		else
		{
			MessageBoxW(m_hwnd, L"Failed to decrypt SQWT file.",
				L"Error", MB_OK | MB_ICONERROR);
		}
	}
	catch (const std::exception& ex)
	{
		std::wstring err(ex.what(), ex.what() + strlen(ex.what()));
		MessageBoxW(m_hwnd, (L"Failed to load SQWT file:\n" + err).c_str(),
			L"Error", MB_OK | MB_ICONERROR);
	}
}

void MainWindow::OpenPatchFile(const std::wstring& path)
{
	// Patch parsing will be implemented in PatchSummaryView
	// For now, show a placeholder notification
	std::wstring msg = L"Patch file detected:\n" + path +
		L"\n\nFull patch viewing and application will be available\n"
		L"when PatchSummaryView is implemented.";
	MessageBoxW(m_hwnd, msg.c_str(), L"ZiPatch", MB_OK | MB_ICONINFORMATION);
	SetStatusText(L"Patch loaded: " + path);
}

void MainWindow::SetStatusText(const std::wstring& text)
{
	if (m_hwndStatus)
		SendMessage(m_hwndStatus, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(text.c_str()));
}

void MainWindow::OnDestroy()
{
}

void MainWindow::OnSplitterDrag(int mouseY)
{
	// Future: splitter dragging support
}
