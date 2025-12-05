#include "Skybound.h"
#include <ft2build.h>
#include FT_FREETYPE_H

FT_Library g_SkyLib = nullptr;

struct sFreeTypeFont : public sFont
{
    FT_Face Face = nullptr;
    sFreeTypeFont()
    {

    }

    virtual ~sFreeTypeFont()
    {

    }
    virtual bool LoadGlyph(uint32_t codepoint, sGlyph& out);
    bool PreloadGlyphs(sU32 start, sU32 end);
};


sFont::sFont()
{
}


bool sFreeTypeFont::LoadGlyph(uint32_t codepoint, sGlyph& out)
{
    // Загружаем глиф
    if (FT_Load_Char(Face, codepoint, FT_LOAD_RENDER))
    {
        return false;
    }

    FT_GlyphSlot slot = Face->glyph;
    FT_Bitmap& bmp = slot->bitmap;

    out.width = bmp.width;
    out.height = bmp.rows;
    out.bearingX = slot->bitmap_left;
    out.bearingY = slot->bitmap_top;
    out.advance = slot->advance.x >> 6; // из 26.6 fixed в пиксели
    out.bitmap.assign(bmp.buffer, bmp.buffer + bmp.width * bmp.rows);
    return true;
}

bool sFreeTypeFont::PreloadGlyphs(sU32 start, sU32 end)
{
    for (sU32 cp = start; cp <= end; ++cp)
    {
        sGlyph g;
        if (LoadGlyph(cp, g))
        {
            _Chars[cp] = g;
        }
    }
    return true;
}

sFont *sFont::LoadFromFile(const char* p_fileName, float size)
{
    if (g_SkyLib == nullptr)
    {
        if (FT_Init_FreeType(&g_SkyLib))
            return nullptr;
    }
    if (g_SkyLib == nullptr)
    {
        return nullptr;
    }

    sFreeTypeFont* p_font = new sFreeTypeFont();
    if (FT_New_Face(g_SkyLib, p_fileName, 0, &p_font->Face))
    {
        goto ERROR;
    }

    if (FT_Set_Pixel_Sizes(p_font->Face, 0, (FT_UInt)size))
    {
        goto ERROR;
    }

    p_font->PreloadGlyphs(0x20, 0x7E);
    return p_font;

ERROR:
    if (p_font != nullptr)
    {
        delete p_font;
    }
    return nullptr;
}

int sFont::DrawGlyph(uint32_t codepoint, sPicture &pict, sPos2D &pos, sColor c)
{
    int penX = pos.X;
    int baselineY = pos.Y;
    auto it = _Chars.find(codepoint);
    if (it == _Chars.end())
    {
        sGlyph g;
        if (!LoadGlyph(codepoint, g))
        {
            return 0;
        }

        _Chars[codepoint] = g;
        it = _Chars.find(codepoint);
    }

    const sGlyph& glyph = it->second;

    // левый верхний угол глифа
    int x0 = penX + glyph.bearingX;
    int y0 = baselineY - glyph.bearingY; // FT отсчитывает от базовой линии вверх

    for (int y = 0; y < glyph.height; ++y)
    {
        for (int x = 0; x < glyph.width; ++x)
        {
            unsigned char a = glyph.bitmap[y * glyph.width + x];
            if (a == 0) continue;

            float alpha = (a / 255.0f);
            if (alpha <= 0.01f) continue;

            int sx = x0 + x;
            int sy = y0 + y;
            c.SetA(alpha);
            pict.PutPixel(sx, sy, c);
        }
    }

    return glyph.advance;
}

void sFont::PrintText(sPicture& pic, const sPos2D& loc, sColor color, const std::string& text)
{
    int penX = loc.X;
    int y = loc.Y;
    for (unsigned char ch : text)
    {
        if (ch == '\n')
        {
            y += 20;
            penX = loc.X;
            continue;
        }

        sPos2D new_pos = { penX, y};
        penX += DrawGlyph((uint32_t)ch, pic, new_pos, color);
    }
}

