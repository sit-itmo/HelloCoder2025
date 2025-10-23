#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <stdarg.h>

#ifdef _DEBUG
#define AG_PRINTLN(a, ...) { printf("\n%d: %lld:" a , __LINE__, __rdtsc(), ##__VA_ARGS__); }
#define AG_PRINT(a, ...) { printf( a , ##__VA_ARGS__); }
#define AG_DEBUG(a, ...) { printf("\n%d: " a , __LINE__, ##__VA_ARGS__); }
#define AG_TRACE(a, ...) { printf("\n%d: " a , __LINE__, ##__VA_ARGS__); }
#define AG_ERROR(a, ...) { printf("\n%d ------------- ERROR!\n" a , __LINE__, ##__VA_ARGS__); __debugbreak();}
#define AG_WARNING(a, ...) { printf("\n%d: warning! " a , __LINE__, ##__VA_ARGS__); }
#else
#define AG_PRINTLN(a, ...) {}
#define AG_PRINT(a, ...) { }
#define AG_DEBUG(a, ...) { }
#define AG_TRACE(a, ...) { }
#define AG_ERROR(a, ...) { }
#define AG_WARNING(a, ...) { }
#endif

#if 0
void AG_PRINTLN(const char* p_text, ...);

void AG_PRINTLN(const char* p_text, ...)
{

#if 0
    va_list args;
    va_start(args, p_text);
    printf("\n%d: ", __LINE__);
    vprintf(p_text, args);
    va_end(args);
#endif
}
#endif

#define M_PI       3.14159265358979323846 
#define ARCANOID_WINDOW_CLASS L"ArcanoidWindowClass"

#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

typedef union _AG_Color
{
    UINT32 ARGB; // => 0xAARRGGBB;
    struct
    {
        BYTE B;
        BYTE G;
        BYTE R;
        BYTE A;
    };
    struct
    {
        UINT32 RGB : 24;
        UINT32 Alpha : 8;
    };
}AG_Color;


typedef struct AG_BackBuffer
{
    HBITMAP     hBitmap;
    HDC         hDC;
    AG_Color* pPixels;
    int         Width;
    int         Height;
    BITMAPINFO  BitmapHeader;
} AG_BackBuffer;

typedef struct _AG_Brick
{ 
    float x, y, w, h; 
    BOOLEAN alive; 
    int hp; 
    AG_Color color; 
} AG_Brick;

#define AG_MAX_ROWS 10
#define AG_MAX_COLS 14

AG_Brick g_bricks[AG_MAX_ROWS * AG_MAX_COLS];
int g_rows = 8, g_cols = 12;

float g_paddleX = 0, g_paddleY = 0, g_paddleW = 100, g_paddleH = 16;
float g_ballX = 0, g_ballY = 0, g_ballVX = 0, g_ballVY = 0, g_ballR = 8;
int   g_score = 0, g_lives = 3, g_level = 1;
BOOLEAN  g_running = 0;
BOOLEAN g_keyLeft = 0, g_keyRight = 0, g_keyA = 0, g_keyD = 0;

RECT  g_playArea = { 0 };

AG_BackBuffer g_AG_RenderBuffer = { 0 };

void AG_BackbufferDestroy(AG_BackBuffer* p_renderBuffer)
{
    if(p_renderBuffer == NULL)
    { 
        return;
    }
    if (p_renderBuffer->hDC)
    { 
        DeleteDC(p_renderBuffer->hDC);
        p_renderBuffer->hDC = NULL;
    }

    p_renderBuffer->pPixels = NULL;
    if (p_renderBuffer->hBitmap) 
    { 
        DeleteObject(p_renderBuffer->hBitmap);
        p_renderBuffer->hBitmap = NULL;
    }
    p_renderBuffer->Width = p_renderBuffer->Height = 0;
}

