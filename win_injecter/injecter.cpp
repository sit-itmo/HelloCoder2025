#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <shlwapi.h>   // PathRemoveFileSpec
#pragma comment(lib, "Shlwapi.lib")

static void PrintLastErrorA(const char* where)
{
    DWORD e = GetLastError();
    CHAR msg[512] = { 0 };
    FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, e, 0, msg, sizeof(msg), NULL);
    printf("%s failed: (%lu) %s\n", where, e, msg);
}

// ------------------------------------------------------------
// Enumerate all processes and print them
// ------------------------------------------------------------
void EnumerateProcesses()
{
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
    {
        PrintLastErrorA("CreateToolhelp32Snapshot");
        return;
    }

    PROCESSENTRY32 pe{};
    pe.dwSize = sizeof(pe);

    if (Process32First(snap, &pe))
    {
        do
        {
            printf("PID=%5lu  EXE=%ws\n", pe.th32ProcessID, pe.szExeFile);
        } while (Process32Next(snap, &pe));
    }
    else
    {
        PrintLastErrorA("Process32First");
    }

    CloseHandle(snap);
}

// ------------------------------------------------------------
// Open process by PID
// ------------------------------------------------------------
HANDLE OpenProcessByPid(
    DWORD pid,
    DWORD access = PROCESS_QUERY_INFORMATION | PROCESS_VM_READ)
{
    HANDLE h = OpenProcess(access, FALSE, pid);
    if (!h)
        PrintLastErrorA("OpenProcess(pid)");
    return h;
}

// ------------------------------------------------------------
// Open process by executable name (case-insensitive)
// Returns first match
// ------------------------------------------------------------
HANDLE OpenProcessByName(
    const wchar_t* exeName,
    DWORD access = PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
    DWORD* outPid = nullptr)
{
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
    {
        PrintLastErrorA("CreateToolhelp32Snapshot");
        return NULL;
    }

    PROCESSENTRY32 pe{};
    pe.dwSize = sizeof(pe);

    HANDLE hProcess = NULL;

    if (Process32First(snap, &pe))
    {
        do
        {
            if (_wcsicmp(pe.szExeFile, exeName) == 0)
            {
                hProcess = OpenProcess(access, FALSE, pe.th32ProcessID);
                if (!hProcess)
                {
                    PrintLastErrorA("OpenProcess(name)");
                }
                else if (outPid)
                {
                    *outPid = pe.th32ProcessID;
                }
                break;
            }
        } while (Process32Next(snap, &pe));
    }
    else
    {
        PrintLastErrorA("Process32First");
    }

    CloseHandle(snap);
    return hProcess;
}


static bool BuildFullPathInCwdW(const wchar_t* fileName, wchar_t* outPath, DWORD outCap)
{
    wchar_t cwd[MAX_PATH];

    DWORD n = GetCurrentDirectoryW((DWORD)_countof(cwd), cwd);
    if (n == 0 || n >= _countof(cwd))
        return false;

    // Join: <cwd>\<fileName>
    // Ensure there is exactly one backslash
    wchar_t tmp[MAX_PATH];
    if (cwd[n - 1] == L'\\' || cwd[n - 1] == L'/')
        wsprintfW(tmp, L"%s%s", cwd, fileName);
    else
        wsprintfW(tmp, L"%s\\%s", cwd, fileName);

    // Normalize to absolute path (resolves .., . etc.)
    DWORD r = GetFullPathNameW(tmp, outCap, outPath, NULL);
    return (r != 0 && r < outCap);
}

