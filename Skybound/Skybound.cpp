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

void Skybound::Start()
{
    printf("\n[Skybound Hello!]\n");
    pPlatform = Win32PlatformBuilder::Build();
    pPlatform->Setup("Skybound!", {800, 600});
    pPlatform->Loop();
}