BOOL AG_BackbufferCreate(AG_BackBuffer* p_renderBuffer, HWND hwnd, int w, int h)
{
    if (p_renderBuffer == NULL)
    {
        return FALSE;
    }
    if (w <= 0 || h <= 0)
    {
        return FALSE;
    }
    AG_BackbufferDestroy(p_renderBuffer);

    ZeroMemory(&p_renderBuffer->BitmapHeader, sizeof(p_renderBuffer->BitmapHeader));
    p_renderBuffer->BitmapHeader.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    p_renderBuffer->BitmapHeader.bmiHeader.biWidth = w;
    p_renderBuffer->BitmapHeader.bmiHeader.biHeight = -h;
    p_renderBuffer->BitmapHeader.bmiHeader.biPlanes = 1;
    p_renderBuffer->BitmapHeader.bmiHeader.biBitCount = 32;// BGRA
    p_renderBuffer->BitmapHeader.bmiHeader.biCompression = BI_RGB;

    HDC wndDC = GetDC(hwnd);
    p_renderBuffer->hDC = CreateCompatibleDC(wndDC);
    p_renderBuffer->hBitmap = CreateDIBSection(wndDC, &p_renderBuffer->BitmapHeader, DIB_RGB_COLORS, &p_renderBuffer->pPixels, NULL, 0);
    ReleaseDC(hwnd, wndDC);

    if (!p_renderBuffer->hDC || !p_renderBuffer->hBitmap || !p_renderBuffer->pPixels)
    {
        AG_BackbufferDestroy(p_renderBuffer);
        return FALSE;
    }

    SelectObject(p_renderBuffer->hDC, p_renderBuffer->hBitmap);
    p_renderBuffer->Width = w;
    p_renderBuffer->Height = h;

    AG_Color* p = (AG_Color*)p_renderBuffer->pPixels;
    for (int i = 0; i < w * h; ++i)
    {
        p[i].ARGB = 0xFF202020;
    }
    return 1;
}

inline void AG_PutPixel(AG_BackBuffer* p_renderBuffer, int x, int y, AG_Color color)
{
    if (p_renderBuffer == NULL
        || x < 0 || y < 0
        || x >= p_renderBuffer->Width
        || y >= p_renderBuffer->Height
        || p_renderBuffer->pPixels
        || color.A == 0)
    {
        return;
    }
    AG_Color* p_px = p_renderBuffer->pPixels + y * p_renderBuffer->Width + x;

    if (color.A == 255)
    {
        p_px->ARGB = color.ARGB;
    }
    else
    {
        int invA = 255 - color.A;
        color.A = 0xFF;
        color.B = (BYTE)((color.B * color.A + p_px->B * invA) / 255);
        color.G = (BYTE)((color.G * color.A + p_px->G * invA) / 255);
        color.R = (BYTE)((color.R * color.A + p_px->R * invA) / 255);
    }
}

void AG_FillRectPx(AG_BackBuffer* p_renderBuffer, int x0, int y0, int x1, int y1, AG_Color color)
{
    if (p_renderBuffer == NULL) return;
    if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
    if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }
    if (x0 >= p_renderBuffer->Width || y0 >= p_renderBuffer->Height || x1 < 0 || y1 < 0) return;
    if (x0 < 0) x0 = 0; if (y0 < 0) y0 = 0;
    if (x1 >= p_renderBuffer->Width)  x1 = p_renderBuffer->Width - 1;
    if (y1 >= p_renderBuffer->Height) y1 = p_renderBuffer->Height - 1;

    for (int y = y0; y <= y1; ++y) 
    {
        AG_Color* row = (p_renderBuffer->pPixels + y * p_renderBuffer->Width);
        for (int x = x0; x <= x1; ++x) row[x] = color;
    }
}

