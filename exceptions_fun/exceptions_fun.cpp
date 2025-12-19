// build: cl /W4 /EHsc seh_div0_fix.c
// (Note: /EHsc is fine; SEH is enabled by MSVC for C.)
// Run: seh_div0_fix.exe

#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>

static volatile int g_divisor = 0;   // volatile so the compiler won't optimize away the fault
static volatile int g_dividend = 42;

static int SafeDivideWithFix(int a)
{
    int result = 0;
    int retry = 1;

    while (retry)
    {
        retry = 0;

        __try
        {
            // Faulting instruction
            printf("Division BEFORE: %d / %d = %d\n", a, g_divisor, result);
            result = g_dividend / g_divisor;
            printf("Division AFTER : %d / %d = %d\n", a, g_divisor, result);
        }
        __except (GetExceptionCode() == EXCEPTION_INT_DIVIDE_BY_ZERO
            ? EXCEPTION_EXECUTE_HANDLER
            : EXCEPTION_CONTINUE_SEARCH)
        {
            printf("[SEH] Divide by zero caught\n");
            ++g_divisor;   // fix state
            retry = 1;     // explicitly retry
        }
    }

    return result;
}

int Fun1_SEH_DIV(void)
{
    printf("Initial divisor = %d\n", g_divisor);

    int r = SafeDivideWithFix(g_dividend);

    printf("Final result = %d (dividend=%d, divisor=%d)\n", r, g_dividend, g_divisor);
    return 0;
}


static PVOID g_veh_handle = NULL;

void DumpExceptionPointers(const EXCEPTION_POINTERS* p);

bool g_veh_continue = true;
static LONG CALLBACK VectoredHandler(PEXCEPTION_POINTERS p)
{
    if (!p || !p->ExceptionRecord || !p->ContextRecord)
        return EXCEPTION_CONTINUE_SEARCH;

    DWORD code = p->ExceptionRecord->ExceptionCode;

    printf("\n================ VEH HIT ================\n");
    DumpExceptionPointers(p);

    if (code == EXCEPTION_INT_DIVIDE_BY_ZERO)
    {
        printf("[VEH] Caught EXCEPTION_INT_DIVIDE_BY_ZERO at ExceptionAddress=%p\n",
            p->ExceptionRecord->ExceptionAddress);

        // "Fix" divisor by incrementing it
        p->ContextRecord->Ecx++;

        if (g_veh_continue)
        {
            // Continue at the faulting instruction (it will be retried with new divisor)
            return EXCEPTION_CONTINUE_EXECUTION;
        }
        else
        {
            return EXCEPTION_CONTINUE_SEARCH;
        }
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

static int DoDivide(void)
{
    int result = 0;
    printf("Division BEFORE: %d / %d = %d\n", g_dividend, g_divisor, result);
    result = g_dividend / g_divisor;
    printf("Division AFTER : %d / %d = %d\n", g_dividend, g_divisor, result);

    return result;
}

int Fun2_VEH_DIV(void)
{
    g_veh_continue = true;
    printf("Initial: dividend=%d divisor=%d\n", g_dividend, g_divisor);

    g_veh_handle = AddVectoredExceptionHandler(/*FirstHandler=*/1, VectoredHandler);
    if (!g_veh_handle)
    {
        printf("AddVectoredExceptionHandler failed. GetLastError=%lu\n", GetLastError());
        return 1;
    }

    int result = DoDivide();

    printf("After:   dividend=%d divisor=%d result=%d\n", g_dividend, g_divisor, result);

    //RemoveVectoredExceptionHandler(g_veh_handle);
    //g_veh_handle = NULL;

    return 0;
}


#include <string>

struct Marker
{
    std::string Caption;
    Marker(const char* p_marker) : Caption(p_marker) { printf("\n    MARKER: [%s]\n", Caption.c_str()); }
    ~Marker() { printf("\n~~~~MARKER: [%s]\n", Caption.c_str()); }
};


struct MyMegaException
{
    std::string Caption;
    std::string File;
    int Line;
    int Code;

    MyMegaException(int code, const char* p_capt = "", const char* p_file = "", int line = 0) 
        : Caption(p_capt), File(p_file), Line(line), Code(code) {}
};

#define MEGA_EXT(a, c) MyMegaException((a), (c), __FILE__, __LINE__)
#define MEGA(a) MyMegaException((a), "", __FILE__, __LINE__)

static int DoDivideCpp_stage2(void)
{
    int result = 0;
    Marker m1("DoDivideCpp_stage2+1");

    printf("Division BEFORE: %d / %d = %d\n", g_dividend, g_divisor, result);
    
    if (g_divisor == 0)
    {
        Marker m2("DoDivideCpp_stage2+2");
        throw 7;
    }
    if (g_divisor == -1)
    {
        Marker m3("DoDivideCpp_stage2+2222");
        throw MEGA(7);
    }
    Marker m4("DoDivideCpp_stage2+3");

    result = g_dividend / g_divisor;
    printf("Division AFTER : %d / %d = %d\n", g_dividend, g_divisor, result);

    return result;
}


static int DoDivideCpp_stage1(void)
{
    int result = 0;
    Marker m("DoDivideCpp_stage1+1");
    printf("Division BEFORE #1 ------\n", g_dividend, g_divisor, result);
    DoDivideCpp_stage2();
    printf("Division AFTER  #1 ++++++\n", g_dividend, g_divisor, result);

    return result;
}

static int Fun3_CPP_DIV(void)
{
    int result = 0;
    Marker m("DoDivideCpp_stage0+1");
    printf("Division BEFORE #0 ------\n", g_dividend, g_divisor, result);
    try
    {
        Marker m2("DoDivideCpp_stage0+2");
        printf("Division BEFORE #0-1 ------\n", g_dividend, g_divisor, result);
        DoDivideCpp_stage1();
        printf("Division AFTER  #0-1 ++++++\n", g_dividend, g_divisor, result);
    }
    catch (int& e)
    {
        printf("ERROR: %d\n", e);
    }
    catch (MyMegaException& e)
    {
        printf("ERROR: %d\n", e);
    }
    printf("Division AFTER  #0 ++++++\n", g_dividend, g_divisor, result);

    return result;
}

int main(void)
{
    //Fun2_VEH_DIV();
    //g_divisor = 0;
    //g_veh_continue = false;
    //Fun1_SEH_DIV();
    g_divisor = -1;
    Fun3_CPP_DIV();

    return 0;
}

