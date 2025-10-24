#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "arcanoid.h"

#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>

DWORD g_WIN_LastPrintTime = 0;
DWORD g_WIN_LastFrame = 0;
DWORD g_WIN_TotalFrames = 0;

#define ARCANOID_WINDOW_CLASS L"ArcanoidWindowClass"

#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

typedef struct WIN_BackBuffer
{
    AG_BackBuffer *pBB;
    HBITMAP     hBitmap;
    HDC         hDC;
    BITMAPINFO  BitmapHeader;
} WIN_BackBuffer;

static WIN_BackBuffer g_WIN_RenderBuffer = { 0 };

void WIN_BackbufferDestroy(WIN_BackBuffer* p_renderBuffer)
{
    if (p_renderBuffer == NULL)
    {
        return;
    }
    if (p_renderBuffer->hDC)
    {
        DeleteDC(p_renderBuffer->hDC);
        p_renderBuffer->hDC = NULL;
    }

    AG_ASSERT(p_renderBuffer->pBB != NULL);

    if (p_renderBuffer->hBitmap)
    {
        DeleteObject(p_renderBuffer->hBitmap);
        p_renderBuffer->hBitmap = NULL;
    }

    p_renderBuffer->pBB->pPixels = NULL;
    p_renderBuffer->pBB->Width = p_renderBuffer->pBB->Height = 0;
}

BOOL WIN_BackbufferCreate(WIN_BackBuffer* p_renderBuffer, AG_BackBuffer* p_agBuffer, HWND hwnd, int w, int h)
{
    AG_ASSERT(p_agBuffer != NULL);

    if (p_renderBuffer == NULL)
    {
        return FALSE;
    }
    if (w <= 0 || h <= 0)
    {
        return FALSE;
    }
    p_renderBuffer->pBB = p_agBuffer;
    AG_ASSERT(p_renderBuffer->pBB != NULL);
  
    WIN_BackbufferDestroy(p_renderBuffer);

    ZeroMemory(&p_renderBuffer->BitmapHeader, sizeof(p_renderBuffer->BitmapHeader));
    p_renderBuffer->BitmapHeader.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    p_renderBuffer->BitmapHeader.bmiHeader.biWidth = w;
    p_renderBuffer->BitmapHeader.bmiHeader.biHeight = -h;
    p_renderBuffer->BitmapHeader.bmiHeader.biPlanes = 1;
    p_renderBuffer->BitmapHeader.bmiHeader.biBitCount = 32;// BGRA
    p_renderBuffer->BitmapHeader.bmiHeader.biCompression = BI_RGB;

    HDC wndDC = GetDC(hwnd);
    p_renderBuffer->hDC = CreateCompatibleDC(wndDC);
    p_renderBuffer->hBitmap = CreateDIBSection(wndDC, &p_renderBuffer->BitmapHeader, DIB_RGB_COLORS, &p_renderBuffer->pBB->pPixels, NULL, 0);
    ReleaseDC(hwnd, wndDC);

    if (!p_renderBuffer->hDC || !p_renderBuffer->hBitmap || !p_renderBuffer->pBB->pPixels)
    {
        WIN_BackbufferDestroy(p_renderBuffer);
        return FALSE;
    }

    SelectObject(p_renderBuffer->hDC, p_renderBuffer->hBitmap);
    
    AG_ASSERT(p_renderBuffer->pBB != NULL);
    p_renderBuffer->pBB->Width = w;
    p_renderBuffer->pBB->Height = h;

    AG_Color* p = (AG_Color*)p_renderBuffer->pBB->pPixels;
    AG_ASSERT(p != NULL);
    for (int i = 0; i < w * h; ++i)
    {
        p[i] = AG_RGB(32, 32, 32);
    }
    return 1;
}

