#ifndef NONE_VERSION
#include <stdio.h>
#else
int printf(const char* ppp)
{
    return 0;
}

#endif

int main(int argc, char *argv, char *env[])
{
    printf("Hello Coder");
    while (1);
    return 0;
}

