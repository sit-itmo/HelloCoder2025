// GameSvc.c - Sample Windows Service (EXE) with Named Pipe IPC (WinAPI, C)
//
// Pipe: \\.\pipe\GameSvc
// Protocol: simple request/response headers + payload.
// Commands: ECHO, GAME_START, GAME_STOP, GAME_PUT_KEY, GAME_GET_SCREEN
//
// Build (Developer Command Prompt):
//   cl /W4 /DUNICODE /D_UNICODE GameSvc.c /link advapi32.lib
//
// Run as service via SCM (use installer tool below or sc.exe).
// For debugging in console (optional): GameSvc.exe --console

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <strsafe.h>

#define SVC_NAME            L"GameSvc"
#define PIPE_NAME           L"\\\\.\\pipe\\GameSvc"
#define PIPE_BUFSZ          (64 * 1024)

typedef enum _GAME_CMD {
    CMD_ECHO = 1,
    CMD_GAME_START = 2,
    CMD_GAME_STOP = 3,
    CMD_GAME_PUT_KEY = 4,
    CMD_GAME_GET_SCREEN = 5
} GAME_CMD;

#pragma pack(push, 1)
typedef struct _REQ_HDR {
    DWORD Magic;      // 'GSV1'
    DWORD Cmd;        // GAME_CMD
    DWORD PayloadLen; // bytes following
} REQ_HDR;

typedef struct _RSP_HDR {
    DWORD Magic;      // 'GSV1'
    DWORD Status;     // 0=OK, Win32 error otherwise
    DWORD PayloadLen; // bytes following
} RSP_HDR;
#pragma pack(pop)

static SERVICE_STATUS          gSvcStatus;
static SERVICE_STATUS_HANDLE   gSvcStatusHandle = NULL;
static HANDLE                  gStopEvent = NULL;
static HANDLE                  gPipeThread = NULL;

static CRITICAL_SECTION        gGameLock;
static BOOL                    gGameRunning = FALSE;
static DWORD                   gLastKey = 0;

// "Screen" buffer returned by GameGetScreen
static CHAR                    gScreen[2048];
static DWORD                   gScreenLen = 0;

static DWORD MAGIC_GSV1 = 0x31565347; // 'GSV1' little-endian
static HANDLE gGameThread = NULL;
static HANDLE gGameWakeEvent = NULL;  // optional: wake loop when GameStart happens

static DWORD WINAPI GameLoopThread(LPVOID unused)
{
    (void)unused;

    DWORD frame = 0;

    for (;;)
    {
        // Exit if service is stopping
        if (WaitForSingleObject(gStopEvent, 0) == WAIT_OBJECT_0)
            break;

        // Optional: sleep/wake logic
        // If you want the loop to run only when game is "running":
        BOOL running = FALSE;
        EnterCriticalSection(&gGameLock);
        running = gGameRunning;
        LeaveCriticalSection(&gGameLock);

        if (!running)
        {
            // Wait until either stop is requested or someone wakes us (GameStart)
            HANDLE waits[2] = { gStopEvent, gGameWakeEvent };
            DWORD w = WaitForMultipleObjects(2, waits, FALSE, 250); // periodic wake
            if (w == WAIT_OBJECT_0) break; // stop
            continue; // still not running or woke up, loop continues
        }

        // --- Generate "screen" buffer for GameGetScreen ---
        // Keep it short/fast; do heavy work outside the lock.
        CHAR tmp[2048];
        DWORD lastKey = 0;

        EnterCriticalSection(&gGameLock);
        lastKey = gLastKey;
        LeaveCriticalSection(&gGameLock);

        // Example "frame" content
        // You can replace this with your real renderer.
        StringCchPrintfA(
            tmp, sizeof(tmp),
            "Frame: %lu\r\nLastKey: %lu (0x%08lX)\r\nStatus: RUNNING\r\n",
            frame++, lastKey, lastKey
        );

        // Publish it atomically under the lock
        EnterCriticalSection(&gGameLock);
        {
            size_t n = 0;
            if (SUCCEEDED(StringCchLengthA(tmp, sizeof(tmp), &n)))
            {
                if (n > sizeof(gScreen)) n = sizeof(gScreen);
                memcpy(gScreen, tmp, n);
                gScreenLen = (DWORD)n;
            }
            else
            {
                gScreenLen = 0;
            }
        }
        LeaveCriticalSection(&gGameLock);

        Sleep(33); // ~30 FPS. adjust as needed.
    }

    return 0;
}