BOOL WINAPI InjectLibW(DWORD dwProcessId, PCWSTR pszLibFile) {

    BOOL bOk = FALSE; // Assume that the function fails
    HANDLE hProcess = NULL, hThread = NULL;
    PWSTR pszLibFileRemote = NULL;

    __try {
        // Get a handle for the target process.
        hProcess = OpenProcess(
            PROCESS_QUERY_INFORMATION |   // Required by Alpha
            PROCESS_CREATE_THREAD |   // For CreateRemoteThread
            PROCESS_VM_OPERATION |   // For VirtualAllocEx/VirtualFreeEx
            PROCESS_VM_WRITE,             // For WriteProcessMemory
            FALSE, dwProcessId);
        if (hProcess == NULL) __leave;

        // Calculate the number of bytes needed for the DLL's pathname
        int cch = 1 + lstrlenW(pszLibFile);
        int cb = cch * sizeof(wchar_t);

        // Allocate space in the remote process for the pathname
        pszLibFileRemote = (PWSTR)
            VirtualAllocEx(hProcess, NULL, cb, MEM_COMMIT, PAGE_READWRITE);
        if (pszLibFileRemote == NULL) __leave;

        // Copy the DLL's pathname to the remote process' address space
        if (!WriteProcessMemory(hProcess, pszLibFileRemote,
            (PVOID)pszLibFile, cb, NULL)) __leave;

        // Get the real address of LoadLibraryW in Kernel32.dll
        PTHREAD_START_ROUTINE pfnThreadRtn = (PTHREAD_START_ROUTINE)
            GetProcAddress(GetModuleHandle(TEXT("Kernel32")), "LoadLibraryW");
        if (pfnThreadRtn == NULL) __leave;

        // Create a remote thread that calls LoadLibraryW(DLLPathname)
        hThread = CreateRemoteThread(hProcess, NULL, 0,
            pfnThreadRtn, pszLibFileRemote, 0, NULL);
        if (hThread == NULL) __leave;

        // Wait for the remote thread to terminate
        WaitForSingleObject(hThread, INFINITE);

        bOk = TRUE; // Everything executed successfully
    }
    __finally { // Now, we can clean everything up

        // Free the remote memory that contained the DLL's pathname
        if (pszLibFileRemote != NULL)
            VirtualFreeEx(hProcess, pszLibFileRemote, 0, MEM_RELEASE);

        if (hThread != NULL)
            CloseHandle(hThread);

        if (hProcess != NULL)
            CloseHandle(hProcess);
    }

    return(bOk);
}

typedef DWORD(WINAPI* TpFormatMessageA)(
    DWORD dwFlags,
    LPCVOID lpSource,
    DWORD dwMessageId,
    DWORD dwLanguageId,
    LPSTR lpBuffer,
    DWORD nSize,
    va_list* Arguments
    );
typedef HANDLE(WINAPI* TpGetCurrentProcess)(VOID);
typedef HMODULE(WINAPI* TpGetModuleHandleA)(LPCSTR lpModuleName);
typedef DWORD(WINAPI* TpGetModuleFileNameA)(
    HMODULE hModule, LPSTR lpFilename, DWORD nSize);
typedef DWORD(WINAPI* TpGetProcessId)(HANDLE Process);
typedef int (WINAPI* TpMessageBoxA)(
    _In_opt_ HWND hWnd,
    _In_opt_ LPCSTR lpText,
    _In_opt_ LPCSTR lpCaption,
    _In_ UINT uType);

struct ShellCodeParams
{
    TpFormatMessageA pFormatMessageA;
    TpGetCurrentProcess pGetCurrentProcess;
    TpGetModuleHandleA pGetModuleHandleA;
    TpGetModuleFileNameA pGetModuleFileNameA;
    TpGetProcessId pGetProcessId;
    TpMessageBoxA pMessageBoxA;
};


