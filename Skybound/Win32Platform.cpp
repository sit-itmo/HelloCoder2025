#include <windows.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>

#include "Skybound.h"

class Win32Platform : public Platform
{
private:
    Size2D RenderSize;
    HWND   hWnd = NULL;
    Picture BackBuffer;
    BITMAPINFO Bmi = { 0 };

    bool InitWindow(const sString& caption);
    bool InitGraphics();
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void Reset();
    static void InitConsole();

public:

    bool Setup(const sString& caption, const Size2D& size);
    void Loop();
    bool GetKeyState(KeyCode code);

    ~Win32Platform();

};

Platform* Win32PlatformBuilder::Build()
{
    return new Win32Platform();
}

#ifdef _DEBUG
void Win32Platform::InitConsole()
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
void Win32Platform::InitConsole(void)
{

}
#endif

Win32Platform::~Win32Platform()
{
    Reset();
}

bool Win32Platform::InitWindow(const sString& caption)
{
    HINSTANCE hInstance = GetModuleHandle(NULL);

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "SkyboundWin32PlatformWndClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClass(&wc))
        return false;

    RECT rc = { 0, 0, RenderSize.W, RenderSize.H };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    int winW = rc.right - rc.left;
    int winH = rc.bottom - rc.top;

    hWnd = CreateWindowExA(
        0,
        wc.lpszClassName,
        caption.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        winW, winH,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!hWnd)
        return false;

    ShowWindow(hWnd, SW_NORMAL);
    UpdateWindow(hWnd);
    return true;
}

bool Win32Platform::InitGraphics()
{
    BackBuffer.Resize(RenderSize);

    ZeroMemory(&Bmi, sizeof(Bmi));
    Bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    Bmi.bmiHeader.biWidth = RenderSize.W;
    Bmi.bmiHeader.biHeight = -(LONG)RenderSize.H; // top-down
    Bmi.bmiHeader.biPlanes = 1;
    Bmi.bmiHeader.biBitCount = 32;
    Bmi.bmiHeader.biCompression = BI_RGB;
    return true;
}

LRESULT CALLBACK Win32Platform::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            PostQuitMessage(0);
            return 0;
        }
        break;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void Win32Platform::Reset()
{

}

bool Win32Platform::Setup(const sString& caption, const Size2D& size)
{
    Reset();
    RenderSize = size;
    InitWindow(caption);
    InitGraphics();
    return true;
}

void Win32Platform::Loop()
{
    DWORD prevTime = GetTickCount();
    bool g_running = true;

    MSG msg;
    while (g_running)
    {
        // обработка сообщений Windows
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                g_running = false;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // расчёт dt
        DWORD currTime = GetTickCount();
        float dt = (currTime - prevTime) / 1000.0f;
        if (dt > 0.05f) dt = 0.05f; // ограничим шаг
        prevTime = currTime;

        // вывод на экран
        HDC hdc = GetDC(hWnd);
        StretchDIBits(
            hdc,
            0, 0, RenderSize.W, RenderSize.H,
            0, 0, RenderSize.W, RenderSize.H,
            BackBuffer.Data(),
            &Bmi,
            DIB_RGB_COLORS,
            SRCCOPY
        );
        ReleaseDC(hWnd, hdc);
        Sleep(1);
    }

}

bool Win32Platform::GetKeyState(KeyCode code)
{
    switch (code)
    {
    case KeyCode_Left: return (GetAsyncKeyState(VK_LEFT) & 0x8000) != 0;
    case KeyCode_Right: return (GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0;
    case KeyCode_Space: return (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
    case KeyCode_Fire: return (GetAsyncKeyState(0x5A) & 0x8000) != 0; // Z
    }
    return false;
}