static void LogScreenLineA(const char* line)
{
    EnterCriticalSection(&gGameLock);
    {
        // append line to gScreen with simple truncation
        size_t cur = (size_t)gScreenLen;
        size_t cap = sizeof(gScreen);
        size_t add = 0;
        if (FAILED(StringCchLengthA(line, 4096, &add))) add = 0;

        // plus CRLF
        if (cur + add + 2 >= cap) {
            // naive truncate: reset buffer
            cur = 0;
        }

        if (add > 0 && cur < cap) {
            memcpy(gScreen + cur, line, add);
            cur += add;
        }
        if (cur + 2 < cap) {
            gScreen[cur++] = '\r';
            gScreen[cur++] = '\n';
        }
        gScreenLen = (DWORD)cur;
    }
    LeaveCriticalSection(&gGameLock);
}

// --- Game "internal" functions (requested names/behavior) ---
static DWORD GameStart(void)
{
    EnterCriticalSection(&gGameLock);
    gGameRunning = TRUE;
    LeaveCriticalSection(&gGameLock);

    if (gGameWakeEvent) SetEvent(gGameWakeEvent);

    LogScreenLineA("[GameStart] Game running = TRUE");
    return 0;
}

static DWORD GameStop(void)
{
    EnterCriticalSection(&gGameLock);
    gGameRunning = FALSE;
    LeaveCriticalSection(&gGameLock);

    LogScreenLineA("[GameStop] Game running = FALSE");
    return 0;
}

static DWORD GamePutKey(DWORD key)
{
    EnterCriticalSection(&gGameLock);
    gLastKey = key;
    LeaveCriticalSection(&gGameLock);

    CHAR buf[128];
    StringCchPrintfA(buf, 128, "[GamePutKey] key=%lu (0x%08lX)", key, key);
    LogScreenLineA(buf);
    return 0;
}

// Returns buffer via out pointers (service will copy into response payload)
static DWORD GameGetScreen(const BYTE** outBuf, DWORD* outLen)
{
    if (!outBuf || !outLen) return ERROR_INVALID_PARAMETER;

    EnterCriticalSection(&gGameLock);
    *outBuf = (const BYTE*)gScreen;
    *outLen = gScreenLen;
    LeaveCriticalSection(&gGameLock);

    return 0;
}

// ------------------------------------------------------------

static void SvcSetStatus(DWORD state, DWORD win32ExitCode, DWORD waitHintMs)
{
    gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    gSvcStatus.dwCurrentState = state;
    gSvcStatus.dwWin32ExitCode = win32ExitCode;
    gSvcStatus.dwWaitHint = waitHintMs;

    if (state == SERVICE_START_PENDING) {
        gSvcStatus.dwControlsAccepted = 0;
    }
    else {
        gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    }

    SetServiceStatus(gSvcStatusHandle, &gSvcStatus);
}