DWORD WINAPI ShellCode(LPVOID lpThreadParameter)
{
    //while (1);
    //__debugbreak();
    ShellCodeParams* p_params = (ShellCodeParams*)lpThreadParameter;
    CHAR buf[128];
    CHAR mod_name[128];
    HANDLE h_proc = p_params->pGetCurrentProcess();
    HMODULE h_mod = p_params->pGetModuleHandleA(NULL);
    p_params->pGetModuleFileNameA(h_mod, mod_name, sizeof(mod_name));
    LPCSTR args[] = {
        (LPCSTR)(ULONG_PTR)p_params->pGetProcessId(h_proc),
        mod_name
    };
    CHAR example[] = { 'E', 'x', 'a', 'm', 'p', 'l', 'e', 0 };
    
    CHAR message[] = {
        'H', 'A', 'C', 'K', ' ', 'F', 'i', 'r', 'e', 'd', ' ', 'f', 'r', 'o', 'm', ' ', 'p', 'r', 'o', 'c', 'e', 's', 's', ' ', 'P', 'r', 'o', 'c', 'e', 's', 's', 'I', 'D', ' ', '=', ' ', '%', '1', '!', 'u', '!', '\n',
        'P', 'r', 'o', 'c', 'e', 's', 's', 'N', 'a', 'm', 'e', ':', ' ', '%', '2', '!', 's', '!', 0 };
    

    p_params->pFormatMessageA(
        FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
        message,
        0, // dwFlags
        0, // dwLanguageId
        buf, // lpBuffer
        sizeof(buf) / sizeof(buf[0]), // nSize
        (va_list*)args
    );

    p_params->pMessageBoxA(NULL, buf, example, MB_OK);
    return 0;
}



