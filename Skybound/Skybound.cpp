#include <fstream>
#include <sstream>

#include "Skybound.h"

sRefControl::~sRefControl()
{
    RefsDel();
    SKY_ASSERT(_Reference == 0);
}

void sRefControl::RefsDel()
{
    SKY_ASSERT(_Reference != 0);
    _Reference--;
};


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

Skybound::~Skybound()
{
    _Settings.Save(SKYBOUND_SETTINGS_FILE);
    if (pPlatform != nullptr)
    {
        delete pPlatform;
        pPlatform = nullptr;
    }
    if (pGameplay != nullptr)
    {
        delete pGameplay;
        pGameplay = nullptr;
    }
    if (pAssets != nullptr)
    {
        delete pAssets;
        pAssets = nullptr;
    }
    //SkyLevel* pCurrentLevel = nullptr;
}

void Skybound::DESTROY()
{
    if (pObject != nullptr)
    {
        delete pObject;
        pObject = nullptr;
    }
}

void WIN_InitConsole(void);

void Skybound::Start()
{
    SKY_PRINTLN("[Skybound Hello!]");
    _Settings.Load(SKYBOUND_SETTINGS_FILE);
    
    SKY_PRINTLN("Loading...");
    pAssets = new sAssetManager();
    pGameplay = new sGameplay();
    pGameplay->Init();
    pPlatform->Setup("Skybound!", _Settings.ScreenSize);
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

#include "ThorSerialize/JsonThor.h"
#include "ThorSerialize/SerUtil.h"

ThorsAnvil_MakeTrait(sSize2D, W, H);
ThorsAnvil_MakeTrait(sLogSettings, FlushAlways, TraceEnabled, PrintModule, PrintFile, PrintTime, WriteConsole, WriteFile, LogFileName);
ThorsAnvil_MakeTrait(sSettings, ScreenSize, FullScreen, MainTileSize, Level, WindowCaption, AllowLogs, LogSettings);

bool sSettings::Save(const char* p_path)
{
    if (p_path == nullptr || p_path[0] == '\0')
    {
        SKY_WARNING("sSettings: No settings file provied!");
        return false;
    }

    std::string str_json_base;
    // Deserialize JSON
    try
    {
        // Serialize to string
        std::stringstream str_json;
        str_json << ThorsAnvil::Serialize::jsonExport(*this) << std::endl;
        str_json_base = str_json.str();
    }
    catch (const std::exception& e)
    {
        SKY_WARNING_EX("sSettings: Failed to serialize data: %s!", e.what());
        return false;
    }


    // Try to save to file
    std::ofstream file(p_path, std::ios::out | std::ios::binary);
    if (!file.is_open())
    {
        SKY_WARNING_EX("sSettings: Failed to open file %s!", p_path);
        return false;
    }

    file.write(str_json_base.data(), (std::streamsize)str_json_base.size());
    if (!file.good())
    {
        SKY_WARNING_EX("sSettings: Failed to write data to file %s!", p_path);
        return false;
    }

    file.close();
    if (!file)
    {
        SKY_WARNING_EX("sSettings: Failed to close file %s!", p_path);
        return false;
    }

    return true;
}

bool sSettings::Load(const char* p_path)
{
    if (p_path == nullptr || p_path[0] == '\0')
    {
        SKY_WARNING("sSettings: No settings file provied!");
        return false;
    }

    // Open file
    std::ifstream file(p_path, std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        SKY_WARNING_EX("sSettings: Failed to open %s file!", p_path);
        return false;
    }

    // Read entire file into a string
    std::stringstream buffer;
    buffer << file.rdbuf();

    if (!file.good())
    {
        SKY_WARNING_EX("sSettings: Failed to read %s file!", p_path);
        return false;
    }

    file.close();
    if (!file)
    {
        SKY_WARNING_EX("sSettings: Failed to close %s file!", p_path);
        return false;
    }

    std::string str_json = buffer.str();
    if (str_json.empty())
    {
        SKY_WARNING_EX("sSettings: No json data in %s file!", p_path);
        return false;
    }

    // Deserialize JSON
    try
    {
        std::istringstream input(str_json);
        input >> ThorsAnvil::Serialize::jsonImport(*this);

        if (!input.good() && !input.eof())
        {
            SKY_WARNING_EX("sSettings: Failed to parse %s file!", p_path);
            return false;
        }
    }
    catch (const std::exception& e)
    {
        SKY_WARNING_EX("sSettings: Failed to parse %s file: %s!", p_path, e.what());
        return false;
    }
    return true;
}
