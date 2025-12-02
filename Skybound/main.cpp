#include <windows.h>
#include "Skybound.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    Skybound::getSingleton()->InitConsoleAndTest();
    return 0;
}