void AG_FillCirclePx(AG_BackBuffer* p_renderBuffer, int cx, int cy, int radius, AG_Color color)
{
    if (p_renderBuffer == NULL)
    {
        return;
    }
    if (radius <= 0) { AG_PutPixel(p_renderBuffer, cx, cy, color); return; }
    int r2 = radius * radius;
    for (int y = -radius; y <= radius; ++y) 
    {
        int yy = cy + y;
        if (yy < 0 || yy >= p_renderBuffer->Height) continue;
        int dx = (int)(sqrt((double)(r2 - y * y)) + 0.5);
        int x0 = cx - dx, x1 = cx + dx;
        if (x1 < 0 || x0 >= p_renderBuffer->Width) continue;
        if (x0 < 0) x0 = 0; if (x1 >= p_renderBuffer->Width) x1 = p_renderBuffer->Width - 1;

        AG_Color* row = (p_renderBuffer->pPixels + yy * p_renderBuffer->Width);
        for (int x = x0; x <= x1; ++x) row[x] = color;
    }
}

void AG_StrokeRectPx(AG_BackBuffer* p_renderBuffer, int x0, int y0, int x1, int y1, AG_Color color)
{
    AG_FillRectPx(p_renderBuffer, x0, y0, x1, y0, color);
    AG_FillRectPx(p_renderBuffer, x0, y1, x1, y1, color);
    AG_FillRectPx(p_renderBuffer, x0, y0, x0, y1, color);
    AG_FillRectPx(p_renderBuffer, x1, y0, x1, y1, color);
}


void AG_BufferClear(AG_BackBuffer* p_renderBuffer, AG_Color color)
{
    if (p_renderBuffer == NULL)
    {
        return;
    }
    color.A = 0xFF;
    int count = p_renderBuffer->Width * p_renderBuffer->Height;
    for (int i = 0; i < count; ++i)
    {
        p_renderBuffer->pPixels[i].ARGB = color.ARGB;
    }
}


void AG_LayoutPlayArea(void) {
    int margin = 20;
    g_playArea.left = margin;
    g_playArea.top = margin + 40;  // место под HUD
    g_playArea.right = g_AG_RenderBuffer.Width - margin;
    g_playArea.bottom = g_AG_RenderBuffer.Height - margin;
}

void AG_ResetPaddle(void)
{
    g_paddleW = (float)((g_playArea.right - g_playArea.left) / 6);
    if (g_paddleW < 60) g_paddleW = 60;
    g_paddleH = 16.0f;
    g_paddleX = (float)((g_playArea.left + g_playArea.right - (int)g_paddleW) / 2);
    g_paddleY = (float)(g_playArea.bottom - g_paddleH - 8);
}

void AG_ResetBall(BOOLEAN attach) 
{
    g_ballR = 8.0f;
    g_ballX = g_paddleX + g_paddleW * 0.5f;
    g_ballY = g_paddleY - g_ballR - 2.0f;
    float speed = 6.0f + (float)g_level * 0.8f;
    float angle = (float)(-M_PI * 0.35); // немного влево вверх
    g_ballVX = cosf(angle) * speed;
    g_ballVY = sinf(angle) * speed;
    if (attach) g_running = 0;
}

void AG_BuildBricks(void) 
{
    int areaW = g_playArea.right - g_playArea.left;
    int areaH = g_playArea.bottom - g_playArea.top;

    g_cols = (areaW >= 800) ? 12 : (areaW >= 600 ? 10 : 8);
    if (g_cols > AG_MAX_COLS) g_cols = AG_MAX_COLS;
    g_rows = (areaH >= 600) ? 9 : 7;
    if (g_rows > AG_MAX_ROWS) g_rows = AG_MAX_ROWS;

    float cellW = (float)areaW / g_cols;
    float cellH = (float)areaH / (g_rows + 8); // верхн€€ часть под кирпичи

    for (int r = 0; r < g_rows; ++r) 
    {
        for (int c = 0; c < g_cols; ++c)
        {
            AG_Brick* b = &g_bricks[r * g_cols + c];
            float pad = 3.f;
            b->x = g_playArea.left + c * cellW + pad;
            b->y = g_playArea.top + r * cellH + pad;
            b->w = cellW - pad * 2;
            b->h = cellH - pad * 2;
            b->alive = 1;
            b->hp = 1;
            AG_Color color;
            color.A = 0xFF;
            color.R = (120 + (r * 15) % 135);
            color.G = (80 + (c * 13) % 150);
            color.B = (180 - (r * 10) % 100);
            b->color.ARGB = color.ARGB;
        }
    }
}

