#pragma once

#include <stdio.h>
#include <string>
#include <vector>

// sType stands for Skybound
typedef unsigned int sU32;
typedef unsigned long long sU64;
typedef float sTime;
typedef std::string sID;
typedef std::string sString;
typedef unsigned int sColor;

#define SKYBOUND_DEFAULT_LOG_FILE "skybound.log"

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

    void PutText(const char* p_text);
    void LogMessage(const char* p_file, int line, const char* p_module, enum Kind kind, const char* p_text);
    void LogMessageEx(const char* p_file, int line, const char* p_module, enum Kind kind, const char* p_text, ...);

private:
    bool _FlushAlways = false;
    bool _TraceEnabled = false;
    bool _PrintModule = true;
    bool _PrintFile = true;
    bool _PrintTime = true;
    bool _Ready = false;
    bool _WriteConsole = true;
    bool _WriteFile = false;
    FILE* pLogFile = nullptr;
   
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
#define _SKY_MESSAGE(kind, a) { Skybound::getSingleton()->Log.LogMessage(__FILE__, __LINE__, SKY_MODULE, kind, a); }
#define _SKY_MESSAGE_EX(kind, a, ...) { Skybound::getSingleton()->Log.LogMessageEx(__FILE__, __LINE__, SKY_MODULE, kind, a, ##__VA_ARGS__); }
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


#include "Formatting.h"
#include "Types2D.h"

typedef unsigned int sColor;

class Picture
{
private:
    Size2D _Size;    
    sColor* pPixels;  

public:
    static sColor AlphaBlend(sColor dst, sColor src);

public:
    Picture();
    Picture(Size2D size);
    Picture(unsigned int w, unsigned int h);
    
    Picture(const Picture& other);
    Picture(Picture&& other) noexcept;
    Picture& operator=(const Picture& other);
    Picture& operator=(Picture&& other) noexcept;
    ~Picture();
    void Resize(Size2D newSize);

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
    void DrawPicture(const Picture& src,
        const Pos2D& srcPos, const Size2D& srcSize,
        const Pos2D& dstPos, const Size2D& dstSize);
    
    inline Size2D GetSize() const { return _Size; }

    inline unsigned int Width() const { return _Size.W; }
    inline unsigned int Height() const { return _Size.H; }

    inline sColor* Data() { return pPixels; }
    inline const sColor* Data() const { return pPixels; }
};

class Gameplay;

class IApplication
{
public:
    virtual bool Update(sTime curTime, sTime prevTime) = 0;
    virtual void Render(Picture* p_buffer) = 0;
};


class Platform
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
    virtual ~Platform() {}
    virtual bool SetupConsole() = 0;
    virtual bool Setup(const sString& caption, const Size2D& size) = 0;
    virtual void Loop()=0;
    virtual bool GetKeyState(KeyCode code)=0;

    void AddApplication(IApplication* p_app);
    void DelApplication(IApplication* p_app);
};

class Skybound
{
private:
    static Skybound* pObject;
    Skybound() {}
public:
    static Skybound* getSingleton();


public:
    void Start();

    Logging Log;

    Platform* GetPlatform() { return pPlatform; }
    Gameplay* GetGameplay() { return pGameplay; }

private:
    Platform* pPlatform = nullptr;
    Gameplay* pGameplay = nullptr;
};


class Gameplay : public IApplication
{
public:
    bool Update(sTime curTime, sTime prevTime);
    void Render(Picture* p_buffer);
};

class Win32PlatformBuilder
{
public:
    static Platform* Build();
};


