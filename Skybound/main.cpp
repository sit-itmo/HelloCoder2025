#define SKY_MODULE "MAIN"

#include <windows.h>
#include "Skybound.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    SKY_PRINTLN("Start system");
    Skybound::getSingleton()->Start();
    return 0;
}