void AG_NewGame(void) 
{
    AG_LayoutPlayArea();
    AG_ResetPaddle();
    AG_BuildBricks();
    g_score = 0;
    g_lives = 3;
    g_level = 1;
    AG_ResetBall(1);
}

void NextLevel(void)
{
    g_level++;
    AG_BuildBricks();
    AG_ResetPaddle();
    AG_ResetBall(1);
}

void AG_DrawWorld(void) 
{
    AG_Color color;
    color.ARGB = 0xFF101014;
    AG_BufferClear(&g_AG_RenderBuffer, color);

    color.ARGB = 0xFF46465A;
    AG_StrokeRectPx(&g_AG_RenderBuffer, g_playArea.left, g_playArea.top, g_playArea.right, g_playArea.bottom, color);

    for (int r = 0; r < g_rows; ++r) 
    {
        for (int c = 0; c < g_cols; ++c) 
        {
            AG_Brick* b = &g_bricks[r * g_cols + c];
            if (!b->alive) continue;
            AG_FillRectPx(&g_AG_RenderBuffer, (int)b->x, (int)b->y, (int)(b->x + b->w), (int)(b->y + b->h), b->color);
            color.ARGB = 0xFF000000;
            AG_StrokeRectPx(&g_AG_RenderBuffer, (int)b->x, (int)b->y, (int)(b->x + b->w), (int)(b->y + b->h), color);
        }
    }

    color.ARGB = 0xFFC8D2F0;
    AG_FillRectPx(&g_AG_RenderBuffer, (int)g_paddleX, (int)g_paddleY, (int)(g_paddleX + g_paddleW), (int)(g_paddleY + g_paddleH), color);
    color.ARGB = 0xFFFFB43C;
    AG_FillCirclePx(&g_AG_RenderBuffer, (int)roundf(g_ballX), (int)roundf(g_ballY), (int)g_ballR, color);
}

void AG_ClampPaddle(void) 
{
    float minX = (float)g_playArea.left + 2;
    float maxX = (float)g_playArea.right - g_paddleW - 2;
    if (g_paddleX < minX) g_paddleX = minX;
    if (g_paddleX > maxX) g_paddleX = maxX;
}

BOOLEAN AG_CircleIntersectsRect(float cx, float cy, float r, float rx, float ry, float rw, float rh, float* nx, float* ny) 
{
    float closestX = (cx < rx) ? rx : (cx > rx + rw ? rx + rw : cx);
    float closestY = (cy < ry) ? ry : (cy > ry + rh ? ry + rh : cy);
    float dx = cx - closestX;
    float dy = cy - closestY;
    float dist2 = dx * dx + dy * dy;
    if (dist2 <= r * r) 
    {
        float len = sqrtf(dist2);
        if (len > 1e-4f) { *nx = dx / len; *ny = dy / len; }
        else 
        { 
            if (fabsf(dx) > fabsf(dy)) { *nx = (dx > 0) ? 1.f : -1.f; *ny = 0.f; }
            else { *nx = 0.f; *ny = (dy > 0) ? 1.f : -1.f; }
        }
        return 1;
    }
    return 0;
}

