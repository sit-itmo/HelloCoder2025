#include "Skybound.h"

Skybound* Skybound::pObject = nullptr;

Skybound* Skybound::getSingleton()
{
    if (pObject == nullptr)
    {
        pObject = new Skybound();
    }
    return pObject;
}


void WIN_InitConsole(void);

void Skybound::InitConsoleAndTest()
{
    WIN_InitConsole();
    printf("\n[Skybound Hello!]\n");
}


