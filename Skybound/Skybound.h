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


#include "Color.h"

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
    void LogMessagePrefix(const char* p_file, int line, const char* p_module, enum Kind kind);

    static void Demo();

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

class sGameplay;

class sAsset
{
private:
    sU32 _Reference = 0;

public:
    inline sU32 Refs() const { return _Reference; };
    inline void RefsAdd() { _Reference++; };
    void RefsDel();
    sAsset() { RefsAdd(); };
    virtual ~sAsset();
};

class sPicture : public sAsset
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

class SkyEntity
{
protected:
    sVec2D  _Position;
    sSize2D _Size;
    bool    _Enabled;

public:
    inline bool Enabled() const { return _Enabled; }
    inline const sVec2D& Position() const { return _Position; }
    inline const sSize2D& Size() const { return _Size; }

    SkyEntity() : _Position(), _Size(), _Enabled(true) {}

    virtual ~SkyEntity() {}

    virtual void Update(sTime curTime, sTime prevTime) = 0;
    virtual void Draw(sPicture* p_buffer) = 0;
    
    bool Intersects(const sVec2D& otherPos, const sSize2D& otherSize) const;
    bool Intersects(const SkyEntity& other) const;

};

class SkyActiveEntity : public SkyEntity
{
protected:
    sVec2D  _Vector;
    void DoMove(float dt);

public:
    inline bool IsMoving() const { return _Vector.x != 0.0f && _Vector.y != 0.0f; }
    inline void Stop() { _Vector.x = 0.0f; _Vector.y = 0.0f; }
    inline const sVec2D& Vector() const { return _Vector; }

    SkyActiveEntity() : SkyEntity(), _Vector() {}

    virtual ~SkyActiveEntity() {}

};


class SkyCharacterEntity : public SkyActiveEntity
{
protected:
    int _Health = 0;
    int _MaxHealth = 0;
    bool _OnGround = false;
    float _MoveSpeed = 200.0f;
    float _JumpSpeed = -450.0f;
    int   _Direction = 1; // 1 right, -1 left
    float _ShootCooldown = 0.0f;

    void ApplyGravity(float dt);
    void MoveAndCollide(float dt);

    virtual void Reset();

    void CheckHazards();

public:

    inline int Health() const { return _Health; }
    inline int MaxHealth() const { return _MaxHealth; }
    inline bool OnGround() const { return _OnGround; }
    inline float MoveSpeed() const { return _MoveSpeed; }
    inline float JumpSpeed() const { return _JumpSpeed; }
    inline int   Direction() const { return _Direction; }

    SkyCharacterEntity() : SkyActiveEntity() {}

    virtual ~SkyCharacterEntity() {}

};

class SkyTile : public SkyEntity
{
    friend class SkyLevel;

private:
    void Setup(sPos2D pos, sSize2D size);

protected:
    SkyTile() : SkyEntity() {}
    void DrawTileRect(sPicture *p_pic, sColor color);
public:
    ~SkyTile() {};
    virtual void Update(sTime curTime, sTime delta);
    virtual bool IsSolid() const=0;
    virtual bool IsHazard() const=0;
};

class SkyEmptyTile : public SkyTile
{
public:
    virtual void Update(sTime curTime, sTime delta) {}
    virtual void Draw(sPicture* p_buffer) {}
    virtual bool IsSolid() const { return false; }
    virtual bool IsHazard() const { return false; }
};

class SkyGroundTile : public SkyTile
{
public:
    virtual void Draw(sPicture* p_buffer);
    virtual bool IsSolid() const { return true; }
    virtual bool IsHazard() const { return false; }
};

class SkyDecorTile : public SkyTile
{
public:
    virtual void Draw(sPicture* p_buffer);
    virtual bool IsSolid() const { return false; }
    virtual bool IsHazard() const { return false; }
};

class SkySpikeTile : public SkyTile
{
public:
    virtual void Draw(sPicture* p_buffer);
    virtual bool IsSolid() const { return true; }
    virtual bool IsHazard() const { return true; }
};

class SkyLevel
{
protected:
    SkyLevel() {}
    float _Gravity = 900.0f;
    sSize2D _SizeOfTile = {32, 32};
    const sSize2D _SizeInTiles;
    SkyTile** _pLevelMesh = nullptr;

    SkyEmptyTile _EmptyTile;

public:
    virtual ~SkyLevel();

    SkyLevel(const sSize2D & sizeInTiles);
    inline float Gravity() const { return _Gravity; }
    inline const sSize2D& SizeOfTile() const { return _SizeOfTile; }
    inline const sSize2D& SizeInTiles() const { return _SizeInTiles; }
    inline const int ScreenH() const { return _SizeOfTile.H * _SizeInTiles.H; }
    inline const int ScreenW() const { return _SizeOfTile.W * _SizeInTiles.W; }

    virtual bool SetupLevel() = 0;
    
    inline bool IsSolid(int tx, int ty) const { return GetTile(tx, ty)->IsSolid(); }
    inline bool IsHazard(int tx, int ty) const { return GetTile(tx, ty)->IsHazard(); }

    void AddTile(SkyTile *p_tile, int tx, int ty);
    const SkyTile* GetTile(int tx, int ty) const;

    inline void AddGroundTile(int tx, int ty) { AddTile(new SkyGroundTile(), tx, ty); }
    inline void AddSpikeTile(int tx, int ty) { AddTile(new SkySpikeTile(), tx, ty); }
    inline void AddDecorTile(int tx, int ty) { AddTile(new SkyDecorTile(), tx, ty); }
};

class SkyLevel_Level1 : public SkyLevel
{
public:
    SkyLevel_Level1();
    virtual bool SetupLevel();
};

class SkyLevel_Level2 : public SkyLevel
{
public:
    SkyLevel_Level2();
    virtual bool SetupLevel();
};

class SkyLevel_Level3 : public SkyLevel
{
public:
    SkyLevel_Level3();
    virtual bool SetupLevel();
};


class sGameplay : public IApplication
{
public:
    enum { MAX_LAYERS = 4 };
private:
    std::vector<SkyEntity*> _Entities[MAX_LAYERS];
    sPos2D _Camera = { 0, 0 };

public:
    inline const sPos2D& Camera() const { return _Camera; }

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

class sFont : public sAsset
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
    
    ~sAssetManager();
public:

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


public:
    void Start();

    Logging Log;

    void SetCurrentLevel(SkyLevel* val) {
        if (pCurrentLevel != nullptr) { delete pCurrentLevel; } 
        pCurrentLevel = val;
    }
    SkyLevel* CurrentLevel() { return pCurrentLevel; }
    sPlatform* GetPlatform() { return pPlatform; }
    sGameplay* GetGameplay() { return pGameplay; }
    sAssetManager* GetAssets() { return pAssets; }

private:
    SkyLevel* pCurrentLevel = nullptr;
    sPlatform* pPlatform = nullptr;
    sGameplay* pGameplay = nullptr;
    sAssetManager* pAssets = nullptr;
};

#define SKY_LEVEL() Skybound::getSingleton()->CurrentLevel()
#define SKY_ASSETS() Skybound::getSingleton()->GetAssets()
#define SKY_GAMEPLAY() Skybound::getSingleton()->GetGameplay()

