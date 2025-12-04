#define SKY_MODULE "MAIN"

#include <windows.h>
#include "Skybound.h"

int Dump_RunTest();
int WINAPI WinMain_old(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    SKY_PRINTLN("Start system");

    //Dump_RunTest();
    //Logging::Demo();

    //WinMain_old(hInstance, NULL, NULL, nCmdShow);
    
    Skybound::getSingleton()->Start();
    return 0;
}
