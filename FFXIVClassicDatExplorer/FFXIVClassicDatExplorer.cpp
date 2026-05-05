#include "framework.h"
#include "FFXIVClassicDatExplorer.h"
#include "MainWindow.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    MainWindow app(hInstance);
    if (!app.Init(nCmdShow))
    {
        return FALSE;
    }

    return app.Run();
}
