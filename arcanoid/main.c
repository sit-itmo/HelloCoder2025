#include <windows.h>
#include <stdio.h>
#include "arcanoid.h"
#include "win_frontend.h"

int APIENTRY wWinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR lpCmdLine, int nCmdShow) 
{
    int w = 1024;
    int h = 768;
    WCHAR game_name[64] = { 0 };

    _snwprintf(game_name, (sizeof(game_name) / sizeof(game_name[0])),
        L"Arcanoid v0.1 %S %S", __DATE__, __TIME__);
#ifdef _DEBUG
    WIN_InitConsole();
#endif

    AG_PRINTLN("Start Game %d x %d!",  w, h);

    HWND hwnd = WIN_WindowCreate(w, h, game_name);
    if (!hwnd)
    {
        AG_PRINTLN("\nError: failed to create window!");
        return 0;
    }
    AG_PRINTLN("Main window was created!");
    WIN_MainLoop(hwnd);
    AG_PRINTLN("Completed!");
    return 0;
}
