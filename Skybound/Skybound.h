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
    void RefsAdd();
    void RefsDel();
    sRefControl() { RefsAdd(); };
    virtual ~sRefControl();
};

struct IPicture
{
    virtual void Clear(sColor color)=0;
    virtual void PutPixel(unsigned int x, unsigned int y, sColor color) = 0;
    virtual sColor GetPixel(unsigned int x, unsigned int y) const = 0;
    virtual void DrawPicture(const IPicture* src,
        const sPos2D& srcPos, const sSize2D& srcSize,
        const sPos2D& dstPos, const sSize2D& dstSize) = 0;
    virtual void DrawRect(const sPos2D& pos, const sSize2D& size, sColor color)=0;
    virtual sSize2D GetSize() const = 0;
    virtual unsigned int Width() const = 0;
    virtual unsigned int Height() const = 0;
    virtual sColor* Data() = 0;
    virtual const sColor* Data() const = 0;
};


class sPicture : public IPicture, public sRefControl
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
    void Clear(sColor color = 0x00000000);
    bool LoadPNG(const char *p_path);

    virtual void PutPixel(unsigned int x, unsigned int y, sColor color);
    virtual sColor GetPixel(unsigned int x, unsigned int y) const;
    virtual void DrawRect(const sPos2D& pos, const sSize2D& size, sColor color);
    virtual void DrawPicture(const IPicture* src,
        const sPos2D& srcPos, const sSize2D& srcSize,
        const sPos2D& dstPos, const sSize2D& dstSize);
    
    virtual sSize2D GetSize() const { return _Size; }

    virtual unsigned int Width() const { return _Size.W; }
    virtual unsigned int Height() const { return _Size.H; }

    virtual sColor* Data() { return pPixels; }
    virtual const sColor* Data() const { return pPixels; }
};

class sSprite
{
public:
    enum Kind
    {
        None,
        Horizontal,
        Vertical
    };
protected:
    sPicture* _pTexture = nullptr;
    sPos2D  _SrcPosition = { 0, 0 };
    sSize2D _SrcSize = { 0, 0 };
    sTime _AnimationStartTime = 0;
    int _AnimationCurrentFrame = 0;
    int _AnimationTotalFrames = 0;
    float _AnimationSpeed = 1;
    enum Kind _Animation = None;
public:
    inline const sPicture* pTexture() const { return _pTexture; }
    inline void setSrcPosition(const sPos2D& v) { _SrcPosition = v; }
    inline const sPos2D& SrcPosition() const { return _SrcPosition; }
    inline void setSrcSize(const sSize2D& v) { _SrcSize = v; }
    inline const sSize2D& SrcSize() const { return _SrcSize; }
    inline sTime     AnimationStartTime() const { return _AnimationStartTime; }
    inline int       AnimationCurrentFrame() const { return  _AnimationCurrentFrame; }
    inline int       AnimationTotalFrames() const { return  _AnimationTotalFrames; }
    inline float AnimationSpeed() const { return _AnimationSpeed; }
    inline enum Kind Animation() const { return  _Animation; }

    sSprite();

    void Init(sPicture* p_pic, const sPos2D& pos, const sSize2D& size,
        enum Kind anim = None, float animSpeed = 1.0f, int animTotal = 0);
    virtual ~sSprite();

    void AnimationStart(sTime curTime);
    void AnimationStop();
    void Update(sTime curTime, sTime dt);
    void Draw(IPicture* p_buffer, const sPos2D& pos, const sSize2D& size);
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
    virtual const sSize2D &ScreenSize() = 0;
    virtual void Loop()=0;
    virtual bool GetKeyState(KeyCode code)=0;
    virtual float GetTime()=0;

    void AddApplication(IApplication* p_app);
    void DelApplication(IApplication* p_app);
};

class sPlatformEmpty : public sPlatform
{
    sSize2D _FakeSize = { 0, 0 };
public:
    ~sPlatformEmpty() {}
    bool SetupConsole() { return false; }
    bool Setup(const sString& caption, const sSize2D& size) { return false; }
    const sSize2D& ScreenSize() { return _FakeSize; }
    void Loop() {}
    bool GetKeyState(KeyCode code) { return false; }
    float GetTime() { return 0.0f; }
};

#include "Gameplay.h"
#include "Profiler.h"

class sGameplay : public IApplication
{
public:
    enum { MAX_LAYERS = 4 };
private:
    std::vector<SkyEntity*> _ToAdd;
    std::vector<SkyEntity*> _ToDel;
    std::vector<SkyEntity*> _Entities[MAX_LAYERS];
    SkyPlayer* _pPlayer = nullptr;
    bool _InsideLoop = false;
    SkyPictureWithCamera _Screen;
    sProfiler::AllSnapshots _PrevSnapshot;
    float _StatusFrameTime = 0;
    std::string _StatusFrameLine = "";
    std::string _StatusStatLine = "";
public:

    inline const sVec2D& Camera() const { return _Screen.Camera; }
    inline const SkyPlayer* pPlayer() const { return _pPlayer; }

    virtual ~sGameplay();

    void AddEntity(SkyEntity* p_obj, int layer = 0);
    void DelEntity(SkyEntity* p_obj, int layer = -1);

    void UpdateCamera();
    bool Init();
    bool Update(sTime curTime, sTime delta);
    bool UpdateGui(sTime curTime, sTime delta);
    void Render(sPicture* p_buffer);
    void RenderGui(sPicture* p_buffer);
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
    float _Size = 0;
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
    sPlatform* pPlatform = new sPlatformEmpty();
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

