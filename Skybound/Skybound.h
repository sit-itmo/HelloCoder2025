#pragma once

#include <stdio.h>
#include <string>
#include <vector>
#include <map>

// sType stands for Skybound
typedef unsigned int sU32;
typedef unsigned long long sU64;
typedef float sTime;
typedef std::string sID;
typedef std::string sString;
//typedef unsigned int sColor;



#define SKYBOUND_DEFAULT_LOG_FILE "skybound.log"
#define SKYBOUND_SETTINGS_FILE "skybound.json"

#include "Color.h"
#include "Types2D.h"
#include "Settings.h"
#include "Formatting.h"

struct Logging
{
    enum { MaxMessageSize = 256 };
    enum Kind
    {
        Kind_PRINT,
        Kind_PLINE,
        Kind_TRACE,
        Kind_ERROR,
        Kind_DEBUG,
        Kind_WARNI
    };

    inline void SetSettings(const sLogSettings& set) { _Settings = set; }
    
    void PutText(const char* p_text);
    void LogMessage(const char* p_file, int line, const char* p_module, enum Kind kind, const char* p_text);
    void LogMessageEx(const char* p_file, int line, const char* p_module, enum Kind kind, const char* p_text, ...);
    void LogMessagePrefix(const char* p_file, int line, const char* p_module, enum Kind kind);

    static void Demo();

private:
    bool _Ready = false;
    FILE* pLogFile = nullptr;
    sLogSettings _Settings;

    bool Setup();
};

#ifndef SKY_MODULE
#define SKY_MODULE "none"
#endif

#if defined(_WIN32) && defined(_DEBUG)
#define SKY_BREAKPOINT()  __debugbreak();
#elif defined(_DEBUG)
#define SKY_BREAKPOINT()  asm("int $3")
#else
#define SKY_BREAKPOINT() {}
#endif

#ifdef _DEBUG
#define _SKY_MESSAGE(kind, a) { Skybound::getSingleton()->Log().LogMessage(__FILE__, __LINE__, SKY_MODULE, kind, a); }
#define _SKY_MESSAGE_EX(kind, a, ...) { Skybound::getSingleton()->Log().LogMessageEx(__FILE__, __LINE__, SKY_MODULE, kind, a, ##__VA_ARGS__); }
#else
#define _SKY_MESSAGE(kind, a)         { }
#define _SKY_MESSAGE_EX(kind, a, ...) { }
#endif

#define SKY_PRINTLN(a) _SKY_MESSAGE(Logging::Kind::Kind_PLINE, a)
#define SKY_PRINT(a)   _SKY_MESSAGE(Logging::Kind::Kind_PRINT, a)
#define SKY_DEBUG(a)   _SKY_MESSAGE(Logging::Kind::Kind_DEBUG, a)
#define SKY_TRACE(a)   _SKY_MESSAGE(Logging::Kind::Kind_TRACE, a)
#define SKY_ERROR(a) { _SKY_MESSAGE(Logging::Kind::Kind_ERROR, a); SKY_BREAKPOINT(); }
#define SKY_WARNING(a) _SKY_MESSAGE(Logging::Kind::Kind_WARNI, a)

#define SKY_PRINTLN_EX(a, ...) _SKY_MESSAGE_EX(Logging::Kind::Kind_PLINE, a, ##__VA_ARGS__)
#define SKY_PRINT_EX(a, ...)   _SKY_MESSAGE_EX(Logging::Kind::Kind_PRINT, a, ##__VA_ARGS__)
#define SKY_DEBUG_EX(a, ...)   _SKY_MESSAGE_EX(Logging::Kind::Kind_DEBUG, a, ##__VA_ARGS__)
#define SKY_TRACE_EX(a, ...)   _SKY_MESSAGE_EX(Logging::Kind::Kind_TRACE, a, ##__VA_ARGS__)
#define SKY_ERROR_EX(a, ...) { _SKY_MESSAGE_EX(Logging::Kind::Kind_ERROR, a, ##__VA_ARGS__); SKY_BREAKPOINT(); }
#define SKY_WARNING_EX(a, ...) _SKY_MESSAGE_EX(Logging::Kind::Kind_WARNI, a, ##__VA_ARGS__)

#ifdef _DEBUG
#define SKY_ASSERT(a) if (!(a)) { SKY_ERROR(#a); }
#else
#define SKY_ASSERT(a) {}
#endif

class sGameplay;

class sRefControl
{
private:
    sU32 _Reference = 0;

public:
    inline sU32 Refs() const { return _Reference; };
    inline void RefsAdd() { _Reference++; };
    void RefsDel();
    sRefControl() { RefsAdd(); };
    virtual ~sRefControl();
};

class sPicture : public sRefControl
{
private:
    sSize2D _Size;    
    sColor* pPixels;  

public:
    sPicture();
    sPicture(sSize2D size);
    sPicture(unsigned int w, unsigned int h);
    
    sPicture(const sPicture& other);
    sPicture(sPicture&& other) noexcept;
    sPicture& operator=(const sPicture& other);
    sPicture& operator=(sPicture&& other) noexcept;
    ~sPicture();
    void Resize(sSize2D newSize);

    bool LoadPNG(const char *p_path);

