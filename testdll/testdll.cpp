#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

struct FindBiggestCtx
{
    DWORD pid = 0;
    HWND  bestHwnd = NULL;
    long long bestArea = -1;
};

static bool IsGoodTopLevel(HWND h)
{
    if (!IsWindowVisible(h)) return false;
    if (IsIconic(h)) return false;                 // minimized
    if (GetWindow(h, GW_OWNER) != NULL) return false; // skip owned popups/tool windows

    // Skip "cloaked" windows (some UWP / modern windows), optional:
    // (requires dwmapi; leaving out to keep it minimal)

    RECT r{};
    if (!GetWindowRect(h, &r)) return false;
    int w = r.right - r.left;
    int hgt = r.bottom - r.top;
    if (w <= 0 || hgt <= 0) return false;

    return true;
}

static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    FindBiggestCtx* ctx = (FindBiggestCtx*)lParam;

    DWORD winPid = 0;
    GetWindowThreadProcessId(hwnd, &winPid);
    if (winPid != ctx->pid)
        return TRUE;

    if (!IsGoodTopLevel(hwnd))
        return TRUE;

    RECT r{};
    GetWindowRect(hwnd, &r);
    long long w = (long long)(r.right - r.left);
    long long h = (long long)(r.bottom - r.top);
    long long area = w * h;

    // Prefer TOPMOST windows if present; otherwise pick largest.
    LONG ex = GetWindowLongW(hwnd, GWL_EXSTYLE);
    bool isTopmost = (ex & WS_EX_TOPMOST) != 0;

    LONG bestEx = ctx->bestHwnd ? GetWindowLongW(ctx->bestHwnd, GWL_EXSTYLE) : 0;
    bool bestIsTopmost = ctx->bestHwnd ? ((bestEx & WS_EX_TOPMOST) != 0) : false;

    if (ctx->bestHwnd == NULL)
    {
        ctx->bestHwnd = hwnd;
        ctx->bestArea = area;
        return TRUE;
    }

    // If we already have a topmost, don't replace it with non-topmost.
    if (bestIsTopmost && !isTopmost)
        return TRUE;

    // If the new one is topmost and old isn't, take it.
    if (!bestIsTopmost && isTopmost)
    {
        ctx->bestHwnd = hwnd;
        ctx->bestArea = area;
        return TRUE;
    }

    // Same topmost-ness -> choose biggest area
    if (area > ctx->bestArea)
    {
        ctx->bestHwnd = hwnd;
        ctx->bestArea = area;
    }

    return TRUE;
}

// Returns "best" window in current process (largest visible top-level; prefers WS_EX_TOPMOST)
static HWND FindBiggestWindowInCurrentProcess()
{
    FindBiggestCtx ctx;
    ctx.pid = GetCurrentProcessId();
    EnumWindows(EnumWindowsProc, (LPARAM)&ctx);
    return ctx.bestHwnd;
}

static void MakeWindowTopmost(HWND hwnd)
{
    // Keep position/size, just change z-order to TOPMOST
    SetWindowPos(hwnd, HWND_TOPMOST,
        0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

static void RenderOnWindowOnce(HWND hwnd)
{
    RECT rc{};
    GetClientRect(hwnd, &rc);

    HDC hdc = GetDC(hwnd);
    if (!hdc) return;

    // Example "SetDC..." usage:
    SetDCPenColor(hdc, RGB(0, 255, 0));
    SetDCBrushColor(hdc, RGB(0, 0, 0));

    HPEN hPen = CreatePen(PS_SOLID, 3, RGB(0, 255, 0));
    HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0));

    HGDIOBJ oldPen = SelectObject(hdc, hPen);
    HGDIOBJ oldBrush = SelectObject(hdc, hBrush);

    // Draw a border rectangle in client area
    Rectangle(hdc, 10, 10, rc.right - 10, rc.bottom - 10);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(0, 255, 0));

    const char* msg = "Rendering directly via GetDC (demo).";
    TextOutA(hdc, 20, 20, msg, (int)lstrlenA(msg));

    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(hPen);
    DeleteObject(hBrush);

    ReleaseDC(hwnd, hdc);
}

int main()
{
    HWND hwnd = FindBiggestWindowInCurrentProcess();
    if (!hwnd)
    {
        printf("No suitable top-level window found in this process.\n");
        return 1;
    }

    // Bring it to front & make it topmost (so it won't be hidden by normal windows)
    ShowWindow(hwnd, SW_SHOW);
    MakeWindowTopmost(hwnd);
    SetForegroundWindow(hwnd);

    // Demo render
    RenderOnWindowOnce(hwnd);

    printf("Target hwnd=%p\n", (void*)hwnd);
    Sleep(2000); // just to keep console alive to observe
    return 0;
}

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vector>
#include <algorithm>

static bool IsAncestorOrSelf(HWND ancestor, HWND h)
{
    for (HWND cur = h; cur != NULL; cur = GetParent(cur))
    {
        if (cur == ancestor) return true;
    }
    return false;
}

static bool GetRectArea(const RECT& r, long long* outArea)
{
    long long w = (long long)r.right - r.left;
    long long h = (long long)r.bottom - r.top;
    if (w <= 0 || h <= 0) return false;
    *outArea = w * h;
    return true;
}

