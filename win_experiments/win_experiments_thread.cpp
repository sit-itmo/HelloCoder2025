#include <stdio.h>
#include <windows.h>
#include <mutex>

CRITICAL_SECTION cs;
HANDLE M;
std::mutex mutex;

int TestCounter0(long long* p_counter, long long count)
{
    for (long long i = 0; i < count; i++)
    {
        *p_counter = *p_counter + 1;
    }
    return 0;
}

int TestCounterI(long long* p_counter, long long count)
{
    for (long long i = 0; i < count; i++)
    {
        InterlockedIncrement64(p_counter);
    }
    return 0;
}

int TestCounterCS(long long* p_counter, long long count)
{
    for (long long i = 0; i < count; i++)
    {
        EnterCriticalSection(&cs);
        *p_counter = *p_counter + 1;
        LeaveCriticalSection(&cs);
    }
    return 0;
}

int TestCounterMutex(long long* p_counter, long long count)
{
    for (long long i = 0; i < count; i++)
    {
        WaitForSingleObject(M, INFINITE);
        *p_counter = *p_counter + 1;
        ReleaseMutex(M);
    }
    return 0;
}

int TestCounter_std(long long* p_counter, long long count)
{
    for (long long i = 0; i < count; i++)
    {
        std::unique_lock<std::mutex> lck(mutex);
        *p_counter = *p_counter + 1;
    }
    return 0;
}


DWORD WINAPI WorkerThreadFuncCounter(LPVOID lpThreadParameter)
{
    printf("\n(%d)------Start thread", GetCurrentThreadId());
    TestCounter_std((long long*)lpThreadParameter, 10000000);
    printf("\n(%d)------End thread", GetCurrentThreadId());
    return 0;
}

bool SetupConsole()
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

        // Get the handle to the console output
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE)
        return false;

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode))
        return false;

    // Add ENABLE_VIRTUAL_TERMINAL_PROCESSING to the existing mode
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;

    SetConsoleMode(hOut, dwMode);
    return true;
}

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nShowCmd
)
{
    long long val = 0;

    SetupConsole();

    int thread_count = 10;
    HANDLE* p_hthreads = NULL;
    DWORD* p_threadids = NULL;

    M = CreateMutex(
        NULL,              // Атрибуты безопасности по умолчанию
        FALSE,             // Начальное состояние - не захвачен
        NULL               // Без имени (только для текущего процесса)
    );
    InitializeCriticalSection(&cs);

    p_threadids = (DWORD*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DWORD) * thread_count);
    p_hthreads = (HANDLE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(HANDLE) * thread_count);

    UINT64 start_ticks = GetTickCount64();
    for (int i = 0; i < thread_count; i++)
    {
        p_hthreads[i] = CreateThread(NULL, 0, WorkerThreadFuncCounter, &val,
            0, p_threadids + i);
    }
    WaitForMultipleObjects(thread_count, p_hthreads, TRUE, INFINITE);
    printf("\n================= ALL threads are done %lld (%lld)!", val, GetTickCount64() - start_ticks);

    CloseHandle(M);
    DeleteCriticalSection(&cs);
    HeapFree(GetProcessHeap(), 0, p_threadids);
    HeapFree(GetProcessHeap(), 0, p_hthreads);
    return 0;
}

