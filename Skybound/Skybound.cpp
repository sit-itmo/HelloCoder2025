#include "Skybound.h"

Skybound* Skybound::pObject = nullptr;

Skybound* Skybound::getSingleton()
{
    if (pObject == nullptr)
    {
        pObject = new Skybound();
        pObject->pPlatform = sWin32PlatformBuilder::Build();
    }
    return pObject;
}


void WIN_InitConsole(void);

void Skybound::Start()
{
    SKY_PRINTLN("[Skybound Hello!]");
    pAssets = new sAssetManager();
    pGameplay = new sGameplay();
    pGameplay->Init();
    pPlatform->Setup("Skybound!", {800, 600});
    pPlatform->AddApplication(pGameplay);
    pPlatform->Loop();
}

void sPlatform::AddApplication(IApplication* p_app)
{
    Apps.push_back(p_app);
}

void sPlatform::DelApplication(IApplication* p_app)
{

}