void WIN_Iteration(HWND hwnd)
{
    DWORD cur_time = GetTickCount();

    AG_ASSERT(hwnd != 0);

    if (g_WIN_LastPrintTime == 0)
    {
        g_WIN_LastPrintTime = cur_time;
    }

    if ((cur_time - g_WIN_LastPrintTime) > 1000)
    {
        WCHAR game_name[64] = { 0 };

        float fps = (float)(g_WIN_TotalFrames - g_WIN_LastFrame) / ((float)(cur_time - g_WIN_LastPrintTime) / 1000.0f);

        _snwprintf(game_name, (sizeof(game_name) / sizeof(game_name[0])),
            L"Arcanoid v0.1 %S %S FPS=%.3f", __DATE__, __TIME__, fps);

        SetWindowTextW(hwnd, game_name);

        g_WIN_LastFrame = g_WIN_TotalFrames;
        g_WIN_LastPrintTime = cur_time;
    }

    g_WIN_TotalFrames++;

    AG_GameIteration(cur_time);

    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    if (g_WIN_RenderBuffer.hDC && g_WIN_RenderBuffer.hBitmap)
    {
        AG_DrawWorld();
        
        AG_ASSERT(g_WIN_RenderBuffer.pBB != NULL);

        BitBlt(hdc, 0, 0, g_WIN_RenderBuffer.pBB->Width, g_WIN_RenderBuffer.pBB->Height,
            g_WIN_RenderBuffer.hDC, 0, 0, SRCCOPY);
    }
    EndPaint(hwnd, &ps);
}

LRESULT CALLBACK AG_WindowWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        RECT rc; GetClientRect(hwnd, &rc);
        WIN_BackbufferCreate(&g_WIN_RenderBuffer, AG_GetBackBuffer(), hwnd, rc.right - rc.left, rc.bottom - rc.top);
        AG_UpdateGameArea(1);
        return 0;
    }

    case WM_SIZE:
    {
        int w = LOWORD(lParam);
        int h = HIWORD(lParam);
        if (w > 0 && h > 0)
        {
            WIN_BackbufferCreate(&g_WIN_RenderBuffer, AG_GetBackBuffer(), hwnd, w, h);
            AG_UpdateGameArea(0);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_LBUTTONDOWN:
    {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        AG_TRACE("Mouse down (%d, %d)", x, y);
        AG_MouseAction(x, y, wParam, 1);
        SetCapture(hwnd);
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_MOUSEMOVE:
    {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        AG_TRACE("Mouse move (%d, %d)", x, y);
        AG_MouseAction(x, y, wParam, 0);
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_LBUTTONUP:
    {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        AG_TRACE("Mouse up (%d, %d)", x, y);
        AG_MouseAction(x, y, wParam, 2);
        ReleaseCapture();
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_KEYDOWN:
    {
        AG_PRINTLN("Key down %d!", wParam);
        switch (wParam) {
        case VK_ESCAPE: PostQuitMessage(0); break;
        case VK_LEFT:   AG_KeyAction(AG_Key_Left, 1); break;
        case VK_RIGHT:  AG_KeyAction(AG_Key_Right, 1); break;
        case 'A': case 'a': AG_KeyAction(AG_Key_A, 1); break;
        case 'D': case 'd': AG_KeyAction(AG_Key_D, 1); break;
        case VK_SPACE:  AG_KeyAction(AG_Key_Space, 1); break;
        case 'R': case 'r': AG_KeyAction(AG_Key_Reset, 1); break;
        }
        return 0;
    }

    case WM_KEYUP:
    {
        AG_PRINTLN("Key up %d!", wParam);
        switch (wParam) {
        case VK_LEFT:   AG_KeyAction(AG_Key_Left, 0); break;
        case VK_RIGHT:  AG_KeyAction(AG_Key_Right, 0); break;
        case 'A': case 'a': AG_KeyAction(AG_Key_A, 0); break;
        case 'D': case 'd': AG_KeyAction(AG_Key_D, 0); break;
        }
        return 0;
    }

    case WM_PAINT:
    {
        WIN_Iteration(hwnd);
        return 0;
    }

    case WM_DESTROY:
        WIN_BackbufferDestroy(&g_WIN_RenderBuffer);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

HWND WIN_WindowCreate(int w, int h, LPWSTR p_caption)
{
    HINSTANCE hInst = GetModuleHandle(NULL);
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

#ifdef _DEBUG
void WIN_InitConsole(void)
{
    // Try to attach to parent console first; if none, create a new one.
    if (!AttachConsole(ATTACH_PARENT_PROCESS)) 
    {
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
#else
void WIN_InitConsole(void)
{

}
#endif


void WIN_MainLoop(HWND hwnd)
{
    // Main Loop
    MSG msg;
    BOOL running = TRUE;

    AG_PRINTLN("Start main loop!");
    while (running)
    {
        // Process all pending messages
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                running = FALSE;
                break;
            }

            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        // If no messages are left in the queue, trigger your redraw
        if (running)
        {
            InvalidateRect(hwnd, NULL, FALSE);
            UpdateWindow(hwnd); // optional, to force immediate repaint
        }
    }
}