#ifndef NONE_VERSION
#include <stdio.h>
#else

#ifdef WIN32

int printf(const char* ppp)
{
    ppp;
    return 0;
}
#else
static inline long sys_write(int fd, const void* buf, unsigned long len) {
    long ret;
    asm volatile(
        "syscall"
        : "=a"(ret)                          // ret in RAX
        : "a"(1),                            // RAX = __NR_write
        "D"(fd),                           // RDI = fd
        "S"(buf),                          // RSI = buf
        "d"(len)                           // RDX = len
        : "rcx", "r11", "memory"
        );
    return ret;
}

int my_strlen(const char* s) {
    int len = 0;
    while (s[len] != '\0') {
        len++;
    }
    return len;
}

int printf(const char* ppp)
{
    sys_write(1, ppp, my_strlen(ppp));
    return 0;
}

static inline void sys_exit(int code) {
    asm volatile(
        "syscall"
        : /* no outputs */
    : "a"(60),                           // RAX = __NR_exit
        "D"(code)                          // RDI = exit code
        : "rcx", "r11", "memory"
        );
    __builtin_unreachable();
}

#endif

#endif

int main(int argc, char *argv, char *env[])
{
    argc;
    argv;
    env;
    printf("\nHello Coder\n");
    
#ifdef NONE_VERSION
#ifdef WIN32
    while (1);
#else
    sys_exit(0);
#endif
#endif
    return 0;
}

