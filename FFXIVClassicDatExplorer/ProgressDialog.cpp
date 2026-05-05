#include "ProgressDialog.h"
#include <commctrl.h>

#define IDC_PROGRESS_BAR    1001
#define IDC_PROGRESS_MSG    1002
#define IDC_CANCEL_BTN      IDCANCEL
#define WM_UPDATE_MESSAGE   (WM_USER + 101)
#define WM_UPDATE_PROGRESS  (WM_USER + 102)

bool ProgressDialog::s_registered = false;

ProgressDialog::ProgressDialog(HWND parent, const std::wstring& title, const std::wstring& message, bool showMarquee)
    : m_parent(parent), m_title(title), m_message(message), m_showMarquee(showMarquee)
{
}

ProgressDialog::~ProgressDialog()
{
    Close();
    if (m_taskThread.joinable()) {
        m_taskThread.join();
    }
}

void ProgressDialog::Show()
{
    if (!s_registered)
    {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = DefDlgProcW;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszClassName = L"ProgressDialogClass";
        RegisterClassExW(&wc);
        s_registered = true;
    }

    m_hwnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        L"#32770",
        m_title.c_str(),
        WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | DS_CENTER,
        0, 0, 400, 150,
        m_parent,
        nullptr,
        GetModuleHandle(nullptr),
        nullptr
    );

    if (!m_hwnd) return;

    SetWindowLongPtr(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    SetWindowLongPtr(m_hwnd, DWLP_DLGPROC, reinterpret_cast<LONG_PTR>(DialogProc));

    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    m_hwndMessage = CreateWindowExW(0, L"STATIC", m_message.c_str(),
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        20, 20, 360, 30, m_hwnd, (HMENU)IDC_PROGRESS_MSG, GetModuleHandle(nullptr), nullptr);
    SendMessage(m_hwndMessage, WM_SETFONT, (WPARAM)hFont, TRUE);

    DWORD progressStyle = WS_CHILD | WS_VISIBLE;
    if (m_showMarquee)
    {
        progressStyle |= PBS_MARQUEE;
    }
    else
    {
        progressStyle |= PBS_SMOOTH;
    }

    m_hwndProgress = CreateWindowExW(0, PROGRESS_CLASSW, nullptr,
        progressStyle,
        20, 60, 360, 25, m_hwnd, (HMENU)IDC_PROGRESS_BAR, GetModuleHandle(nullptr), nullptr);

    CreateWindowExW(0, L"BUTTON", L"Cancel",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        160, 95, 80, 25, m_hwnd, (HMENU)IDC_CANCEL_BTN, GetModuleHandle(nullptr), nullptr);

    if (m_showMarquee)
    {
        // Enable marquee mode with 30ms animation interval
        SendMessage(m_hwndProgress, PBM_SETMARQUEE, TRUE, 30);
    }
    else
    {
        SendMessage(m_hwndProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
        SendMessage(m_hwndProgress, PBM_SETPOS, 0, 0);
    }

    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);

    EnableWindow(m_parent, FALSE);
}

void ProgressDialog::Close()
{
    if (m_hwnd)
    {
        EnableWindow(m_parent, TRUE);
        SetForegroundWindow(m_parent);
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

void ProgressDialog::SetMessage(const std::wstring& message)
{
    {
        std::lock_guard<std::mutex> lock(m_messageMutex);
        m_message = message;
    }
    if (m_hwnd)
    {
        PostMessage(m_hwnd, WM_UPDATE_MESSAGE, 0, 0);
    }
}

void ProgressDialog::SetProgress(size_t current, size_t total)
{
    if (total > 0)
    {
        int percent = static_cast<int>((current * 100) / total);
        m_progressPercent.store(percent);
        if (m_hwnd)
        {
            PostMessage(m_hwnd, WM_UPDATE_PROGRESS, 0, 0);
        }
    }
}

void ProgressDialog::UpdateMessageInternal()
{
    if (m_hwndMessage)
    {
        std::wstring msg;
        {
            std::lock_guard<std::mutex> lock(m_messageMutex);
            msg = m_message;
        }
        SetWindowTextW(m_hwndMessage, msg.c_str());
    }
}

void ProgressDialog::UpdateProgressInternal()
{
    if (m_hwndProgress)
    {
        int percent = m_progressPercent.load();
        SendMessage(m_hwndProgress, PBM_SETPOS, percent, 0);
    }
}

void ProgressDialog::WaitForCompletion()
{
    if (!m_hwnd) return;

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (msg.message == WM_USER + 100)
        {
            OnTaskComplete();
            break;
        }

        if (!IsDialogMessage(m_hwnd, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    if (m_taskThread.joinable())
    {
        m_taskThread.join();
    }
}

void ProgressDialog::RethrowIfException()
{
    if (m_taskException)
    {
        std::rethrow_exception(m_taskException);
    }
}

INT_PTR CALLBACK ProgressDialog::DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    ProgressDialog* dlg = reinterpret_cast<ProgressDialog*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (msg)
    {
    case WM_UPDATE_MESSAGE:
        if (dlg)
        {
            dlg->UpdateMessageInternal();
        }
        return TRUE;

    case WM_UPDATE_PROGRESS:
        if (dlg)
        {
            dlg->UpdateProgressInternal();
        }
        return TRUE;

    case WM_COMMAND:
        if (dlg && LOWORD(wParam) == IDC_CANCEL_BTN)
        {
            dlg->m_cancelled = true;
            EnableWindow(GetDlgItem(hwnd, IDC_CANCEL_BTN), FALSE);
            SetWindowTextW(GetDlgItem(hwnd, IDC_PROGRESS_MSG), L"Cancelling...");
        }
        break;

    case WM_CLOSE:
        if (dlg)
        {
            dlg->m_cancelled = true;
        }
        return TRUE;
    }

    return FALSE;
}

void ProgressDialog::OnTaskComplete()
{
    Close();
}