static DWORD WINAPI PipeClientThread(LPVOID param)
{
    HANDLE hPipe = (HANDLE)param;
    BYTE* inBuf = (BYTE*)HeapAlloc(GetProcessHeap(), 0, PIPE_BUFSZ);
    BYTE* outBuf = (BYTE*)HeapAlloc(GetProcessHeap(), 0, PIPE_BUFSZ);

    if (!inBuf || !outBuf) {
        if (inBuf) HeapFree(GetProcessHeap(), 0, inBuf);
        if (outBuf) HeapFree(GetProcessHeap(), 0, outBuf);
        FlushFileBuffers(hPipe);
        DisconnectNamedPipe(hPipe);
        CloseHandle(hPipe);
        return 0;
    }

    for (;;) {
        DWORD got = 0;
        BOOL ok = ReadFile(hPipe, inBuf, PIPE_BUFSZ, &got, NULL);
        if (!ok || got < sizeof(REQ_HDR)) break;

        REQ_HDR* req = (REQ_HDR*)inBuf;
        if (req->Magic != MAGIC_GSV1) break;
        if (req->PayloadLen > (PIPE_BUFSZ - sizeof(REQ_HDR))) break;
        if (got < sizeof(REQ_HDR) + req->PayloadLen) break;

        DWORD status = 0;
        DWORD rspPayloadLen = 0;

        // Response payload will be written after RSP_HDR into outBuf
        RSP_HDR* rsp = (RSP_HDR*)outBuf;
        BYTE* rspPayload = outBuf + sizeof(RSP_HDR);

        switch ((DWORD)req->Cmd) {
        case CMD_ECHO:
            // payload is echoed back
            rspPayloadLen = req->PayloadLen;
            if (rspPayloadLen) memcpy(rspPayload, inBuf + sizeof(REQ_HDR), rspPayloadLen);
            status = 0;
            break;

        case CMD_GAME_START:
            status = GameStart();
            rspPayloadLen = 0;
            break;

        case CMD_GAME_STOP:
            status = GameStop();
            rspPayloadLen = 0;
            break;

        case CMD_GAME_PUT_KEY:
            if (req->PayloadLen != sizeof(DWORD)) {
                status = ERROR_INVALID_PARAMETER;
            }
            else {
                DWORD key = *(DWORD*)(inBuf + sizeof(REQ_HDR));
                status = GamePutKey(key);
            }
            rspPayloadLen = 0;
            break;

        case CMD_GAME_GET_SCREEN:
        {
            const BYTE* p = NULL;
            DWORD n = 0;
            status = GameGetScreen(&p, &n);
            if (status == 0) {
                // Cap to buffer size
                if (n > (PIPE_BUFSZ - sizeof(RSP_HDR))) n = (PIPE_BUFSZ - sizeof(RSP_HDR));
                rspPayloadLen = n;
                if (n) memcpy(rspPayload, p, n);
            }
        } break;

        default:
            status = ERROR_NOT_SUPPORTED;
            rspPayloadLen = 0;
            break;
        }

        rsp->Magic = MAGIC_GSV1;
        rsp->Status = status;
        rsp->PayloadLen = rspPayloadLen;

        DWORD toWrite = sizeof(RSP_HDR) + rspPayloadLen;
        DWORD wrote = 0;
        ok = WriteFile(hPipe, outBuf, toWrite, &wrote, NULL);
        if (!ok || wrote != toWrite) break;
    }

    HeapFree(GetProcessHeap(), 0, inBuf);
    HeapFree(GetProcessHeap(), 0, outBuf);

    FlushFileBuffers(hPipe);
    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);
    return 0;
}

static DWORD WINAPI PipeServerThread(LPVOID unused)
{
    (void)unused;

    for (;;) {
        if (WaitForSingleObject(gStopEvent, 0) == WAIT_OBJECT_0) break;

        HANDLE hPipe = CreateNamedPipeW(
            PIPE_NAME,
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            PIPE_BUFSZ,
            PIPE_BUFSZ,
            0,
            NULL);

        if (hPipe == INVALID_HANDLE_VALUE) {
            Sleep(250);
            continue;
        }

        // Wait for a client or stop
        BOOL connected = FALSE;
        for (;;) {
            if (WaitForSingleObject(gStopEvent, 0) == WAIT_OBJECT_0) {
                CloseHandle(hPipe);
                return 0;
            }

            connected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
            if (connected) break;
            Sleep(50);
        }

        // Handle each client in its own thread; keep accepting new clients.
        HANDLE hClient = CreateThread(NULL, 0, PipeClientThread, hPipe, 0, NULL);
        if (hClient) CloseHandle(hClient);
        else {
            // fallback: close
            DisconnectNamedPipe(hPipe);
            CloseHandle(hPipe);
        }
    }

    return 0;
}

static DWORD WINAPI SvcCtrlHandlerEx(DWORD ctrl, DWORD evtType, LPVOID evtData, LPVOID ctx)
{
    (void)evtType; (void)evtData; (void)ctx;

    switch (ctrl) {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        if (gSvcStatus.dwCurrentState == SERVICE_RUNNING) {
            SvcSetStatus(SERVICE_STOP_PENDING, 0, 2000);
            SetEvent(gStopEvent);
        }
        return NO_ERROR;
    default:
        return NO_ERROR;
    }
}