BOOL WINAPI InjectShellCodeW(DWORD dwProcessId) {

    BOOL bOk = FALSE; // Assume that the function fails
    HANDLE hProcess = NULL, hThread = NULL;
    CHAR *p_remoteBuf = NULL;

    __try {
        // Get a handle for the target process.
        hProcess = OpenProcess(
            PROCESS_QUERY_INFORMATION |   // Required by Alpha
            PROCESS_CREATE_THREAD |   // For CreateRemoteThread
            PROCESS_VM_OPERATION |   // For VirtualAllocEx/VirtualFreeEx
            PROCESS_VM_WRITE,             // For WriteProcessMemory
            FALSE, dwProcessId);
        if (hProcess == NULL) __leave;

        int cb = 2048;

        // Allocate space in the remote process for the pathname
        p_remoteBuf = (CHAR*)
            VirtualAllocEx(hProcess, NULL, cb, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
        if (p_remoteBuf == NULL) __leave;

        ShellCodeParams params;
#define TTT_GETPROC(l, a) \
    params.p ## a = (Tp ## a)GetProcAddress(GetModuleHandleA(#l), #a);

        TTT_GETPROC(Kernel32, FormatMessageA);
        TTT_GETPROC(Kernel32, GetCurrentProcess);
        TTT_GETPROC(Kernel32, GetModuleHandleA);
        TTT_GETPROC(Kernel32, GetModuleFileNameA);
        TTT_GETPROC(Kernel32, GetProcessId);
        TTT_GETPROC(User32, MessageBoxA);

        if (!WriteProcessMemory(hProcess, p_remoteBuf,
            (PVOID)&params, sizeof(params), NULL)) __leave;

        CHAR* p_code_pounter = (char *)&ShellCode;
        if (p_code_pounter[0] == (char)0xE9)
        {
            p_code_pounter = (CHAR *)(((LONG)p_code_pounter) 
                + 5 + (*(INT*)(p_code_pounter + 1)));
        }

        if (!WriteProcessMemory(hProcess, p_remoteBuf + sizeof(params),
            (PVOID)p_code_pounter, 700, NULL)) __leave;

        //CHAR buff[] = { 0xEB, 0xFE };
        //if (!WriteProcessMemory(hProcess, p_remoteBuf,
        //    (PVOID)&buff, sizeof(buff), NULL)) __leave;


        //p_remoteBuf[0] = 0xEB;
        //p_remoteBuf[1] = 0xFE;

        // Create a remote thread that calls LoadLibraryW(DLLPathname)
        hThread = CreateRemoteThread(hProcess, NULL, 0,
            (LPTHREAD_START_ROUTINE)(p_remoteBuf + sizeof(params)),
            //(LPTHREAD_START_ROUTINE)p_remoteBuf,
            p_remoteBuf, 0, NULL);
        if (hThread == NULL) __leave;

        // Wait for the remote thread to terminate
        WaitForSingleObject(hThread, INFINITE);

        bOk = TRUE; // Everything executed successfully
    }
    __finally { // Now, we can clean everything up

        // Free the remote memory that contained the DLL's pathname
        if (p_remoteBuf != NULL)
            VirtualFreeEx(hProcess, p_remoteBuf, 0, MEM_RELEASE);

        if (hThread != NULL)
            CloseHandle(hThread);

        if (hProcess != NULL)
            CloseHandle(hProcess);
    }

    return(bOk);
}


int main()
{
    //CHAR* p_code_pounter = (char*)ShellCode;
    //if (p_code_pounter[0] == (char)0xE9)
    //{
    //    p_code_pounter = (CHAR*)(((LONG)p_code_pounter)
    //        + 5 + (*(INT*)(p_code_pounter + 1)));
    //}
    // 009A11D1 + 5 + 00000CFA = 9A1ED0
    //009A11D1 E9 FA 0C 00 00       jmp         ShellCode (09A1ED0h)
#if 0
    ShellCodeParams params;

#define TTT_GETPROC(l, a) \
    params.p ## a = (Tp ## a)GetProcAddress(GetModuleHandleA(#l), #a);

    TTT_GETPROC(Kernel32, FormatMessageA);
    TTT_GETPROC(Kernel32, GetCurrentProcess);
    TTT_GETPROC(Kernel32, GetModuleHandleA);
    TTT_GETPROC(Kernel32, GetModuleFileNameA);
    TTT_GETPROC(Kernel32, GetProcessId);
    TTT_GETPROC(User32, MessageBoxA);

    ShellCode(&params);
#endif

    const wchar_t* p_dll_name = L"testdll.dll";
    //wchar_t exePath[MAX_PATH];
    //wchar_t dllPath[MAX_PATH];
    //
    //DWORD len = GetModuleFileNameW(NULL, exePath, MAX_PATH);
    //if (len == 0 || len == MAX_PATH)
    //    return NULL;
    //
    //PathRemoveFileSpecW(exePath);
    //wsprintfW(dllPath, L"%s\\%s", exePath, p_dll_name);

    wchar_t dll_full_path[MAX_PATH];
    printf("Test DLL\n");
    BuildFullPathInCwdW(p_dll_name, dll_full_path, (DWORD)_countof(dll_full_path));
    //HANDLE hmod = LoadLibraryW(dll_full_path);

    DWORD target_pid = 0;
    HANDLE h_proc = OpenProcessByName(L"winhex.exe", PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, &target_pid);
    if (h_proc)
    {
        CloseHandle(h_proc);
        InjectShellCodeW(target_pid);
        //InjectLibW(target_pid, dll_full_path);
    }
    return 0;
}

int demo()
{
    printf("=== All processes ===\n");
    EnumerateProcesses();

    printf("\n=== Open by PID ===\n");
    DWORD pid = GetCurrentProcessId();
    HANDLE h1 = OpenProcessByPid(pid);
    if (h1)
    {
        printf("Opened current process by PID %lu, handle=%p\n", pid, h1);
        CloseHandle(h1);
    }

    printf("\n=== Open by name ===\n");
    DWORD foundPid = 0;
    HANDLE h2 = OpenProcessByName(L"explorer.exe",
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        &foundPid);

    if (h2)
    {
        printf("Opened explorer.exe, PID=%lu, handle=%p\n", foundPid, h2);
        CloseHandle(h2);
    }
    else
    {
        printf("explorer.exe not found or access denied\n");
    }

    return 0;
}

