#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include "resource.h"

#define IDC_FIRST_BUTTON 1000
#define IDC_OPEN_DIALOG  2000

static int g_buttonCount = 1;
static int g_nextY = 60;

static INT_PTR CALLBACK MyDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    (void)lParam;

    switch (msg)
    {
    case WM_INITDIALOG:
    {
        SetDlgItemTextA(hDlg, IDC_EDIT_NAME, "Default name");
        CheckDlgButton(hDlg, IDC_CHECK_ENABLE, BST_UNCHECKED);
        return (INT_PTR)TRUE;
    }

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_BTN_APPLY:
        {
            char name[256] = { 0 };
            GetDlgItemTextA(hDlg, IDC_EDIT_NAME, name, (int)sizeof(name));

            BOOL enabled = (IsDlgButtonChecked(hDlg, IDC_CHECK_ENABLE) == BST_CHECKED);

            char buf[512];
            wsprintfA(buf, "Apply clicked!\nName: %s\nEnabled: %s",
                name, enabled ? "YES" : "NO");
            MessageBoxA(hDlg, buf, "Dialog", MB_OK | MB_ICONINFORMATION);
            return (INT_PTR)TRUE;
        }

        case IDOK:
        {
            // Collect values and close dialog with IDOK
            char name[256] = { 0 };
            GetDlgItemTextA(hDlg, IDC_EDIT_NAME, name, (int)sizeof(name));
            BOOL enabled = (IsDlgButtonChecked(hDlg, IDC_CHECK_ENABLE) == BST_CHECKED);

            // Example: do something with these values before closing
            // (here we just show them)
            char buf[512];
            wsprintfA(buf, "OK pressed.\nName: %s\nEnabled: %s",
                name, enabled ? "YES" : "NO");
            MessageBoxA(hDlg, buf, "Dialog Result", MB_OK);

            EndDialog(hDlg, IDOK);
            return (INT_PTR)TRUE;
        }

        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)TRUE;
        }
        break;
    }
    }

    return (INT_PTR)FALSE;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        // First "spawn buttons" button
        CreateWindowExA(
            0, "BUTTON", "Create Button",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            20, 20, 150, 30,
            hwnd, (HMENU)IDC_FIRST_BUTTON,
            GetModuleHandle(NULL), NULL
        );

        // Special button that opens resource dialog
        CreateWindowExA(
            0, "BUTTON", "Open Dialog...",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            200, 20, 150, 30,
            hwnd, (HMENU)IDC_OPEN_DIALOG,
            GetModuleHandle(NULL), NULL
        );
        break;
    }

    case WM_COMMAND:
    {
        if (HIWORD(wParam) == BN_CLICKED)
        {
            int id = LOWORD(wParam);

            if (id == IDC_OPEN_DIALOG)
            {
                DialogBoxParamA(
                    GetModuleHandle(NULL),
                    MAKEINTRESOURCEA(IDD_MYDIALOG),
                    hwnd,
                    MyDialogProc,
                    0
                );
                return 0;
            }

            // Any other button click => spawn another button
            {
                char text[64];
                wsprintfA(text, "Button %d", g_buttonCount + 1);

                CreateWindowExA(
                    0, "BUTTON", text,
                    WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                    20, g_nextY, 150, 30,
                    hwnd,
                    (HMENU)(IDC_FIRST_BUTTON + g_buttonCount),
                    GetModuleHandle(NULL),
                    NULL
                );

                g_buttonCount++;
                g_nextY += 40;
            }
        }
        break;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    (void)hPrevInstance; (void)lpCmdLine;

    const char CLASS_NAME[] = "SimpleWindowClass";

    WNDCLASSA wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassA(&wc);

    HWND hwnd = CreateWindowExA(
        0,
        CLASS_NAME,
        "WinAPI: Buttons + Resource Dialog",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        480, 420,
        NULL, NULL,
        hInstance,
        NULL
    );

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
