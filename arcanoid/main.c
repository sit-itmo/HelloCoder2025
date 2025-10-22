// win_putpixel_demo.c
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

typedef struct {
    HBITMAP     hbm;
    HDC         hdc;        // совместимый DC для бэкбуфера
    void* bits;       // указатель на пиксели
    int         width;
    int         height;
    int         stride;     // байт на строку
    BITMAPINFO  bmi;
} Backbuffer;

static const wchar_t* kClassName = L"PutPixelDemoWndClass";
static Backbuffer g_bb = { 0 };
static bool g_drawing = false;
static int  g_lastX = -1, g_lastY = -1;
static int  g_brushRadius = 2;        // радиус "кисти" в пикселях
static uint32_t g_color = 0xFFFF3C00; // ARGB (A=FF, R=FF, G=3C, B=00) — оранжевый

// --- Утилиты ----------------------------------------------------------------

static void DestroyBackbuffer(void) {
    if (g_bb.hdc) { DeleteDC(g_bb.hdc); g_bb.hdc = NULL; }
    if (g_bb.hbm) { DeleteObject(g_bb.hbm); g_bb.hbm = NULL; }
    g_bb.bits = NULL;
    g_bb.width = g_bb.height = g_bb.stride = 0;
}

static bool CreateBackbuffer(HWND hwnd, int w, int h) {
    if (w <= 0 || h <= 0) return false;
    DestroyBackbuffer();

    ZeroMemory(&g_bb.bmi, sizeof(g_bb.bmi));
    g_bb.bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    g_bb.bmi.bmiHeader.biWidth = w;
    g_bb.bmi.bmiHeader.biHeight = -h;      // top-down DIB
    g_bb.bmi.bmiHeader.biPlanes = 1;
    g_bb.bmi.bmiHeader.biBitCount = 32;      // BGRA 8:8:8:8
    g_bb.bmi.bmiHeader.biCompression = BI_RGB;

    HDC wndDC = GetDC(hwnd);
    g_bb.hdc = CreateCompatibleDC(wndDC);
    g_bb.hbm = CreateDIBSection(wndDC, &g_bb.bmi, DIB_RGB_COLORS, &g_bb.bits, NULL, 0);
    ReleaseDC(hwnd, wndDC);

    if (!g_bb.hdc || !g_bb.hbm || !g_bb.bits) {
        DestroyBackbuffer();
        return false;
    }

    SelectObject(g_bb.hdc, g_bb.hbm);
    g_bb.width = w;
    g_bb.height = h;
    g_bb.stride = w * 4;

    // Заливка фона тёмно-серым
    uint32_t* p = (uint32_t*)g_bb.bits;
    for (int i = 0; i < w * h; ++i) p[i] = 0xFF202020;
    return true;
}

// «putpixel» в бэкбуфер (ARGB, хранение — BGRA; альфа игнорим при копировании)
static inline void PutPixel(int x, int y, uint32_t argb) {
    if (x < 0 || y < 0 || x >= g_bb.width || y >= g_bb.height || !g_bb.bits) return;
    uint8_t a = (uint8_t)(argb >> 24);
    uint8_t r = (uint8_t)(argb >> 16);
    uint8_t g = (uint8_t)(argb >> 8);
    uint8_t b = (uint8_t)(argb >> 0);

    uint8_t* row = (uint8_t*)g_bb.bits + y * g_bb.stride;
    uint32_t* px = (uint32_t*)(row + x * 4);

    if (a == 255) {
        // непрозрачный — просто записываем BGRA
        *px = (uint32_t)(0xFF000000 | (b) | (g << 8) | (r << 16));
    }
    else {
        // простое альфа-смешивание поверх уже имеющегося пикселя
        uint32_t dst = *px;
        uint8_t db = (uint8_t)(dst & 0xFF);
        uint8_t dg = (uint8_t)((dst >> 8) & 0xFF);
        uint8_t dr = (uint8_t)((dst >> 16) & 0xFF);

        uint32_t invA = 255 - a;
        uint8_t rb = (uint8_t)((b * a + db * invA) / 255);
        uint8_t rg = (uint8_t)((g * a + dg * invA) / 255);
        uint8_t rr = (uint8_t)((r * a + dr * invA) / 255);
        *px = 0xFF000000 | rb | (rg << 8) | (rr << 16);
    }
}