void AG_UpdateBall(void) 
{
    if (!g_running) 
    { 
        g_ballX = g_paddleX + g_paddleW * 0.5f;
        g_ballY = g_paddleY - g_ballR - 2.0f;
        return;
    }

    float nextX = g_ballX + g_ballVX;
    float nextY = g_ballY + g_ballVY;

    if (nextX - g_ballR < g_playArea.left) { nextX = g_playArea.left + g_ballR; g_ballVX = -g_ballVX; }
    if (nextX + g_ballR > g_playArea.right) { nextX = g_playArea.right - g_ballR; g_ballVX = -g_ballVX; }
    if (nextY - g_ballR < g_playArea.top) { nextY = g_playArea.top + g_ballR; g_ballVY = -g_ballVY; }

    if (nextY - g_ballR > g_playArea.bottom) 
    {
        if (--g_lives < 0) 
        { // game over
            g_lives = 3; g_score = 0; g_level = 1; AG_BuildBricks();
        }
        AG_ResetBall(1);
        return;
    }

    float nx = 0, ny = 0;
    if (AG_CircleIntersectsRect(nextX, nextY, g_ballR, g_paddleX, g_paddleY, g_paddleW, g_paddleH, &nx, &ny)) 
    {
        float hit = (nextX - (g_paddleX + g_paddleW * 0.5f)) / (g_paddleW * 0.5f); // -1..1
        if (hit < -1) hit = -1; if (hit > 1) hit = 1;
        float speed = sqrtf(g_ballVX * g_ballVX + g_ballVY * g_ballVY);
        float angle = (float)(-M_PI * 0.25 - hit * (M_PI * 0.35));
        g_ballVX = cosf(angle) * speed;
        g_ballVY = sinf(angle) * speed;
        nextY = g_paddleY - g_ballR - 0.1f;
    }
    else 
    {
        int aliveCount = 0;
        for (int r = 0; r < g_rows; ++r) 
        {
            for (int c = 0; c < g_cols; ++c) 
            {
                AG_Brick* b = &g_bricks[r * g_cols + c];
                if (!b->alive) continue;
                aliveCount++;
                if (AG_CircleIntersectsRect(nextX, nextY, g_ballR, b->x, b->y, b->w, b->h, &nx, &ny)) 
                {
                    AG_ERROR("HIT the block %d x %d!", c, r);
                    float dot = g_ballVX * nx + g_ballVY * ny;
                    g_ballVX -= 2 * dot * nx;
                    g_ballVY -= 2 * dot * ny;
                    g_score += 10;
                    if (--b->hp <= 0) 
                    { 
                        b->alive = 0; 
                        aliveCount--; 
                    }
                    nextX += nx * 1.2f;
                    nextY += ny * 1.2f;
                    goto bricks_done;
                }
            }
        }
    bricks_done:
        if (aliveCount == 0)
        {
            NextLevel();
        }
    }

    g_ballX = nextX; g_ballY = nextY;
}

void AG_UpdatePaddle(void) 
{
    float move = 0.f;
    if (g_keyLeft || g_keyA)  move -= 1.f;
    if (g_keyRight || g_keyD) move += 1.f;
    float speed = 9.0f;
    g_paddleX += move * speed;
    AG_ClampPaddle();
}

void AG_Tick(void)
{
    AG_UpdatePaddle();
    AG_UpdateBall();
}

static UINT_PTR g_timerId = 0;

