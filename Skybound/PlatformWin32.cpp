#include <windows.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>

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

