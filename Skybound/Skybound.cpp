#include <fstream>
#include <sstream>

#include "Skybound.h"

sRefControl::~sRefControl()
{
    RefsDel();
    SKY_ASSERT(_Reference == 0);
}

void sRefControl::RefsAdd()
{
    SKY_TRACE_EX("REF++ 0x%x", this);
    _Reference++;
};

void sRefControl::RefsDel()
{
    SKY_TRACE_EX("REF-- 0x%x", this);
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
    if (pPlatform != nullptr)
    {
        delete pPlatform;
        pPlatform = nullptr;
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
    _Log.SetSettings(_Settings.LogSettings);
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

sSprite::sSprite()
{

}

void sSprite::Init(sPicture* p_pic, const sPos2D& pos, const sSize2D& size, enum Kind anim, float animSpeed, int animTotal)
{
    _pTexture = p_pic;
    if (_pTexture != nullptr)
    {
        _pTexture->RefsAdd();
    }
    _SrcPosition = pos;
    _SrcSize = size;
    _Animation = anim;
    _AnimationSpeed = animSpeed;
    _AnimationCurrentFrame = 0;
    _AnimationStartTime = 0;
    _AnimationTotalFrames = animTotal;
}

sSprite::~sSprite()
{
    if (_pTexture)
    {
        _pTexture->RefsDel();
        _pTexture = nullptr;
    }
}

void sSprite::AnimationStart(sTime curTime)
{
    _AnimationCurrentFrame = 0;
    _AnimationStartTime = curTime;
}
void sSprite::AnimationStop()
{
    _AnimationCurrentFrame = 0;
    _AnimationStartTime = 0;
}

void sSprite::Update(sTime curTime, sTime dt)
{
    if (_AnimationStartTime != 0)
    {
        _AnimationCurrentFrame = (int)((curTime - _AnimationStartTime) / _AnimationSpeed) % _AnimationTotalFrames;
    }
}

void sSprite::Draw(IPicture* p_buffer, const sPos2D& pos, const sSize2D& size)
{
    if (p_buffer == nullptr || _pTexture == nullptr)
    {
        return;
    }
    switch (_Animation)
    {
    case None:
        p_buffer->DrawPicture(_pTexture, _SrcPosition, _SrcSize, pos, size);
        break;
    case Horizontal:
        p_buffer->DrawPicture(_pTexture,
            _SrcPosition + sPos2D(_SrcSize.W * _AnimationCurrentFrame, 0),
            _SrcSize, pos, size);
        break;
    case Vertical:
        p_buffer->DrawPicture(_pTexture,
            _SrcPosition + sPos2D(0, _SrcSize.H * _AnimationCurrentFrame),
            _SrcSize, pos, size);
        break;
    }
}

