#pragma once

#include <string>

namespace ANSI
{
    // Basic color enum (foreground)
    enum class Color
    {
        Default = 39,
        Black = 30,
        Red = 31,
        Green = 32,
        Yellow = 33,
        Blue = 34,
        Magenta = 35,
        Cyan = 36,
        White = 37,

        // Bright variants
        BrightBlack = 90,
        BrightRed = 91,
        BrightGreen = 92,
        BrightYellow = 93,
        BrightBlue = 94,
        BrightMagenta = 95,
        BrightCyan = 96,
        BrightWhite = 97
    };

    // Background colors (just offset by +10 from foreground)
    inline int bgCode(Color c)
    {
        return static_cast<int>(c) + 10 - 30; // 30..37 -> 40..47; 90..97 -> 100..107
    }

    // Build ANSI escape sequence for given fg/bg and attributes
    inline std::string makeStyle(Color fg = Color::Default,
        Color bg = Color::Default,
        bool bold = false,
        bool underline = false)
    {
        std::string seq = "\033["; // ESC[

        bool first = true;

        auto add = [&](int code)
            {
                if (!first) seq += ';';
                seq += std::to_string(code);
                first = false;
            };

        // Reset first, then add options
        add(0);

        if (bold)      add(1);
        if (underline) add(4);

        if (fg != Color::Default)
            add(static_cast<int>(fg));

        if (bg != Color::Default)
            add(bgCode(bg));

        seq += 'm';
        return seq;
    }

    inline std::string reset()
    {
        return "\033[0m";
    }

    // Wrap a string with color/style and reset at the end
    inline std::string colorize(const std::string& text,
        Color fg = Color::Default,
        Color bg = Color::Default,
        bool bold = false,
        bool underline = false)
    {
        return makeStyle(fg, bg, bold, underline) + text + reset();
    }
}
