#ifndef NONE_VERSION
#include <stdio.h>
#else
int printf(const char* ppp)
{
    ppp;
    return 0;
}

#endif

int main(int argc, char *argv, char *env[])
{
    argc;
    argv;
    env;
    printf("\nHello Coder\n");
    //while (1);
    return 0;
}

