#include <stdio.h>
#include <windows.h>
#include <strsafe.h>

int PureAsciiTest(int flags)
{
    CHAR buf[128] = { 0 };
    CHAR mod_name[128] = { 0 };
    HANDLE h_proc = GetCurrentProcess();
    HMODULE h_mod = GetModuleHandleA(NULL);
    GetModuleFileNameA(h_mod, mod_name, sizeof(mod_name));

    snprintf(buf, sizeof(buf) - 1,
        "This is an example of MessageBox\n"
        "Fired from process ProcessID=%d\n"
        "ProcessName: %s!", GetProcessId(h_proc), mod_name);
    return MessageBoxA(NULL, buf, "Example", flags);
}

int PureUnicodeTest(int flags)
{
    WCHAR buf[128] = { 0 };
    WCHAR mod_name[128] = { 0 };
    HANDLE h_proc = GetCurrentProcess();
    HMODULE h_mod = GetModuleHandleW(NULL);
    GetModuleFileNameW(h_mod, mod_name, sizeof(mod_name));

    _snwprintf(buf, sizeof(buf)/sizeof(buf[0]) - 1,
        L"This is an example of MessageBox\n"
        L"Fired from process ProcessID=%d\n"
        L"ProcessName: %s!", GetProcessId(h_proc), mod_name);
    return MessageBoxW(NULL, buf, L"Example", flags);
}

int PureBothTest(int flags)
{
    TCHAR buf[128] = { 0 };
    TCHAR mod_name[128] = { 0 };
    HANDLE h_proc = GetCurrentProcess();
    HMODULE h_mod = GetModuleHandle(NULL);
    GetModuleFileName(h_mod, mod_name, sizeof(mod_name));
    LPCTSTR args[] = {
        (LPCTSTR)(ULONG_PTR)GetProcessId(h_proc),
        mod_name
    };

#if 0
    FormatMessage(
        FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
        TEXT("This is an example of MessageBox\n")
        TEXT("Fired from process ProcessID=%1!u!\n")
        TEXT("ProcessName: %2!s!"),
        0, // dwFlags
        0, // dwLanguageId
        buf, // lpBuffer
        sizeof(buf) / sizeof(buf[0]), // nSize
        (va_list*)args
    );

#else
#ifdef _UNICODE
    _snwprintf
#else
    snprintf
#endif
        (buf, sizeof(buf)/ sizeof(buf[0]) - 1,
            TEXT("This is an example of MessageBox\n")
            TEXT("Fired from process ProcessID=%d\n")
            TEXT("ProcessName: %s!"), GetProcessId(h_proc), mod_name);
#endif

    return MessageBox(NULL, buf, TEXT("Example"), flags);
}

void FileTest()
{
    CHAR buf[64];
    HANDLE h_mod = GetModuleHandleA(NULL);
    GetModuleFileNameA(NULL, buf, sizeof(buf));

    FILE* p_file = fopen("d:\\test_lnx.txt", "wt");
    fprintf(p_file, "\nTest Hello %s!", buf);
    fclose(p_file);

    HANDLE h_file = CreateFileW(L"d:\\test_win.txt", (GENERIC_READ | GENERIC_WRITE),
        0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    CHAR buf_to_write[64];
    DWORD written = 0;
    int chars = snprintf(buf_to_write, sizeof(buf_to_write), "\nTest Hello %s!", buf);
    WriteFile(h_file, buf_to_write, chars, &written, NULL);
    
    CloseHandle(h_file);

}



void FileTestDelay()
{
    HANDLE h_file = NULL;
    CHAR buf[64];
    int chars = 0;
    CHAR buf_to_write[64];
    DWORD written = 0;
    BOOL res = FALSE;
    HANDLE h_mod = GetModuleHandleA(NULL);
    GetModuleFileNameA(NULL, buf, sizeof(buf));
    OVERLAPPED ov = {};
    ov.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL); // manual-reset
    if (!ov.hEvent)
    {
        goto END;
    }

    h_file = CreateFileW(L"d:\\test_win2.txt", (GENERIC_READ | GENERIC_WRITE),
        0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);

    chars = snprintf(buf_to_write, sizeof(buf_to_write), "\nTest Hello %s!", buf);

    res = WriteFile(h_file, buf_to_write, chars, NULL, &ov);
    if (!res)
    {
        DWORD err = GetLastError();
        if (err != ERROR_IO_PENDING)
        {
            goto END;
        }

        WaitForSingleObject(ov.hEvent, INFINITE);
        GetOverlappedResult(h_file, &ov, &written, FALSE);
    }

END:
    if (ov.hEvent != NULL)
    {
        CloseHandle(ov.hEvent);
        ov.hEvent = NULL;
    }
    if (h_file != NULL)
    {
        CloseHandle(h_file);
        h_file = NULL;
    }
}


int WINAPI WinMain_0(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nShowCmd
)
{
    //FileTest();

    FileTestDelay();

    //PureAsciiTest(MB_OK);
    //PureUnicodeTest(MB_OK);
    //PureBothTest(MB_OK);

    char bufa[128] = { 0 };
    snprintf(bufa, sizeof(bufa), "\nTest1 %s\nTest2 %S\nTest3 %ws", "Ascii", L"Unicode", L"Uncode");
    wchar_t bufw[128] = { 0 };
    _snwprintf(bufw, sizeof(bufw), L"\nTest1 %s\nTest2 %S\nTest3 %ws", L"Unicode", "Ascii", L"Uncode");

    TCHAR buf[64];
    HRESULT hr = StringCchPrintf(
        buf,
        _countof(buf),
        TEXT("Process ID: %u"),
        GetCurrentProcessId()
    );
    SUCCEEDED(hr);

    auto func = PureBothTest;
    while (1)
    {
        int res = func(MB_YESNO);
        snprintf(bufa, sizeof(bufa), "Result Was: %d", res);
        MessageBoxA(NULL, bufa, "RESULT", MB_OK | MB_ICONHAND);
    }

    PureBothTest(MB_OK);
    PureAsciiTest(MB_OK);
    PureUnicodeTest(MB_OK);
    printf("Hello World!");
    return 0;
}

