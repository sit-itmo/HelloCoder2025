// GameSvcClient.c - Client that talks to the service via Named Pipe (no install).
//
// Build:
//   cl /W4 /DUNICODE /D_UNICODE GameSvcClient.c
//
// Usage examples:
//   GameSvcClient.exe start
//   GameSvcClient.exe stop
//   GameSvcClient.exe echo "hello"
//   GameSvcClient.exe putkey 65
//   GameSvcClient.exe getscreen

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <wchar.h>

#define PIPE_NAME L"\\\\.\\pipe\\GameSvc"
#define PIPE_BUFSZ (64 * 1024)

typedef enum _GAME_CMD {
    CMD_ECHO = 1,
    CMD_GAME_START = 2,
    CMD_GAME_STOP = 3,
    CMD_GAME_PUT_KEY = 4,
    CMD_GAME_GET_SCREEN = 5
} GAME_CMD;

#pragma pack(push, 1)
typedef struct _REQ_HDR {
    DWORD Magic;
    DWORD Cmd;
    DWORD PayloadLen;
} REQ_HDR;

typedef struct _RSP_HDR {
    DWORD Magic;
    DWORD Status;
    DWORD PayloadLen;
} RSP_HDR;
#pragma pack(pop)

static DWORD MAGIC_GSV1 = 0x31565347; // 'GSV1'

static void PrintLastErrorW(const wchar_t* what)
{
    DWORD e = GetLastError();
    wchar_t* msg = NULL;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, e, 0, (LPWSTR)&msg, 0, NULL);
    wprintf(L"%s failed: %lu%s%s\n", what, e, msg ? L" - " : L"", msg ? msg : L"");
    if (msg) LocalFree(msg);
}

static DWORD CallService(DWORD cmd, const void* payload, DWORD payloadLen, BYTE* out, DWORD outCap, DWORD* outLen)
{
    if (outLen) *outLen = 0;

    // Wait briefly for pipe
    if (!WaitNamedPipeW(PIPE_NAME, 2000)) {
        PrintLastErrorW(L"WaitNamedPipe");
        return GetLastError();
    }

    HANDLE h = CreateFileW(PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        PrintLastErrorW(L"CreateFile(pipe)");
        return GetLastError();
    }

    BYTE buf[PIPE_BUFSZ];
    if (payloadLen > PIPE_BUFSZ - sizeof(REQ_HDR)) {
        CloseHandle(h);
        return ERROR_INVALID_PARAMETER;
    }

    REQ_HDR* req = (REQ_HDR*)buf;
    req->Magic = MAGIC_GSV1;
    req->Cmd = cmd;
    req->PayloadLen = payloadLen;
    if (payloadLen && payload) memcpy(buf + sizeof(REQ_HDR), payload, payloadLen);

    DWORD toWrite = sizeof(REQ_HDR) + payloadLen;
    DWORD wrote = 0;
    if (!WriteFile(h, buf, toWrite, &wrote, NULL) || wrote != toWrite) {
        PrintLastErrorW(L"WriteFile(pipe)");
        CloseHandle(h);
        return GetLastError();
    }

    DWORD got = 0;
    if (!ReadFile(h, buf, PIPE_BUFSZ, &got, NULL) || got < sizeof(RSP_HDR)) {
        PrintLastErrorW(L"ReadFile(pipe)");
        CloseHandle(h);
        return GetLastError();
    }

    RSP_HDR* rsp = (RSP_HDR*)buf;
    if (rsp->Magic != MAGIC_GSV1) {
        CloseHandle(h);
        return ERROR_INVALID_DATA;
    }

    DWORD status = rsp->Status;
    DWORD plen = rsp->PayloadLen;
    if (sizeof(RSP_HDR) + plen > got) {
        CloseHandle(h);
        return ERROR_INVALID_DATA;
    }

    if (plen && out && outCap) {
        DWORD n = (plen > outCap) ? outCap : plen;
        memcpy(out, buf + sizeof(RSP_HDR), n);
        if (outLen) *outLen = n;
    }
    else {
        if (outLen) *outLen = 0;
    }

    CloseHandle(h);
    return status;
}

int wmain(int argc, wchar_t** argv)
{
    if (argc < 2) {
        wprintf(L"Usage:\n");
        wprintf(L"  %s start\n", argv[0]);
        wprintf(L"  %s stop\n", argv[0]);
        wprintf(L"  %s echo \"text\"\n", argv[0]);
        wprintf(L"  %s putkey <number>\n", argv[0]);
        wprintf(L"  %s getscreen\n", argv[0]);
        return 1;
    }

    BYTE out[PIPE_BUFSZ];
    DWORD outLen = 0;
    DWORD st = 0;

    if (_wcsicmp(argv[1], L"start") == 0) {
        st = CallService(CMD_GAME_START, NULL, 0, NULL, 0, NULL);
        wprintf(L"GameStart: %lu\n", st);
        return (int)st;
    }
    if (_wcsicmp(argv[1], L"stop") == 0) {
        st = CallService(CMD_GAME_STOP, NULL, 0, NULL, 0, NULL);
        wprintf(L"GameStop: %lu\n", st);
        return (int)st;
    }
    if (_wcsicmp(argv[1], L"echo") == 0) {
        if (argc < 3) return 2;
        // send UTF-16 bytes as payload? We'll send ANSI for simplicity:
        char a[1024];
        int n = WideCharToMultiByte(CP_UTF8, 0, argv[2], -1, a, (int)sizeof(a), NULL, NULL);
        if (n <= 0) return 3;
        st = CallService(CMD_ECHO, a, (DWORD)(n - 1), out, sizeof(out), &outLen);
        wprintf(L"ECHO status=%lu, bytes=%lu\n", st, outLen);
        if (st == 0 && outLen) {
            // print as UTF-8 to console best-effort
            fwrite(out, 1, outLen, stdout);
            fputc('\n', stdout);
        }
        return (int)st;
    }
    if (_wcsicmp(argv[1], L"putkey") == 0) {
        if (argc < 3) return 2;
        DWORD key = (DWORD)wcstoul(argv[2], NULL, 0);
        st = CallService(CMD_GAME_PUT_KEY, &key, sizeof(key), NULL, 0, NULL);
        wprintf(L"GamePutKey(%lu): %lu\n", key, st);
        return (int)st;
    }
    if (_wcsicmp(argv[1], L"getscreen") == 0) {
        st = CallService(CMD_GAME_GET_SCREEN, NULL, 0, out, sizeof(out), &outLen);
        wprintf(L"GameGetScreen status=%lu, bytes=%lu\n", st, outLen);
        if (st == 0 && outLen) {
            fwrite(out, 1, outLen, stdout);
            fputc('\n', stdout);
        }
        return (int)st;
    }

    wprintf(L"Unknown command.\n");
    return 2;
}