static void WINAPI SvcMain(DWORD argc, LPWSTR* argv)
{
    (void)argc; (void)argv;

    InitializeCriticalSection(&gGameLock);
    gScreenLen = 0;
    LogScreenLineA("[Service] Starting...");

    gSvcStatusHandle = RegisterServiceCtrlHandlerExW(SVC_NAME, SvcCtrlHandlerEx, NULL);
    if (!gSvcStatusHandle) return;

    gStopEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!gStopEvent) {
        SvcSetStatus(SERVICE_STOPPED, GetLastError(), 0);
        return;
    }

    gGameWakeEvent = CreateEventW(NULL, FALSE, FALSE, NULL); // auto-reset
    if (!gGameWakeEvent) {
        SvcSetStatus(SERVICE_STOPPED, GetLastError(), 0);
        CloseHandle(gStopEvent);
        gStopEvent = NULL;
        return;
    }

    gGameThread = CreateThread(NULL, 0, GameLoopThread, NULL, 0, NULL);
    if (!gGameThread) {
        SvcSetStatus(SERVICE_STOPPED, GetLastError(), 0);
        CloseHandle(gGameWakeEvent);
        gGameWakeEvent = NULL;
        CloseHandle(gStopEvent);
        gStopEvent = NULL;
        return;
    }

    SvcSetStatus(SERVICE_START_PENDING, 0, 2000);

    gPipeThread = CreateThread(NULL, 0, PipeServerThread, NULL, 0, NULL);
    if (!gPipeThread) {
        SvcSetStatus(SERVICE_STOPPED, GetLastError(), 0);
        CloseHandle(gStopEvent);
        gStopEvent = NULL;
        return;
    }

    SvcSetStatus(SERVICE_RUNNING, 0, 0);
    LogScreenLineA("[Service] Running. Pipe ready: \\\\.\\pipe\\GameSvc");

    WaitForSingleObject(gStopEvent, INFINITE);


    // Stop sequence
    LogScreenLineA("[Service] Stopping...");
    if (gGameThread) {
        WaitForSingleObject(gGameThread, 5000);
        CloseHandle(gGameThread);
        gGameThread = NULL;
    }
    if (gGameWakeEvent) {
        CloseHandle(gGameWakeEvent);
        gGameWakeEvent = NULL;
    }

    if (gPipeThread) {
        WaitForSingleObject(gPipeThread, 5000);
        CloseHandle(gPipeThread);
        gPipeThread = NULL;
    }

    if (gStopEvent) {
        CloseHandle(gStopEvent);
        gStopEvent = NULL;
    }

    DeleteCriticalSection(&gGameLock);

    SvcSetStatus(SERVICE_STOPPED, 0, 0);
}

// Ctrl+C handler
BOOL WINAPI ConsoleCtrl(DWORD type) {
    if (type == CTRL_C_EVENT || type == CTRL_BREAK_EVENT || type == CTRL_CLOSE_EVENT) {
        SetEvent(gStopEvent);
        return TRUE;
    }
    return FALSE;
}

static int RunAsConsole(void)
{
    wprintf(L"[Console] Starting pipe server. Press Ctrl+C to exit.\n");
    InitializeCriticalSection(&gGameLock);
    gStopEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!gStopEvent) return 2;

    gPipeThread = CreateThread(NULL, 0, PipeServerThread, NULL, 0, NULL);
    if (!gPipeThread) return 3;


    SetConsoleCtrlHandler(ConsoleCtrl, TRUE);

    WaitForSingleObject(gStopEvent, INFINITE);

    wprintf(L"[Console] Stopping...\n");
    WaitForSingleObject(gPipeThread, 5000);

    CloseHandle(gPipeThread);
    CloseHandle(gStopEvent);
    DeleteCriticalSection(&gGameLock);
    return 0;
}

int wmain(int argc, wchar_t** argv)
{
    if (argc >= 2 && lstrcmpiW(argv[1], L"--console") == 0) {
        return RunAsConsole();
    }

    SERVICE_TABLE_ENTRYW table[] = {
        { (LPWSTR)SVC_NAME, (LPSERVICE_MAIN_FUNCTIONW)SvcMain },
        { NULL, NULL }
    };

    if (!StartServiceCtrlDispatcherW(table)) {
        // If started from console without --console, show a hint.
        DWORD e = GetLastError();
        if (e == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT) {
            wprintf(L"Not started as a service. Run with --console for debug.\n");
        }
        else {
            wprintf(L"StartServiceCtrlDispatcher failed: %lu\n", e);
        }
        return (int)e;
    }

    return 0;
}
