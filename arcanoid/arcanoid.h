#define WIN32_LEAN_AND_MEAN
#include <stdarg.h>

enum AG_LOG_KIND
{
    AG_LOG_KIND_PRINT,
    AG_LOG_KIND_PLINE,
    AG_LOG_KIND_TRACE,
    AG_LOG_KIND_ERROR,
    AG_LOG_KIND_DEBUG,
    AG_LOG_KIND_WARNI
};

void AG_LogMessage(const char* p_file, int line, enum AG_LOG_KIND kind, const char* p_text, ...);

#if defined(_WIN32) && defined(_DEBUG)
#define AG_BREAKPOINT()  __debugbreak();
#elif defined(_DEBUG)
#define AG_BREAKPOINT()  __debugbreak();
#else
#define AG_BREAKPOINT() {}
#endif

#ifdef _DEBUG
#define AG_PRINTLN(a, ...) AG_LogMessage(__FILE__, __LINE__, AG_LOG_KIND_PLINE, a, ##__VA_ARGS__)
#define AG_PRINT(a, ...) AG_LogMessage(__FILE__, __LINE__, AG_LOG_KIND_PRINT, a, ##__VA_ARGS__)
#define AG_DEBUG(a, ...) AG_LogMessage(__FILE__, __LINE__, AG_LOG_KIND_DEBUG, a, ##__VA_ARGS__)
#define AG_TRACE(a, ...) AG_LogMessage(__FILE__, __LINE__, AG_LOG_KIND_TRACE, a, ##__VA_ARGS__)
#define AG_ERROR(a, ...) { AG_LogMessage(__FILE__, __LINE__, AG_LOG_KIND_ERROR, a, ##__VA_ARGS__); AG_BREAKPOINT(); }
#define AG_WARNING(a, ...) AG_LogMessage(__FILE__, __LINE__, AG_LOG_KIND_WARNI, a, ##__VA_ARGS__)

#define AG_ASSERT(a) if (!(a)) { AG_ERROR(#a); }
#define D(a) AG_PRINTLN(#a) a

#else
#define AG_PRINTLN(a, ...) {}
#define AG_PRINT(a, ...) { }
#define AG_DEBUG(a, ...) { }
#define AG_TRACE(a, ...) { }
#define AG_ERROR(a, ...) AG_LogMessage(__FILE__, __LINE__, AG_LOG_KIND_ERROR, a, ##__VA_ARGS__)
#define AG_WARNING(a, ...) { }
#endif

#ifndef M_PI
#define M_PI       3.14159265358979323846 
#endif

typedef union _AG_Color
{
    unsigned int ARGB; // => 0xAARRGGBB;
    struct
    {
        unsigned char B;
        unsigned char G;
        unsigned char R;
        unsigned char A;
    };
    struct
    {
        unsigned int RGB : 24;
        unsigned int Alpha : 8;
    };
}AG_Color;

inline AG_Color AG_RGB(unsigned char r, unsigned char g, unsigned char b) 
{ 
    AG_Color c;
    c.R = r; c.G = g; c.B = b;
    c.A = 255;
    return c; 
}

typedef struct _AG_BackBuffer
{
    AG_Color* pPixels;
    int         Width;
    int         Height;
} AG_BackBuffer;

typedef struct _AG_Brick
{
    float x, y, w, h;
    int alive;
    int hp;
    AG_Color color;
} AG_Brick;

typedef struct _AG_Rect
{
    long left;
    long top;
    long right;
    long bottom;
} AG_Rect;


enum AG_Key
{
    AG_Key_None = 0,
    AG_Key_Left,
    AG_Key_Right,
    AG_Key_A,
    AG_Key_D,
    AG_Key_Space,
    AG_Key_Reset
};

AG_BackBuffer *AG_GetBackBuffer();
void AG_UpdateGameArea(int resetBall);
void AG_KeyAction(enum AG_Key key, int down);
void AG_MouseAction(int mouseX, int mouseY, int btns, int action);
void AG_GameIteration(unsigned long ticks);
void AG_DrawWorld(void);

int AG_CircleIntersectsRect(float cx, float cy, float r,
    float rx, float ry, float rw, float rh, float* nx, float* ny);
void AG_PutPixel(AG_BackBuffer* p_renderBuffer, int x, int y, AG_Color color);
void AG_FillRectPx(AG_BackBuffer* p_renderBuffer, int x0, int y0, int x1, int y1, AG_Color color);
void AG_FillCirclePx(AG_BackBuffer* p_renderBuffer, int cx, int cy, int radius, AG_Color color);
void AG_StrokeRectPx(AG_BackBuffer* p_renderBuffer, int x0, int y0, int x1, int y1, AG_Color color);
void AG_BufferClear(AG_BackBuffer* p_renderBuffer, AG_Color color);

