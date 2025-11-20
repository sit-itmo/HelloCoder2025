#include <stdio.h>
#include <windows.h>
#include "buf.h"

//Buf ddd;

#ifdef __cplusplus
void do_magic(const char* p_txt)
{
    printf("\n%s", p_txt);
    Sleep(100);
}

struct s32
{
    int v;

    s32() { do_magic("()"); }
    ~s32() { do_magic("~()"); }
    s32(int x) { do_magic("(int)"); v = x; }
    s32(const s32& x) { do_magic("(s32)"); v = x.v; }
    bool operator <(int x) { do_magic("<"); return v < x; }
    s32 operator -(int x) { do_magic("-"); return v - x; }
    s32 operator =(int x) { do_magic("=int"); return v = x; }
    s32 operator =(const s32& x) { do_magic("=s32"); return v = x.v; }
    s32 operator ++(int) { int _v = v; do_magic("++"); v++; return _v; }
};
s32 operator+(const s32& l, const s32& r) { do_magic("+"); return l.v + r.v; }

#else
typedef int s32;
#endif


s32 fib(s32 x)
{
    s32 y = 0;

    if (x < 1)
    {
        return 1;
    }
    y = fib(x - 1) + fib(x - 2);
    return y;
}

int main_v0(int argc, char* argv[])
{
    s32 c = 0;

    for (c = 0; c < 10; c++)
    {
        printf("\nFib %d = %d!", c, fib(c));
    }
    return 0;
}