LRESULT CALLBACK AG_WindowWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{
    switch (msg) 
    {
    case WM_CREATE: 
    {
        RECT rc; GetClientRect(hwnd, &rc);
        AG_BackbufferCreate(&g_AG_RenderBuffer, hwnd, rc.right - rc.left, rc.bottom - rc.top);
        AG_LayoutPlayArea();
        AG_ResetPaddle();
        AG_BuildBricks();
        AG_ResetBall(1); 

        // 60 FPS
        g_timerId = SetTimer(hwnd, 1, 16, NULL);
        return 0;
    }

    case WM_SIZE: 
    {
        int w = LOWORD(lParam);
        int h = HIWORD(lParam);
        if (w > 0 && h > 0) 
        {
            AG_BackbufferCreate(&g_AG_RenderBuffer, hwnd, w, h);
            AG_LayoutPlayArea();
            AG_ResetPaddle();
            AG_ResetBall(!g_running);
            AG_BuildBricks();
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_TIMER: {
        if (wParam == g_timerId) 
        {
            AG_Tick();
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_LBUTTONDOWN: 
    {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        SetCapture(hwnd);
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_MOUSEMOVE: 
    {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_LBUTTONUP: 
    {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        ReleaseCapture();
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_KEYDOWN: 
    {
        AG_PRINTLN("Key down %d!", wParam);
        switch (wParam) {
        case VK_ESCAPE: PostQuitMessage(0); break;
        case VK_LEFT:   g_keyLeft = 1; break;
        case VK_RIGHT:  g_keyRight = 1; break;
        case 'A': case 'a': g_keyA = 1; break;
        case 'D': case 'd': g_keyD = 1; break;
        case VK_SPACE:  g_running = !g_running; break;
        case 'R': case 'r': AG_NewGame(); break;
        }
        return 0;
    }

    case WM_KEYUP: 
    {
        AG_PRINTLN("Key up %d!", wParam);
        switch (wParam) {
        case VK_LEFT:   g_keyLeft = 0; break;
        case VK_RIGHT:  g_keyRight = 0; break;
        case 'A': case 'a': g_keyA = 0; break;
        case 'D': case 'd': g_keyD = 0; break;
        }
        return 0;
    }

    case WM_PAINT: 
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        if (g_AG_RenderBuffer.hDC && g_AG_RenderBuffer.hBitmap) 
        {
            AG_DrawWorld();
            BitBlt(hdc, 0, 0, g_AG_RenderBuffer.Width, g_AG_RenderBuffer.Height, 
                g_AG_RenderBuffer.hDC, 0, 0, SRCCOPY);
        }
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        if (g_timerId) { KillTimer(hwnd, g_timerId); g_timerId = 0; }
        AG_BackbufferDestroy(&g_AG_RenderBuffer);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

HWND AG_WindowCreate(HINSTANCE hInst, int w, int h, LPWSTR p_caption)
{
    WNDCLASSEXW wc = { sizeof(wc) };
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = AG_WindowWndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_CROSS);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = ARCANOID_WINDOW_CLASS;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm = wc.hIcon;

    if (!RegisterClassExW(&wc))
    {
        return 0;
    }

    DWORD style = WS_OVERLAPPEDWINDOW;
    style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
    RECT r = { 0, 0, w, h };
    AdjustWindowRect(&r, style, FALSE);

    HWND hwnd = CreateWindowExW(
        0, ARCANOID_WINDOW_CLASS, p_caption,
        style, CW_USEDEFAULT, CW_USEDEFAULT,
        r.right - r.left, r.bottom - r.top,
        NULL, NULL, hInst, NULL);

    if (!hwnd)
    {
        return 0;
    }

    ShowWindow(hwnd, SW_NORMAL);
    UpdateWindow(hwnd);

    return hwnd;
}

#ifdef WIN32
void InitConsole(void)
{
    // Try to attach to parent console first; if none, create a new one.
    if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
        AllocConsole();
    }

    // Redirect C stdio to the console using special files.
    // Use freopen_s if available (MSVC); fallback to freopen otherwise.

#if defined(_MSC_VER)
    FILE* fp;
    freopen_s(&fp, "CONIN$", "r", stdin);
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
#else
    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
#endif

    // Optional: disable buffering so output appears immediately
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    // Optional: if you want wide-character printf (wprintf) to emit UTF-16
    // uncomment the following lines:
    // _setmode(_fileno(stdin),  _O_WTEXT);
    // _setmode(_fileno(stdout), _O_WTEXT);
    // _setmode(_fileno(stderr), _O_WTEXT);
}
#endif

int APIENTRY wWinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR lpCmdLine, int nCmdShow) 
{
    int w = 1024;
    int h = 768;

#ifdef _DEBUG
    InitConsole();
#endif


    AG_PRINTLN("Start Game %d x %d!",  w, h);

    HWND hwnd = AG_WindowCreate(hInst, w, h, L"Arcanoid v0.1");
    if (!hwnd)
    {
        AG_PRINTLN("\nError: failed to create window!");
        return 0;
    }
    AG_PRINTLN("Main window was created!");


    
    // Main Loop
    MSG msg;
    AG_PRINTLN("Start main loop!");
    while (GetMessageW(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    AG_PRINTLN("Completed!");
    return 0;
}
