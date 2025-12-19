#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <inttypes.h>

static const char* ExceptionCodeName(DWORD code)
{
    switch (code)
    {
    case EXCEPTION_ACCESS_VIOLATION:         return "EXCEPTION_ACCESS_VIOLATION";
    case EXCEPTION_DATATYPE_MISALIGNMENT:    return "EXCEPTION_DATATYPE_MISALIGNMENT";
    case EXCEPTION_BREAKPOINT:              return "EXCEPTION_BREAKPOINT";
    case EXCEPTION_SINGLE_STEP:             return "EXCEPTION_SINGLE_STEP";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:    return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
    case EXCEPTION_FLT_DENORMAL_OPERAND:     return "EXCEPTION_FLT_DENORMAL_OPERAND";
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:       return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
    case EXCEPTION_FLT_INEXACT_RESULT:       return "EXCEPTION_FLT_INEXACT_RESULT";
    case EXCEPTION_FLT_INVALID_OPERATION:    return "EXCEPTION_FLT_INVALID_OPERATION";
    case EXCEPTION_FLT_OVERFLOW:             return "EXCEPTION_FLT_OVERFLOW";
    case EXCEPTION_FLT_STACK_CHECK:          return "EXCEPTION_FLT_STACK_CHECK";
    case EXCEPTION_FLT_UNDERFLOW:            return "EXCEPTION_FLT_UNDERFLOW";
    case EXCEPTION_INT_DIVIDE_BY_ZERO:       return "EXCEPTION_INT_DIVIDE_BY_ZERO";
    case EXCEPTION_INT_OVERFLOW:             return "EXCEPTION_INT_OVERFLOW";
    case EXCEPTION_PRIV_INSTRUCTION:         return "EXCEPTION_PRIV_INSTRUCTION";
    case EXCEPTION_IN_PAGE_ERROR:            return "EXCEPTION_IN_PAGE_ERROR";
    case EXCEPTION_ILLEGAL_INSTRUCTION:      return "EXCEPTION_ILLEGAL_INSTRUCTION";
    case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "EXCEPTION_NONCONTINUABLE_EXCEPTION";
    case EXCEPTION_STACK_OVERFLOW:           return "EXCEPTION_STACK_OVERFLOW";
    case EXCEPTION_INVALID_DISPOSITION:      return "EXCEPTION_INVALID_DISPOSITION";
    case EXCEPTION_GUARD_PAGE:               return "EXCEPTION_GUARD_PAGE";
    case EXCEPTION_INVALID_HANDLE:           return "EXCEPTION_INVALID_HANDLE";
    default:                                 return "UNKNOWN_EXCEPTION";
    }
}

static void DumpExceptionRecord(const EXCEPTION_RECORD* er)
{
    if (!er) return;

    printf("=== EXCEPTION_RECORD ===\n");
    printf("  Code        : 0x%08lX (%s)\n", er->ExceptionCode, ExceptionCodeName(er->ExceptionCode));
    printf("  Flags       : 0x%08lX\n", er->ExceptionFlags);
    printf("  Address     : %p\n", er->ExceptionAddress);
    printf("  Parameters  : %lu\n", er->NumberParameters);

    for (DWORD i = 0; i < er->NumberParameters && i < EXCEPTION_MAXIMUM_PARAMETERS; ++i)
    {
#ifdef _M_X64
        printf("    [%2lu]      : 0x%016" PRIx64 "\n", (unsigned long)i, (uint64_t)er->ExceptionInformation[i]);
#else
        printf("    [%2lu]      : 0x%08" PRIxPTR "\n", (unsigned long)i, (uintptr_t)er->ExceptionInformation[i]);
#endif
    }

    if (er->ExceptionRecord)
    {
        printf("  NestedRecord: %p (not dumped recursively)\n", (void*)er->ExceptionRecord);
    }
    printf("\n");
}

static void DumpContext(const CONTEXT* c)
{
    if (!c) return;

    printf("=== CONTEXT ===\n");

#ifdef _M_X64
    printf("  RIP=0x%016llX  RSP=0x%016llX  RBP=0x%016llX\n",
        (unsigned long long)c->Rip,
        (unsigned long long)c->Rsp,
        (unsigned long long)c->Rbp);

    printf("  RAX=0x%016llX  RBX=0x%016llX  RCX=0x%016llX  RDX=0x%016llX\n",
        (unsigned long long)c->Rax, (unsigned long long)c->Rbx,
        (unsigned long long)c->Rcx, (unsigned long long)c->Rdx);

    printf("  RSI=0x%016llX  RDI=0x%016llX  R8 =0x%016llX  R9 =0x%016llX\n",
        (unsigned long long)c->Rsi, (unsigned long long)c->Rdi,
        (unsigned long long)c->R8, (unsigned long long)c->R9);

    printf("  R10=0x%016llX  R11=0x%016llX  R12=0x%016llX  R13=0x%016llX\n",
        (unsigned long long)c->R10, (unsigned long long)c->R11,
        (unsigned long long)c->R12, (unsigned long long)c->R13);

    printf("  R14=0x%016llX  R15=0x%016llX\n",
        (unsigned long long)c->R14, (unsigned long long)c->R15);

    printf("  EFlags=0x%08lX\n", (unsigned long)c->EFlags);

    printf("  CS=0x%04X  SS=0x%04X  DS=0x%04X  ES=0x%04X  FS=0x%04X  GS=0x%04X\n",
        (unsigned)c->SegCs, (unsigned)c->SegSs, (unsigned)c->SegDs,
        (unsigned)c->SegEs, (unsigned)c->SegFs, (unsigned)c->SegGs);

#else // _M_IX86
    printf("  EIP=0x%08lX  ESP=0x%08lX  EBP=0x%08lX\n",
        (unsigned long)c->Eip, (unsigned long)c->Esp, (unsigned long)c->Ebp);

    printf("  EAX=0x%08lX  EBX=0x%08lX  ECX=0x%08lX  EDX=0x%08lX\n",
        (unsigned long)c->Eax, (unsigned long)c->Ebx,
        (unsigned long)c->Ecx, (unsigned long)c->Edx);

    printf("  ESI=0x%08lX  EDI=0x%08lX\n",
        (unsigned long)c->Esi, (unsigned long)c->Edi);

    printf("  EFlags=0x%08lX\n", (unsigned long)c->EFlags);

    printf("  CS=0x%04X  SS=0x%04X  DS=0x%04X  ES=0x%04X  FS=0x%04X  GS=0x%04X\n",
        (unsigned)c->SegCs, (unsigned)c->SegSs, (unsigned)c->SegDs,
        (unsigned)c->SegEs, (unsigned)c->SegFs, (unsigned)c->SegGs);
#endif

    printf("\n");
}

void DumpExceptionPointers(const EXCEPTION_POINTERS* p)
{
    if (!p) return;
    DumpExceptionRecord(p->ExceptionRecord);
    DumpContext(p->ContextRecord);
}
