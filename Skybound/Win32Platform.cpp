#include <windows.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>

#include "Skybound.h"

class sWin32Platform : public sPlatform
{
private:
    sSize2D RenderSize;
    bool    IsInjected = false;
    HWND   hWnd = NULL;
    sPicture BackBuffer;
    BITMAPINFO Bmi = { 0 };

    bool SetupConsole();
    bool InitWindow(const sString& caption);
    bool InitGraphics();
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void Reset();
    const sSize2D& ScreenSize() { return RenderSize; }

public:

    bool Setup(const sString& caption, const sSize2D& size, long long exParam);
    void Loop();
    float GetTime();
    bool GetKeyState(KeyCode code);

    ~sWin32Platform();

};

sPlatform* sWin32PlatformBuilder::Build()
{
    return new sWin32Platform();
}

#ifdef _DEBUG
bool sWin32Platform::SetupConsole()
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
    
        // Get the handle to the console output
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut == INVALID_HANDLE_VALUE)
            return false;

        DWORD dwMode = 0;
        if (!GetConsoleMode(hOut, &dwMode))
            return false;

        // Add ENABLE_VIRTUAL_TERMINAL_PROCESSING to the existing mode
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;

        SetConsoleMode(hOut, dwMode);
        return true;
}
#else
void sWin32Platform::SetupConsole(void)
{

}
#endif

sWin32Platform::~sWin32Platform()
{
    Reset();
}

bool sWin32Platform::InitWindow(const sString& caption)
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

    RECT rc = { 0, 0, (LONG)RenderSize.W, (LONG)RenderSize.H };
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

bool sWin32Platform::InitGraphics()
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

LRESULT CALLBACK sWin32Platform::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
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

void sWin32Platform::Reset()
{

}

bool sWin32Platform::Setup(const sString& caption, const sSize2D& size, long long exParam)
{
    Reset();
    RenderSize = size;
    if (exParam == 0)
    {
        InitWindow(caption);
    }
    else
    {
        IsInjected = true;
        hWnd = (HWND)exParam;
    }
    InitGraphics();
    return true;
}

float sWin32Platform::GetTime()
{
    return GetTickCount() / 1000.0f;
}

void sWin32Platform::Loop()
{
    DWORD prevTime = GetTickCount();
    bool g_running = true;

    MSG msg;
    while (g_running)
    {
        SKY_PROFSCOPE("main_fps");

        if (IsInjected == false)
        {
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT)
                    g_running = false;
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        DWORD currTime = GetTickCount();
        float dt = (currTime - prevTime) / 1000.0f;
        if (dt > 0.05f) dt = 0.05f;

        {
            SKY_PROFSCOPE("main_update");
            for (auto& a : Apps)
            {
                a->Update(currTime / 1000.0f, dt);
            }
        }
        prevTime = currTime;
        
        {
            SKY_PROFSCOPE("main_draw");
            BackBuffer.Clear();
            for (auto& a : Apps)
            {
                a->Render(&BackBuffer);
            }
        }

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

bool sWin32Platform::GetKeyState(KeyCode code)
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


