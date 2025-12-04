#define SKY_MODULE "MAIN"

#include <windows.h>
#include "Skybound.h"

int Dump_RunTest();
int WINAPI WinMain_old(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    WinMain_old(hInstance, NULL, NULL, nCmdShow);
    return 0;
    SKY_PRINTLN("Start system");
    SKY_PRINTLN("Testing log system...");
    SKY_PRINT("[");
    for (int i = 0; i < 10; i++)
    {
        SKY_PRINT_EX("%d, ", i);
    }
    SKY_PRINT("]");

    SKY_DEBUG("This is a debug message");
    SKY_DEBUG_EX("Debug with formatting %d, 0x%x!", 10, 10);
    SKY_ERROR("This is ERROR message");
    SKY_ERROR_EX("This is ERROR message with formatting (%d)", ENOMEM);
    SKY_TRACE("Trace message");
    SKY_TRACE_EX("Trace message again format 0x%llx", __rdtsc());
    SKY_WARNING("Very important warning!");
    SKY_WARNING("Warining with %d %s formatting keep as is!");
    SKY_WARNING_EX("Very important warning %s!", "yesyes");
    
    for (int i = 0; i < 10; i++)
    {
        SKY_ASSERT(i < 9);
    }
    Dump_RunTest();

    Skybound::getSingleton()->Start();
    return 0;
}