    inline void PutPixel(unsigned int x, unsigned int y, sColor color)
    {
        if (!pPixels) return;
        if (x >= _Size.W || y >= _Size.H) return;

        pPixels[y * _Size.W + x] = color;
    }

    inline sColor GetPixel(unsigned int x, unsigned int y) const
    {
        if (!pPixels) return 0;
        if (x >= _Size.W || y >= _Size.H) return 0;

        return pPixels[y * _Size.W + x];
    }

    void Clear(sColor color = 0x00000000);
    void DrawPicture(const sPicture& src,
        const sPos2D& srcPos, const sSize2D& srcSize,
        const sPos2D& dstPos, const sSize2D& dstSize);
    
    inline sSize2D GetSize() const { return _Size; }

    inline unsigned int Width() const { return _Size.W; }
    inline unsigned int Height() const { return _Size.H; }

    inline sColor* Data() { return pPixels; }
    inline const sColor* Data() const { return pPixels; }
};

class IApplication
{
public:
    virtual bool Update(sTime curTime, sTime prevTime) = 0;
    virtual void Render(sPicture* p_buffer) = 0;
};


class sPlatform
{
protected:
    std::vector<IApplication*> Apps;

public:
    enum KeyCode
    {
        KeyCode_Space,
        KeyCode_Right,
        KeyCode_Left,
        KeyCode_Fire
    };
    virtual ~sPlatform() {}
    virtual bool SetupConsole() = 0;
    virtual bool Setup(const sString& caption, const sSize2D& size) = 0;
    virtual void Loop()=0;
    virtual bool GetKeyState(KeyCode code)=0;
    virtual float GetTime()=0;

    void AddApplication(IApplication* p_app);
    void DelApplication(IApplication* p_app);
};

#include "Gameplay.h"
#include "Profiler.h"

class sGameplay : public IApplication
{
public:
    enum { MAX_LAYERS = 4 };
private:
    std::vector<SkyEntity*> _Entities[MAX_LAYERS];
    sPos2D _Camera = { 0, 0 };
    SkyPlayer* _pPlayer = nullptr;

public:
    inline const sPos2D& Camera() const { return _Camera; }
    inline const SkyPlayer* pPlayer() const { return _pPlayer; }

    virtual ~sGameplay();

    void AddEntity(SkyEntity* p_obj, int layer=0);


    bool Init();
    bool Update(sTime curTime, sTime delta);
    void Render(sPicture* p_buffer);
};

struct sGlyph
{
    int width;
    int height;
    int bearingX;
    int bearingY;
    int advance;
    std::vector<unsigned char> bitmap;
};

class sFont : public sRefControl
{
protected:
    std::map<sU32, sGlyph> _Chars;
    float _Size;
    sFont();

    virtual bool LoadGlyph(uint32_t codepoint, sGlyph& out)=0;

public:
    virtual ~sFont() {}
    
    int DrawGlyph(uint32_t codepoint, sPicture& pict, sPos2D& pos, sColor c);
    static sFont *LoadFromFile(const char *p_fileName, float size);
    void PrintText(sPicture &pic, const sPos2D &loc, sColor color, const std::string& text);
};


class sWin32PlatformBuilder
{
public:
    static sPlatform* Build();
};

class sAssetManager
{
private:
    std::map<sID, sFont*> _Fonts;
    std::map<sID, sPicture*> _Pictures;
    
public:
    ~sAssetManager();

    sFont* getFont(sID id);
    sPicture* getPicture(sID id);

    sFont* addFont(sID id, const char* p_fileName, float size);
    sPicture* addPicture(sID id, const char* p_path);


};

class Skybound
{
private:
    static Skybound* pObject;
    Skybound() {}
public:
    static Skybound* getSingleton();
    static void DESTROY();

public:
    ~Skybound();

    void Start();


    void SetCurrentLevel(SkyLevel* val) {
        if (pCurrentLevel != nullptr) { delete pCurrentLevel; } 
        pCurrentLevel = val;
    }

    inline Logging& Log() { return _Log; }
    inline sProfiler& Profiler() { return _Profiler; }
    inline sSettings& Settings() { return _Settings; }

    inline SkyLevel* CurLevel() { return pCurrentLevel; }
    inline sPlatform& Platform() { return *pPlatform; }
    inline sGameplay& Gameplay() { return *pGameplay; }
    inline sAssetManager& Assets() { return *pAssets; }

private:
    Logging _Log;
    sProfiler _Profiler;
    sSettings _Settings;
    
    SkyLevel* pCurrentLevel = nullptr;
    sPlatform* pPlatform = nullptr;
    sGameplay* pGameplay = nullptr;
    sAssetManager* pAssets = nullptr;
};

#define SKY_LEVEL() Skybound::getSingleton()->CurLevel()
#define SKY_PROFILER() Skybound::getSingleton()->Profiler()
#define SKY_ASSETS() Skybound::getSingleton()->Assets()
#define SKY_GAMEPLAY() Skybound::getSingleton()->Gameplay()
#define SKY_PLATFORM() Skybound::getSingleton()->Platform()

class sProfScope
{
public:
    sProfScope(sID id) : _id(id) { SKY_PROFILER().Begin(_id); }
    ~sProfScope() { SKY_PROFILER().End(_id); }
private:
    sID _id;
};

#define SKY_PROFSCOPE(a) sProfScope(a)

