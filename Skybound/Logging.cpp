#include "Skybound.h"
#include <stdarg.h>

void Logging::LogMessageEx(const char* p_file, int line, const char* p_module, enum Kind kind, const char* p_fmt, ...)
{
    char buf[Logging::MaxMessageSize];
    va_list args;
    va_start(args, p_fmt);
    vsnprintf(buf, sizeof(buf), p_fmt, args);
    va_end(args);
    LogMessage(p_file, line, p_module, kind, buf);
}

const char* GetJustFileName(const char* p_text)
{
    const char* p_res = strrchr(p_text, '\\');
    return p_res == NULL ? p_text : p_res + 1;
}

void Logging::LogMessage(const char* p_file, int line, const char* p_module, enum Kind kind, const char* p_text)
{
    char buf[Logging::MaxMessageSize];

    if (_TraceEnabled == false && kind == Kind_TRACE)
    {
        return;
    }

    if (kind != Kind_PRINT)
    {
        PutText("\n");
        if (_PrintFile)
        {
            snprintf(buf, sizeof(buf), "%12s:%4d: ", GetJustFileName(p_file), line);
            PutText(buf);
        }
        if (_PrintTime)
        {
            snprintf(buf, sizeof(buf), "%8lld ", __rdtsc());
            PutText(buf);
        }
        if (_PrintModule)
        {
            snprintf(buf, sizeof(buf), "[%6s] ", p_module == nullptr ? " " : p_module);
            PutText(buf);
        }
    }

    if (kind == Kind_ERROR)
    {
        PutText(ANSI::makeStyle(ANSI::Color::Red).c_str());
        PutText("![ERROR]!");
        PutText(ANSI::reset().c_str());
    }
    else if (kind == Kind_WARNI)
    {
        PutText(ANSI::makeStyle(ANSI::Color::Yellow).c_str());
        PutText("<WARNING>");
        PutText(ANSI::reset().c_str());
    }
    else if (kind == Kind_DEBUG)
    {
        PutText(ANSI::makeStyle(ANSI::Color::Magenta).c_str());
        PutText("--DEBUG--");
        PutText(ANSI::reset().c_str());
    }
    else if (kind == Kind_TRACE)
    {
        PutText(ANSI::makeStyle(ANSI::Color::Cyan).c_str());
        PutText("--TRACE--");
        PutText(ANSI::reset().c_str());
    }
    else if (kind == Kind_PLINE)
    {
        PutText(ANSI::reset().c_str());
        PutText("         ");
    }

    PutText(p_text);
}

void Logging::PutText(const char* p_text)
{
    if (_Ready == false)
    {
        Setup();
    }
    if (_WriteConsole)
    {
        printf("%s", p_text);
        if (_FlushAlways)
        {
            fflush(stdout);
        }
    }
    if (_WriteFile)
    {
        if (pLogFile == nullptr)
        {
            pLogFile = fopen(SKYBOUND_DEFAULT_LOG_FILE, "at");
        }
        if (pLogFile != nullptr)
        {
            fprintf(pLogFile, "%s", p_text);
            if (_FlushAlways)
            {
                fflush(pLogFile);
            }
        }
    }
}

bool Logging::Setup()
{
    if (_WriteConsole)
    {
        Skybound::getSingleton()->GetPlatform()->SetupConsole();
    }
    _Ready = true;
    return true;
}





