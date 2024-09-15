#include "RunAsAdmin.h"
#include <Windows.h>
void RunAsAdmin::Execute()
{
    wchar_t pwd[MAX_PATH];
    GetCurrentDirectoryW(MAX_PATH, pwd);
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    HINSTANCE res;
    ShellExecuteW(NULL, L"runas", exePath, GetCommandLineW(), pwd, SW_SHOWNORMAL);
}
