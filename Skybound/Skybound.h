#pragma once

#include <string>
#include <vector>

// sType stands for Skybound
typedef unsigned int sU32;
typedef unsigned long long sU64;
typedef float sTime;
typedef std::string sID;
typedef std::string sString;
typedef unsigned int sColor;

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
    virtual bool Setup(const sString &caption, const Size2D& size)=0;
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


