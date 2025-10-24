#pragma once

#include <windows.h>

HWND WIN_WindowCreate(int w, int h, LPWSTR p_caption);
void WIN_InitConsole(void);
void WIN_MainLoop(HWND hwnd);

