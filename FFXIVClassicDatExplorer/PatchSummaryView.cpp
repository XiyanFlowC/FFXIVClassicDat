#include "PatchSummaryView.h"

#include "../FFXIVClassicDat/ZiPatch.h"
#include "../FFXIVClassicDatConsole/Config.h"
#include "ProgressDialog.h"
#include <filesystem>

#define IDC_PATCH_LIST    100
#define IDC_APPLY_BUTTON  101
#define IDC_PATCH_INFO    102

bool PatchSummaryView::s_registered = false;

const wchar_t* PatchSummaryView::ClassName()
{
	return L"FFXIVClassicDatExplorer_PatchSummaryView";
}

PatchSummaryView::PatchSummaryView() = default;
PatchSummaryView::~PatchSummaryView() = default;

bool PatchSummaryView::Create(HWND parent, int x, int y, int w, int h)
{
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

	m_hwnd = CreateWindowExW(0, ClassName(), L"PatchSummary",
		WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
		x, y, w, h, parent, nullptr, GetModuleHandle(nullptr), this);

	if (!m_hwnd) return false;

	m_hwndInfo = CreateWindowExW(0, L"STATIC", L"",
		WS_CHILD | WS_VISIBLE | SS_LEFT,
		4, 4, w - 8, 40, m_hwnd, (HMENU)IDC_PATCH_INFO, GetModuleHandle(nullptr), nullptr);

	HFONT font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage(m_hwndInfo, WM_SETFONT, (WPARAM)font, TRUE);

	m_hwndList = CreateWindowExW(0, WC_LISTVIEWW, L"",
		WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | WS_BORDER,
		4, 48, w - 8, h - 80, m_hwnd, (HMENU)IDC_PATCH_LIST, GetModuleHandle(nullptr), nullptr);

	SendMessage(m_hwndList, WM_SETFONT, (WPARAM)font, TRUE);
	ListView_SetExtendedListViewStyle(m_hwndList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	LVCOLUMNW col = {};
	col.mask = LVCF_TEXT | LVCF_WIDTH;
	col.pszText = const_cast<LPWSTR>(L"Path");
	col.cx = 300;
	ListView_InsertColumn(m_hwndList, 0, &col);

	col.pszText = const_cast<LPWSTR>(L"Mode");
	col.cx = 60;
	ListView_InsertColumn(m_hwndList, 1, &col);

	col.pszText = const_cast<LPWSTR>(L"Chunks");
	col.cx = 60;
	ListView_InsertColumn(m_hwndList, 2, &col);

	col.pszText = const_cast<LPWSTR>(L"Comp. Size");
	col.cx = 80;
	ListView_InsertColumn(m_hwndList, 3, &col);

	col.pszText = const_cast<LPWSTR>(L"Size");
	col.cx = 80;
	ListView_InsertColumn(m_hwndList, 4, &col);

	m_hwndApply = CreateWindowExW(0, L"BUTTON", L"Apply Patch...",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		w - 130, h - 28, 120, 24, m_hwnd, (HMENU)IDC_APPLY_BUTTON,
		GetModuleHandle(nullptr), nullptr);

	SendMessage(m_hwndApply, WM_SETFONT, (WPARAM)font, TRUE);

	return true;
}

void PatchSummaryView::LoadPatch(const std::wstring& path)
{
	m_patchPath = path;
	Clear();

	// Show marquee progress dialog during patch parsing (unknown progress)
	ProgressDialog progress(m_hwnd, L"Loading Patch", L"Parsing patch file, please wait...", true);
	progress.Show();

	std::unique_ptr<ZiPatchFile> tempPatch;
	bool loadSuccess = false;
	std::wstring errorMsg;

	progress.RunTask([&]() {
		try
		{
			// Parse patch file (this may take time due to CRC checking and SHA1 calculations)
			tempPatch = std::make_unique<ZiPatchFile>(path);
			loadSuccess = true;
		}
		catch (const std::exception& ex)
		{
			std::string err(ex.what());
			errorMsg = std::wstring(err.begin(), err.end());
		}
	});

	progress.WaitForCompletion();

	if (!loadSuccess)
	{
		SetWindowTextW(m_hwndInfo, (L"Error: " + errorMsg).c_str());
		return;
	}

	// Transfer ownership to member variable
	m_patch = std::move(tempPatch);

	// Populate list asynchronously to avoid UI freeze
	PopulateListAsync();
}

void PatchSummaryView::PopulateListAsync()
{
	if (!m_patch) return;

	// Show marquee progress dialog during list population (unknown progress)
	ProgressDialog progress(m_hwnd, L"Loading Patch", L"Building file list, please wait...", true);
	progress.Show();

	bool populateSuccess = false;

	progress.RunTask([this, &populateSuccess]() {
		// Give the progress dialog time to show
		Sleep(100);
		populateSuccess = true;
	});

	progress.WaitForCompletion();

	if (populateSuccess)
	{
		// Populate list on main thread
		PopulateList();
	}
}

void PatchSummaryView::Clear()
{
	m_patch.reset();
	ListView_DeleteAllItems(m_hwndList);
	SetWindowTextW(m_hwndInfo, L"No patch loaded.");
}

void PatchSummaryView::Show()
{
	if (m_hwnd) ShowWindow(m_hwnd, SW_SHOW);
}

void PatchSummaryView::Hide()
{
	if (m_hwnd) ShowWindow(m_hwnd, SW_HIDE);
}

void PatchSummaryView::Resize(int w, int h)
{
	if (m_hwnd)
	{
		SetWindowPos(m_hwnd, nullptr, 0, 0, w, h, SWP_NOZORDER | SWP_NOMOVE);
		OnSize(w, h);
	}
}

void PatchSummaryView::PopulateList()
{
	if (!m_patch) return;

	ListView_DeleteAllItems(m_hwndList);

	wchar_t info[512];
	swprintf_s(info, L"FHDR v%d | Result: %S | %u entries | %u add dirs | %u del dirs | %zu blocks",
		m_patch->Version(), m_patch->Result().c_str(),
		m_patch->DeclaredEntries(), m_patch->DeclaredAdddir(),
		m_patch->DeclaredDeldir(), m_patch->BlockCount());
	SetWindowTextW(m_hwndInfo, info);

	// Disable list view updates to improve performance
	SendMessage(m_hwndList, WM_SETREDRAW, FALSE, 0);

	size_t blockCount = m_patch->Blocks().size();
	size_t processedCount = 0;

	for (auto& block : m_patch->Blocks())
	{
		if (block.IsEtry())
		{
			std::string path = block.Path();

			LVITEMW item = {};
			item.mask = LVIF_TEXT;
			item.iItem = ListView_GetItemCount(m_hwndList);

			std::wstring wpath(path.begin(), path.end());
			item.pszText = const_cast<LPWSTR>(wpath.c_str());
			int idx = ListView_InsertItem(m_hwndList, &item);

			auto chunks = block.Chunks();
			std::wstring mode;
			uint32_t compSize = 0;
			uint32_t totalSize = 0;

			if (!chunks.empty())
			{
				switch (chunks[0].mode)
				{
				case CHUNK_MODE_ADD:    mode = L"Add"; break;
				case CHUNK_MODE_DELETE: mode = L"Delete"; break;
				case CHUNK_MODE_MODIFY: mode = L"Modify"; break;
				default:                mode = L"?"; break;
				}
				compSize = chunks[0].compressed_size;
				totalSize = chunks[0].next_size;
			}

			ListView_SetItemText(m_hwndList, idx, 1, const_cast<LPWSTR>(mode.c_str()));

			wchar_t buf[32];
			swprintf_s(buf, L"%zu", chunks.size());
			ListView_SetItemText(m_hwndList, idx, 2, buf);

			swprintf_s(buf, L"%u", compSize);
			ListView_SetItemText(m_hwndList, idx, 3, buf);

			swprintf_s(buf, L"%u", totalSize);
			ListView_SetItemText(m_hwndList, idx, 4, buf);
		}
		else if (block.IsAdir())
		{
			std::string path = block.Path();
			LVITEMW item = {};
			item.mask = LVIF_TEXT;
			item.iItem = ListView_GetItemCount(m_hwndList);

			std::wstring wpath = L"[DIR] " + std::wstring(path.begin(), path.end());
			item.pszText = const_cast<LPWSTR>(wpath.c_str());
			int idx = ListView_InsertItem(m_hwndList, &item);
			ListView_SetItemText(m_hwndList, idx, 1, const_cast<LPWSTR>(L"Create"));
		}
		else if (block.IsDeld())
		{
			std::string path = block.Path();
			LVITEMW item = {};
			item.mask = LVIF_TEXT;
			item.iItem = ListView_GetItemCount(m_hwndList);

			std::wstring wpath = L"[DIR] " + std::wstring(path.begin(), path.end());
			item.pszText = const_cast<LPWSTR>(wpath.c_str());
			int idx = ListView_InsertItem(m_hwndList, &item);
			ListView_SetItemText(m_hwndList, idx, 1, const_cast<LPWSTR>(L"Delete"));
		}

		// Process messages periodically to keep UI responsive
		processedCount++;
		if (processedCount % 100 == 0)
		{
			MSG msg;
			while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	// Re-enable list view updates
	SendMessage(m_hwndList, WM_SETREDRAW, TRUE, 0);
	InvalidateRect(m_hwndList, nullptr, TRUE);
}

void PatchSummaryView::ApplyPatch()
{
	if (!m_patch)
	{
		MessageBoxW(m_hwnd, L"No patch loaded.", L"Error", MB_OK | MB_ICONERROR);
		return;
	}

	std::wstring installPath = Config::GetInstance().m_ffxivInstallPath;

	// Check if installation directory exists
	if (!std::filesystem::exists(installPath))
	{
		std::wstring msg = L"Installation directory does not exist:\n" + installPath +
			L"\n\nPlease verify the FFXIV installation path in the configuration.";
		MessageBoxW(m_hwnd, msg.c_str(), L"Directory Not Found", MB_OK | MB_ICONERROR);
		return;
	}

	// Show confirmation dialog
	std::wstring confirmMsg = L"You are about to apply the patch to:\n" + installPath +
		L"\n\nThis will modify game files. It is recommended to backup your game files first.\n\n" +
		L"Patch information:\n" +
		L"  Version: " + std::to_wstring(m_patch->Version()) + L"\n" +
		L"  Files to modify: " + std::to_wstring(m_patch->DeclaredEntries()) + L"\n" +
		L"  Directories to add: " + std::to_wstring(m_patch->DeclaredAdddir()) + L"\n" +
		L"  Directories to delete: " + std::to_wstring(m_patch->DeclaredDeldir()) + L"\n\n" +
		L"Do you want to continue?";

	int result = MessageBoxW(m_hwnd, confirmMsg.c_str(), L"Confirm Patch Application",
		MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);

	if (result != IDYES)
	{
		return;
	}

	// Apply patch with progress dialog
	ProgressDialog progress(m_hwnd, L"Applying Patch", L"Preparing to apply patch...");
	progress.Show();

	ApplyResult applyResult;
	bool hasException = false;
	std::wstring exceptionMsg;

	progress.RunTask([this, &installPath, &applyResult, &progress, &hasException, &exceptionMsg]() {
		try {
			ZiPatchApplier applier(*m_patch);

			applyResult = applier.Apply(installPath, false, false, L"",
				[&progress](size_t current, size_t total) {
					if (progress.IsCancelled()) {
						throw std::runtime_error("Operation cancelled by user");
					}
					std::wstring msg = L"Processing: " + std::to_wstring(current) +
						L" / " + std::to_wstring(total);
					progress.SetMessage(msg);
					progress.SetProgress(current, total);
				});
		}
		catch (const std::exception& ex) {
			hasException = true;
			std::string err(ex.what());
			exceptionMsg = std::wstring(err.begin(), err.end());
		}
	});

	progress.WaitForCompletion();

	// Show result
	if (hasException)
	{
		std::wstring msg = L"Patch application failed:\n\n" + exceptionMsg;
		MessageBoxW(m_hwnd, msg.c_str(), L"Apply Error", MB_OK | MB_ICONERROR);
		return;
	}

	std::wstring msg;
	if (applyResult.Ok())
	{
		msg = L"Patch applied successfully!\n\n";
	}
	else
	{
		msg = L"Patch applied with errors.\n\n";
		int errorCount = 0;
		for (auto& err : applyResult.errors)
		{
			if (errorCount < 10) {
				msg += std::wstring(err.begin(), err.end()) + L"\n";
			}
			errorCount++;
		}
		if (errorCount > 10) {
			msg += L"... and " + std::to_wstring(errorCount - 10) + L" more errors\n";
		}
		msg += L"\n";
	}

	msg += L"Files added: " + std::to_wstring(applyResult.files_added) + L"\n";
	msg += L"Files modified: " + std::to_wstring(applyResult.files_modified) + L"\n";
	msg += L"Files deleted: " + std::to_wstring(applyResult.files_deleted) + L"\n";
	msg += L"Directories created: " + std::to_wstring(applyResult.dirs_created) + L"\n";
	msg += L"Directories deleted: " + std::to_wstring(applyResult.dirs_deleted) + L"\n";
	msg += L"Bytes written: " + std::to_wstring(applyResult.bytes_written);

	MessageBoxW(m_hwnd, msg.c_str(), L"Apply Result", 
		applyResult.Ok() ? MB_OK | MB_ICONINFORMATION : MB_OK | MB_ICONWARNING);
}

void PatchSummaryView::OnSize(int w, int h)
{
	if (m_hwndInfo)
		SetWindowPos(m_hwndInfo, nullptr, 4, 4, w - 8, 40, SWP_NOZORDER);

	if (m_hwndList)
		SetWindowPos(m_hwndList, nullptr, 4, 48, w - 8, h - 80, SWP_NOZORDER);

	if (m_hwndApply)
		SetWindowPos(m_hwndApply, nullptr, w - 130, h - 28, 120, 24, SWP_NOZORDER);
}

void PatchSummaryView::OnCommand(int id)
{
	if (id == IDC_APPLY_BUTTON)
	{
		ApplyPatch();
	}
}

LRESULT CALLBACK PatchSummaryView::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	PatchSummaryView* view = nullptr;

	if (msg == WM_NCCREATE)
	{
		auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
		view = static_cast<PatchSummaryView*>(cs->lpCreateParams);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(view));
		view->m_hwnd = hwnd;
	}
	else
	{
		view = reinterpret_cast<PatchSummaryView*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
	}

	if (view)
		return view->HandleMessage(msg, wParam, lParam);

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT PatchSummaryView::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_SIZE:
		OnSize(LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_COMMAND:
		OnCommand(LOWORD(wParam));
		break;

	default:
		return DefWindowProc(m_hwnd, msg, wParam, lParam);
	}
	return 0;
}
