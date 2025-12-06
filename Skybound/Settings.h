#pragma once

struct sLogSettings
{
    bool FlushAlways = false;
    bool TraceEnabled = false;
    bool PrintModule = true;
    bool PrintFile = true;
    bool PrintTime = true;
    bool WriteConsole = true;
    bool WriteFile = false;
    std::string LogFileName = SKYBOUND_DEFAULT_LOG_FILE;
};

struct sSettings
{
    sSize2D ScreenSize = {800, 600};
    bool FullScreen = false;
    sSize2D MainTileSize = {32, 32};
    int Level = 1;
    std::string WindowCaption;
    bool AllowLogs = true;
    sLogSettings LogSettings;

    bool Save(const char* p_path);
    bool Load(const char* p_path);
};