#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

static void PrintLastErrorA(const char* where)
{
    DWORD e = GetLastError();
    CHAR msg[512] = { 0 };
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, e, 0, msg, (DWORD)sizeof(msg), NULL);
    printf("%s failed: (%lu) %s\n", where, e, msg);
}

int WINAPI WinMain_pipes(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nShowCmd
)
{
    // \\.\pipe\demo_overlapped_cancelio
    const wchar_t* PIPE_NAME = L"\\\\.\\pipe\\demo_overlapped_cancelio";

    // 1) Create OVERLAPPED pipe server (inbound only, to read from client)
    HANDLE hPipeServer = CreateNamedPipeW(
        PIPE_NAME,
        PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED, // OVERLAPPED server handle
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        1,              // max instances
        0,              // out buffer (unused - inbound)
        4096,           // in buffer
        0,
        NULL);

    if (hPipeServer == INVALID_HANDLE_VALUE)
    {
        PrintLastErrorA("CreateNamedPipeW");
        return 1;
    }

    // Overlapped for ConnectNamedPipe
    OVERLAPPED ovConn = {};
    ovConn.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!ovConn.hEvent)
    {
        PrintLastErrorA("CreateEventW(ovConn)");
        CloseHandle(hPipeServer);
        return 1;
    }

    // 2) Start async connect
    BOOL ok = ConnectNamedPipe(hPipeServer, &ovConn);
    if (!ok)
    {
        DWORD err = GetLastError();
        if (err != ERROR_IO_PENDING && err != ERROR_PIPE_CONNECTED)
        {
            PrintLastErrorA("ConnectNamedPipe");
            CloseHandle(ovConn.hEvent);
            CloseHandle(hPipeServer);
            return 1;
        }
    }

    // 3) Open client end (writer)
    HANDLE hPipeClient = CreateFileW(
        PIPE_NAME,
        GENERIC_WRITE,       // write only
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, // (client side can be sync here)
        NULL);

    if (hPipeClient == INVALID_HANDLE_VALUE)
    {
        PrintLastErrorA("CreateFileW(client)");
        CloseHandle(ovConn.hEvent);
        CloseHandle(hPipeServer);
        return 1;
    }

    // Wait for connect to complete (if pending)
    if (GetLastError() == ERROR_IO_PENDING)
    {
        WaitForSingleObject(ovConn.hEvent, INFINITE);
        DWORD dummy = 0;
        if (!GetOverlappedResult(hPipeServer, &ovConn, &dummy, FALSE))
        {
            PrintLastErrorA("GetOverlappedResult(connect)");
            CloseHandle(hPipeClient);
            CloseHandle(ovConn.hEvent);
            CloseHandle(hPipeServer);
            return 1;
        }
    }
    else
    {
        // If ConnectNamedPipe returned ERROR_PIPE_CONNECTED, the client already connected.
        // That's OK.
    }

    printf("Client connected.\n");

    // 4) Issue OVERLAPPED ReadFile on server, then cancel it with CancelIo
    OVERLAPPED ovRead1 = {};
    ovRead1.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!ovRead1.hEvent)
    {
        PrintLastErrorA("CreateEventW(ovRead1)");
        CloseHandle(hPipeClient);
        CloseHandle(ovConn.hEvent);
        CloseHandle(hPipeServer);
        return 1;
    }

    char buf[256] = {};
    ResetEvent(ovRead1.hEvent);

    ok = ReadFile(hPipeServer, buf, sizeof(buf) - 1, NULL, &ovRead1);
    if (!ok && GetLastError() != ERROR_IO_PENDING)
    {
        PrintLastErrorA("ReadFile(overlapped #1)");
        CloseHandle(ovRead1.hEvent);
        CloseHandle(hPipeClient);
        CloseHandle(ovConn.hEvent);
        CloseHandle(hPipeServer);
        return 1;
    }

    printf("Overlapped read #1 pending. Now canceling with CancelIo...\n");

    // Cancel I/O issued by THIS thread on hPipeServer
    if (!CancelIo(hPipeServer))
        PrintLastErrorA("CancelIo");

    // Wait for the cancelled op to be signaled
    WaitForSingleObject(ovRead1.hEvent, INFINITE);

    DWORD bytesRead1 = 0;
    if (!GetOverlappedResult(hPipeServer, &ovRead1, &bytesRead1, FALSE))
    {
        DWORD e = GetLastError();
        if (e == ERROR_OPERATION_ABORTED)
            printf("Read #1 cancelled: ERROR_OPERATION_ABORTED\n");
        else
            PrintLastErrorA("GetOverlappedResult(read #1)");
    }
    else
    {
        printf("Read #1 unexpectedly completed. bytes=%lu\n", bytesRead1);
    }

    // 5) Issue a second overlapped read, then have client write data to complete it
    OVERLAPPED ovRead2 = {};
    ovRead2.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!ovRead2.hEvent)
    {
        PrintLastErrorA("CreateEventW(ovRead2)");
        CloseHandle(ovRead1.hEvent);
        CloseHandle(hPipeClient);
        CloseHandle(ovConn.hEvent);
        CloseHandle(hPipeServer);
        return 1;
    }

    ZeroMemory(buf, sizeof(buf));
    ResetEvent(ovRead2.hEvent);

    ok = ReadFile(hPipeServer, buf, sizeof(buf) - 1, NULL, &ovRead2);
    if (!ok && GetLastError() != ERROR_IO_PENDING)
    {
        PrintLastErrorA("ReadFile(overlapped #2)");
        CloseHandle(ovRead2.hEvent);
        CloseHandle(ovRead1.hEvent);
        CloseHandle(hPipeClient);
        CloseHandle(ovConn.hEvent);
        CloseHandle(hPipeServer);
        return 1;
    }

    const char* msg = "Hello from pipe client!\r\n";
    DWORD written = 0;
    if (!WriteFile(hPipeClient, msg, (DWORD)lstrlenA(msg), &written, NULL))
    {
        PrintLastErrorA("WriteFile(client)");
    }
    else
    {
        printf("Client wrote %lu bytes.\n", written);
    }

    WaitForSingleObject(ovRead2.hEvent, INFINITE);

    DWORD bytesRead2 = 0;
    if (!GetOverlappedResult(hPipeServer, &ovRead2, &bytesRead2, FALSE))
    {
        PrintLastErrorA("GetOverlappedResult(read #2)");
    }
    else
    {
        buf[bytesRead2] = 0;
        printf("Server read #2 completed. bytes=%lu, data:\n%s\n", bytesRead2, buf);
    }

    // Cleanup
    CloseHandle(ovRead2.hEvent);
    CloseHandle(ovRead1.hEvent);
    CloseHandle(hPipeClient);

    // Disconnect & close server
    DisconnectNamedPipe(hPipeServer);
    CloseHandle(ovConn.hEvent);
    CloseHandle(hPipeServer);
    return 0;
}
