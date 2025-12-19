// sample_tool.c - user-mode test tool for \\.\sample
// Build: cl /W4 sample_tool.c

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <winioctl.h>

// Must match driver
#define FILE_DEVICE_SAMPLE  0x8000

#define IOCTL_SAMPLE_GET_VERSION \
    CTL_CODE(FILE_DEVICE_SAMPLE, 0x801, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_SAMPLE_CLEAR_BUFFER \
    CTL_CODE(FILE_DEVICE_SAMPLE, 0x802, METHOD_BUFFERED, FILE_WRITE_ACCESS)

static void PrintLastError(const char* msg)
{
    DWORD e = GetLastError();
    char buf[512];
    snprintf(buf, sizeof(buf), "%s (GetLastError=%lu)\n", msg, (unsigned long)e);
    fputs(buf, stderr);
}

int main(void)
{
    HANDLE h = CreateFileA(
        "\\\\.\\sample",
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (h == INVALID_HANDLE_VALUE) {
        PrintLastError("CreateFile(\\\\.\\sample) failed");
        return 1;
    }

    // --- IOCTL: get version
    DWORD version = 0;
    DWORD bytes = 0;
    if (!DeviceIoControl(h, IOCTL_SAMPLE_GET_VERSION,
        NULL, 0,
        &version, sizeof(version),
        &bytes, NULL))
    {
        PrintLastError("DeviceIoControl(GET_VERSION) failed");
        CloseHandle(h);
        return 1;
    }
    printf("Driver version: 0x%08lX (bytes=%lu)\n", (unsigned long)version, (unsigned long)bytes);

    // --- Write some text
    const char* msg = "Hello from user-mode! Echo me back.\n";
    DWORD written = 0;
    if (!WriteFile(h, msg, (DWORD)strlen(msg), &written, NULL)) {
        PrintLastError("WriteFile failed");
        CloseHandle(h);
        return 1;
    }
    printf("Wrote %lu bytes\n", (unsigned long)written);

    // --- Read it back
    char readBuf[256] = { 0 };
    DWORD read = 0;
    if (!ReadFile(h, readBuf, sizeof(readBuf) - 1, &read, NULL)) {
        PrintLastError("ReadFile failed");
        CloseHandle(h);
        return 1;
    }
    printf("Read %lu bytes:\n%s\n", (unsigned long)read, readBuf);

    // --- IOCTL: clear buffer
    if (!DeviceIoControl(h, IOCTL_SAMPLE_CLEAR_BUFFER,
        NULL, 0,
        NULL, 0,
        &bytes, NULL))
    {
        PrintLastError("DeviceIoControl(CLEAR_BUFFER) failed");
        CloseHandle(h);
        return 1;
    }
    printf("Buffer cleared.\n");

    CloseHandle(h);
    return 0;
}
