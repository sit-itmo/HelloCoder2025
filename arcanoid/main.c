#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <math.h>

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

inline void PutPixel(AG_BackBuffer* p_renderBuffer, int x, int y, AG_Color color)
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

LRESULT CALLBACK AG_WindowWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{
    switch (msg) 
    {
    case WM_CREATE: 
    {
        RECT rc; GetClientRect(hwnd, &rc);
        AG_BackbufferCreate(&g_AG_RenderBuffer, hwnd, rc.right - rc.left, rc.bottom - rc.top);
        return 0;
    }

    case WM_SIZE: 
    {
        int w = LOWORD(lParam);
        int h = HIWORD(lParam);
        if (w > 0 && h > 0) 
        {
            AG_BackbufferCreate(&g_AG_RenderBuffer, hwnd, w, h);
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
        //wParam == key
        return 0;
    }

    case WM_PAINT: 
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        if (g_AG_RenderBuffer.hDC && g_AG_RenderBuffer.hBitmap) 
        {
            BitBlt(hdc, 0, 0, g_AG_RenderBuffer.Width, g_AG_RenderBuffer.Height, 
                g_AG_RenderBuffer.hDC, 0, 0, SRCCOPY);
        }
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
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


int APIENTRY wWinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR lpCmdLine, int nCmdShow) 
{
    HWND hwnd = AG_WindowCreate(hInst, 1024, 768, L"Arcanoid v0.1");
    if (!hwnd)
    {
        return 0;
    }
    
    // Main Loop
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) 
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}
