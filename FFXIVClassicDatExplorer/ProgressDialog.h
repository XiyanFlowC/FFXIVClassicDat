#pragma once

#include <windows.h>
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <mutex>

class ProgressDialog
{
public:
    ProgressDialog(HWND parent, const std::wstring& title, const std::wstring& message, bool showMarquee = false);
    ~ProgressDialog();

    void Show();
    void Close();
    void SetMessage(const std::wstring& message);
    void SetProgress(size_t current, size_t total);
    bool IsCancelled() const { return m_cancelled; }

    template<typename Func>
    void RunTask(Func&& task)
    {
        m_taskThread = std::thread([this, task = std::forward<Func>(task)]() {
            try {
                task();
            }
            catch (...) {
                m_taskException = std::current_exception();
            }
            PostMessage(m_hwnd, WM_USER + 100, 0, 0);
        });
    }

    void WaitForCompletion();
    void RethrowIfException();

private:
    static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void OnInitDialog();
    void OnCommand(int id);
    void OnTaskComplete();
    void UpdateMessageInternal();
    void UpdateProgressInternal();

    HWND m_parent = nullptr;
    HWND m_hwnd = nullptr;
    HWND m_hwndProgress = nullptr;
    HWND m_hwndMessage = nullptr;
    std::wstring m_title;
    std::wstring m_message;
    std::mutex m_messageMutex;
    std::atomic<int> m_progressPercent{ 0 };
    std::atomic<bool> m_cancelled{ false };
    std::thread m_taskThread;
    std::exception_ptr m_taskException;
    bool m_showMarquee;

    static bool s_registered;
};
