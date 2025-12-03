#include "Skybound.h"

Skybound* Skybound::pObject = nullptr;

Skybound* Skybound::getSingleton()
{
    if (pObject == nullptr)
    {
        pObject = new Skybound();
        pObject->pPlatform = Win32PlatformBuilder::Build();
    }
    return pObject;
}


void WIN_InitConsole(void);

void Skybound::Start()
{
    SKY_PRINTLN("[Skybound Hello!]");
    pPlatform->Setup("Skybound!", {800, 600});
    pPlatform->Loop();
}

