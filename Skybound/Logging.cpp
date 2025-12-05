#include "Skybound.h"
#include <stdarg.h>
#include <sstream>
#include <string>
#include <iostream>

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

void Logging::LogMessagePrefix(const char* p_file, int line, const char* p_module, enum Kind kind)
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
            snprintf(buf, sizeof(buf), "%f ", Skybound::getSingleton()->GetPlatform()->GetTime());
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
    }
    else if (kind == Kind_WARNI)
    {
        PutText(ANSI::makeStyle(ANSI::Color::Yellow).c_str());
        PutText("<WARNING>");
    }
    else if (kind == Kind_DEBUG)
    {
        PutText(ANSI::makeStyle(ANSI::Color::Magenta).c_str());
        PutText("--DEBUG--");
    }
    else if (kind == Kind_TRACE)
    {
        PutText(ANSI::makeStyle(ANSI::Color::Cyan).c_str());
        PutText("--TRACE--");
    }
    else if (kind == Kind_PLINE)
    {
        PutText(ANSI::reset().c_str());
        PutText("         ");
    }
    PutText(ANSI::reset().c_str());
    PutText(" ");

}

void Logging::LogMessage(const char* p_file, int line, const char* p_module, enum Kind kind, const char* p_text)
{
    std::stringstream ss(p_text);
    std::string _line;

    while (std::getline(ss, _line))
    {
        LogMessagePrefix(p_file, line, p_module, kind);
        PutText(_line.c_str());
    }
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

void Logging::Demo()
{
    SKY_PRINTLN("Testing log system...");
    SKY_PRINT("[");
    for (int i = 0; i < 10; i++)
    {
        SKY_PRINT_EX("%d, ", i);
    }
    SKY_PRINT("]");

    SKY_DEBUG("This is a debug message");
    SKY_DEBUG_EX("Debug with formatting %d, 0x%x!", 10, 10);
    SKY_ERROR("This is ERROR message");
    SKY_ERROR_EX("This is ERROR message with formatting (%d)", ENOMEM);
    SKY_TRACE("Trace message");
    SKY_TRACE_EX("Trace message again format 0x%llx", __rdtsc());
    SKY_WARNING("Very important warning!");
    SKY_WARNING("Warining with %d %s formatting keep as is!");
    SKY_WARNING_EX("Very important warning %s!", "yesyes");

    for (int i = 0; i < 10; i++)
    {
        SKY_ASSERT(i < 9);
    }
}