static void PutDisk(int cx, int cy, int radius, uint32_t color) {
    if (radius <= 0) { PutPixel(cx, cy, color); return; }
    int r2 = radius * radius;
    int y0 = cy - radius, y1 = cy + radius;
    for (int y = y0; y <= y1; ++y) {
        int dy = y - cy;
        int dx_max_sq = r2 - dy * dy;
        if (dx_max_sq < 0) continue;
        int dx = (int)(sqrt((double)dx_max_sq) + 0.5);
        int x0 = cx - dx, x1 = cx + dx;
        for (int x = x0; x <= x1; ++x) PutPixel(x, y, color);
    }
}

static void DrawLine(int x0, int y0, int x1, int y1, int radius, uint32_t color) {
    // Брезенхем + утолщение диском вдоль линии
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        PutDisk(x0, y0, radius, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

static void ClearBuffer(uint32_t argb) {
    if (!g_bb.bits) return;
    uint8_t r = (uint8_t)(argb >> 16);
    uint8_t g = (uint8_t)(argb >> 8);
    uint8_t b = (uint8_t)(argb >> 0);
    uint32_t bgra = 0xFF000000 | b | (g << 8) | (r << 16);
    uint32_t* p = (uint32_t*)g_bb.bits;
    int count = g_bb.width * g_bb.height;
    for (int i = 0; i < count; ++i) p[i] = bgra;
}

// --- Оконная процедура -------------------------------------------------------

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        RECT rc; GetClientRect(hwnd, &rc);
        CreateBackbuffer(hwnd, rc.right - rc.left, rc.bottom - rc.top);
        return 0;
    }

    case WM_SIZE: {
        int w = LOWORD(lParam), h = HIWORD(lParam);
        if (w > 0 && h > 0) {
            CreateBackbuffer(hwnd, w, h);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_LBUTTONDOWN: {
        SetCapture(hwnd);
        g_drawing = true;
        g_lastX = GET_X_LPARAM(lParam);
        g_lastY = GET_Y_LPARAM(lParam);
        PutDisk(g_lastX, g_lastY, g_brushRadius, g_color);
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_MOUSEMOVE: {
        if (g_drawing) {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            DrawLine(g_lastX, g_lastY, x, y, g_brushRadius, g_color);
            g_lastX = x; g_lastY = y;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_LBUTTONUP: {
        if (g_drawing) {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            DrawLine(g_lastX, g_lastY, x, y, g_brushRadius, g_color);
            g_drawing = false;
            ReleaseCapture();
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_MOUSEWHEEL: {
        short delta = GET_WHEEL_DELTA_WPARAM(wParam);
        if (delta > 0) g_brushRadius++;
        else if (delta < 0 && g_brushRadius > 0) g_brushRadius--;
        return 0;
    }

    case WM_KEYDOWN: {
        switch (wParam) {
        case VK_ESCAPE: PostQuitMessage(0); break;
        case 'C': case 'c': ClearBuffer(0xFF202020); InvalidateRect(hwnd, NULL, FALSE); break;
        case VK_OEM_6: /* ] */ g_brushRadius++; break;
        case VK_OEM_4: /* [ */ if (g_brushRadius > 0) g_brushRadius--; break;
            // Пример смены цвета: 1..3
        case '1': g_color = 0xFFFF3C00; break; // оранжевый
        case '2': g_color = 0xFF00C8FF; break; // голубой
        case '3': g_color = 0xFFFFD500; break; // жёлтый
        }
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        if (g_bb.hdc && g_bb.hbm) {
            BitBlt(hdc, 0, 0, g_bb.width, g_bb.height, g_bb.hdc, 0, 0, SRCCOPY);
        }
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        DestroyBackbuffer();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// --- Точка входа -------------------------------------------------------------

int APIENTRY wWinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEXW wc = { sizeof(wc) };
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_CROSS);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = kClassName;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm = wc.hIcon;

    if (!RegisterClassExW(&wc)) return 0;

    DWORD style = WS_OVERLAPPEDWINDOW;
    RECT r = { 0,0, 1024, 640 };
    AdjustWindowRect(&r, style, FALSE);

    HWND hwnd = CreateWindowExW(
        0, kClassName, L"PutPixel (DIB) — Win32 C",
        style, CW_USEDEFAULT, CW_USEDEFAULT,
        r.right - r.left, r.bottom - r.top,
        NULL, NULL, hInst, NULL);

    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // цикл сообщений
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}
