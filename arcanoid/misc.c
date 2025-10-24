#include "arcanoid.h"
#include <stdio.h>
#include <stdarg.h>

unsigned long g_AG_LastPrintTime = 0;
unsigned long g_AG_LastFrame = 0;
unsigned long g_AG_TotalFrames = 0;

int g_AG_TraceEnabled = 0;

void AG_LogMessage(const char* p_file, int line, enum AG_LOG_KIND kind, const char* p_fmt, ...)
{
    static FILE* p_game_log = NULL;
    char buf[128] = { 0 };
    char* p_buf = &buf[0];
    char* p_buf_end = &buf[127];
    va_list args;
    va_start(args, p_fmt);

    *p_buf_end = 0;

    if (g_AG_TraceEnabled == 0 && kind == AG_LOG_KIND_TRACE)
    {
        return;
    }

    if (kind != AG_LOG_KIND_PRINT)
    {
        p_buf += snprintf(p_buf, p_buf_end - p_buf, "\n");
    }

    p_buf += snprintf(p_buf, p_buf_end - p_buf, "%8s:%4d: ", p_file, line);

    if (kind == AG_LOG_KIND_ERROR)
    {
        p_buf += snprintf(p_buf, p_buf_end - p_buf, "----------- ERROR!!!\n");
    }
    else if (kind == AG_LOG_KIND_WARNI)
    {
        p_buf += snprintf(p_buf, p_buf_end - p_buf, "WARNING");
    }

    p_buf += vsnprintf(p_buf, p_buf_end - p_buf, p_fmt, args);
    va_end(args);

    if (kind == AG_LOG_KIND_ERROR)
    {
        p_buf += snprintf(p_buf, p_buf_end - p_buf, "\n");
    }
    
    if (p_game_log == NULL)
    {
        p_game_log = fopen("game.log", "a+");
    }
    if (p_game_log)
    {
        fprintf(p_game_log, "%s", buf);
        fflush(stdout);
    }
    printf("%s", buf);
}