static bool IsCandidateBasicVisible(HWND hwnd)
{
    if (!IsWindow(hwnd)) return false;
    if (!IsWindowVisible(hwnd)) return false;

    // If it's a top-level minimized window, ignore.
    // (Child controls can't be minimized by IsIconic, but that's fine.)
    if (GetAncestor(hwnd, GA_ROOT) == hwnd && IsIconic(hwnd)) return false;

    RECT r{};
    if (!GetWindowRect(hwnd, &r)) return false;

    long long area = 0;
    if (!GetRectArea(r, &area)) return false;

    return true;
}

// Checks if hwnd is not covered at sample points.
// If a point belongs to a descendant window of hwnd -> OK.
// If a point returns a window outside hwnd's subtree -> covered -> FAIL.
static bool IsTrulyVisibleNotCovered(HWND hwnd)
{
    RECT r{};
    if (!GetWindowRect(hwnd, &r)) return false;

    // Inset a bit to avoid borders/corners where hit-testing can be weird
    const int inset = 6;
    int left = r.left + inset;
    int top = r.top + inset;
    int right = r.right - inset;
    int bottom = r.bottom - inset;

    if (right <= left || bottom <= top)
        return false;

    // Sample grid: 3x3 points (center + edges-ish)
    const int xs[3] = {
        left + (right - left) / 6,
        left + (right - left) / 2,
        left + (right - left) * 5 / 6
    };
    const int ys[3] = {
        top + (bottom - top) / 6,
        top + (bottom - top) / 2,
        top + (bottom - top) * 5 / 6
    };

    for (int iy = 0; iy < 3; ++iy)
    {
        for (int ix = 0; ix < 3; ++ix)
        {
            POINT pt{ xs[ix], ys[iy] };

            HWND topHwnd = WindowFromPoint(pt);
            if (!topHwnd)
                return false;

            // Important: WindowFromPoint returns the *child* at that point.
            // If it's not within hwnd subtree, hwnd is covered there.
            if (!IsAncestorOrSelf(hwnd, topHwnd))
                return false;
        }
    }

    return true;
}

struct EnumAllCtx
{
    DWORD pid = 0;
    std::vector<HWND> found;
};

static BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lp)
{
    EnumAllCtx* ctx = (EnumAllCtx*)lp;

    DWORD wpid = 0;
    GetWindowThreadProcessId(hwnd, &wpid);
    if (wpid == ctx->pid)
        ctx->found.push_back(hwnd);

    // Recurse further down (EnumChildWindows only enumerates descendants for a given parent,
    // so we call it again per child to be explicit and robust).
    EnumChildWindows(hwnd, EnumChildProc, lp);
    return TRUE;
}

static BOOL CALLBACK EnumTopProc(HWND hwnd, LPARAM lp)
{
    EnumAllCtx* ctx = (EnumAllCtx*)lp;

    DWORD wpid = 0;
    GetWindowThreadProcessId(hwnd, &wpid);
    if (wpid != ctx->pid)
        return TRUE;

    ctx->found.push_back(hwnd);

    // Collect all descendant windows (controls, etc.)
    EnumChildWindows(hwnd, EnumChildProc, lp);
    return TRUE;
}

// Main: returns the largest HWND in PID that is visible and not covered.
HWND FindBestVisibleWindowInProcess(DWORD pid)
{
    EnumAllCtx ctx;
    ctx.pid = pid;

    EnumWindows(EnumTopProc, (LPARAM)&ctx);

    // Remove duplicates (can happen because of recursion approach)
    std::sort(ctx.found.begin(), ctx.found.end());
    ctx.found.erase(std::unique(ctx.found.begin(), ctx.found.end()), ctx.found.end());

    HWND best = NULL;
    long long bestArea = -1;

    for (HWND h : ctx.found)
    {
        if (!IsCandidateBasicVisible(h))
            continue;

        // Must be actually visible (not covered) at sample points
        if (!IsTrulyVisibleNotCovered(h))
            continue;

        RECT r{};
        GetWindowRect(h, &r);

        long long area = 0;
        if (!GetRectArea(r, &area))
            continue;

        if (area > bestArea)
        {
            bestArea = area;
            best = h;
        }
    }

    return best;
}

#include "Skybound.h"
#include <windows.h>
#include <shlwapi.h>   // PathRemoveFileSpec
#pragma comment(lib, "Shlwapi.lib")


BOOL APIENTRY DllMain(
    HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    HWND hwnd = 0;
    DWORD len = 0;
    CHAR exePath[MAX_PATH];

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        MessageBoxA(
            NULL,
            "Hello from DLL!\nDLL_PROCESS_ATTACH fired.",
            "DLL Loaded",
            MB_OK | MB_ICONINFORMATION
        );
        len = GetModuleFileNameA(hModule, exePath, MAX_PATH);
        if (len != 0 && len != MAX_PATH)
        {
            PathRemoveFileSpecA(exePath);
        }

        hwnd = FindBestVisibleWindowInProcess(GetCurrentProcessId());
        if (hwnd == NULL)
        {
            hwnd = FindBiggestWindowInCurrentProcess();
        }
        RenderOnWindowOnce(hwnd);
        Skybound::getSingleton()->Start((long long)hwnd, exePath);
        Skybound::DESTROY();

        break;

    case DLL_PROCESS_DETACH:
        // Optional cleanup
        break;
    }
    return TRUE;
}