#include "Skybound.h"

#pragma once
#include <cstring>     // memcpy, memset
#include <algorithm>   // std::fill, std::min
#include <utility>     // std::swap

sPicture::sPicture()
    : _Size(0, 0)
    , pPixels(nullptr)
{
}

sPicture::sPicture(sSize2D size)
    : _Size(size)
{
    if (_Size.W > 0 && _Size.H > 0)
        pPixels = new sColor[_Size.W * _Size.H];
    else
        pPixels = nullptr;
}

sPicture::sPicture(unsigned int w, unsigned int h)
    : sPicture(sSize2D(w, h))
{
}

sPicture::sPicture(const sPicture& other)
    : _Size(other._Size)
{
    if (other.pPixels)
    {
        pPixels = new sColor[_Size.W * _Size.H];
        std::memcpy(pPixels, other.pPixels, sizeof(sColor) * _Size.W * _Size.H);
    }
    else
    {
        pPixels = nullptr;
    }
}

sPicture::sPicture(sPicture&& other) noexcept
    : _Size(other._Size)
    , pPixels(other.pPixels)
{
    other._Size = sSize2D(0, 0);
    other.pPixels = nullptr;
}

sPicture& sPicture::operator=(const sPicture& other)
{
    if (this == &other)
        return *this;

    // If sizes are equal we can reuse buffer
    if (_Size == other._Size && pPixels != nullptr && other.pPixels != nullptr)
    {
        std::memcpy(pPixels, other.pPixels, sizeof(sColor) * _Size.W * _Size.H);
        return *this;
    }

    // Otherwise reallocate
    delete[] pPixels;
    _Size = other._Size;

    if (other.pPixels)
    {
        pPixels = new sColor[_Size.W * _Size.H];
        std::memcpy(pPixels, other.pPixels, sizeof(sColor) * _Size.W * _Size.H);
    }
    else
    {
        pPixels = nullptr;
    }

    return *this;
}

sPicture& sPicture::operator=(sPicture&& other) noexcept
{
    if (this == &other)
        return *this;

    delete[] pPixels;

    _Size = other._Size;
    pPixels = other.pPixels;

    other._Size = sSize2D(0, 0);
    other.pPixels = nullptr;

    return *this;
}

sPicture::~sPicture()
{
    if (pPixels != nullptr)
    {
        delete[] pPixels;
    }
    pPixels = nullptr;
}

void sPicture::Resize(sSize2D newSize)
{
    if (newSize == _Size)
        return;

    sColor* newPixels = nullptr;

    if (newSize.W > 0 && newSize.H > 0)
    {
        const unsigned int newCount = newSize.W * newSize.H;
        newPixels = new sColor[newCount];

        // Initialize all pixels to 0 (transparent black)
        std::fill(newPixels, newPixels + newCount, 0);

        // Copy overlapping region
        if (pPixels)
        {
            const unsigned int copyW = std::min(_Size.W, newSize.W);
            const unsigned int copyH = std::min(_Size.H, newSize.H);

            for (unsigned int y = 0; y < copyH; ++y)
            {
                std::memcpy(
                    newPixels + y * newSize.W,
                    pPixels + y * _Size.W,
                    sizeof(sColor) * copyW
                );
            }
        }
    }

    delete[] pPixels;
    pPixels = newPixels;
    _Size = newSize;
}

void sPicture::Clear(sColor color)
{
    if (!pPixels) return;

    std::fill(pPixels, pPixels + _Size.W * _Size.H, color);
}

// -------------------------------------------------------------
// Drawing another picture with scaling and alpha blending
//
// src       - source picture
// srcPos    - top-left corner of source rect in src (in pixels)
// srcSize   - size of source rect in src
// dstPos    - top-left corner of destination rect in this picture
// dstSize   - size of destination rect in this picture
//
// Scaling is done with nearest-neighbor sampling.
// Colors are blended as src over dst, using 0xAARRGGBB format.
// -------------------------------------------------------------
void sPicture::DrawPicture(const sPicture& src,
    const sPos2D& srcPos, const sSize2D& srcSize,
    const sPos2D& dstPos, const sSize2D& dstSize)
{
    if (!pPixels || !src.pPixels) return;
    if (srcSize.W == 0 || srcSize.H == 0) return;
    if (dstSize.W == 0 || dstSize.H == 0) return;

    // Loop over destination rectangle
    for (unsigned int dy = 0; dy < dstSize.H; ++dy)
    {
        int dstY = dstPos.Y + static_cast<int>(dy);
        if (dstY < 0 || dstY >= static_cast<int>(_Size.H))
            continue;

        // Vertical mapping [0..dstH-1] -> [0..srcH-1]
        float v = (dstSize.H > 1)
            ? static_cast<float>(dy) / static_cast<float>(dstSize.H - 1)
            : 0.0f;

        float srcYf = static_cast<float>(srcPos.Y) + v * static_cast<float>(srcSize.H - 1);
        int   srcY = static_cast<int>(srcYf + 0.5f);

        if (srcY < srcPos.Y || srcY >= srcPos.Y + static_cast<int>(srcSize.H))
            continue;
        if (srcY < 0 || srcY >= static_cast<int>(src._Size.H))
            continue;

        for (unsigned int dx = 0; dx < dstSize.W; ++dx)
        {
            int dstX = dstPos.X + static_cast<int>(dx);
            if (dstX < 0 || dstX >= static_cast<int>(_Size.W))
                continue;

            // Horizontal mapping [0..dstW-1] -> [0..srcW-1]
            float u = (dstSize.W > 1)
                ? static_cast<float>(dx) / static_cast<float>(dstSize.W - 1)
                : 0.0f;

            float srcXf = static_cast<float>(srcPos.X) + u * static_cast<float>(srcSize.W - 1);
            int   srcX = static_cast<int>(srcXf + 0.5f);

            if (srcX < srcPos.X || srcX >= srcPos.X + static_cast<int>(srcSize.W))
                continue;
            if (srcX < 0 || srcX >= static_cast<int>(src._Size.W))
                continue;

            // Fetch source and destination pixels
            sColor srcColor = src.GetPixel(static_cast<unsigned int>(srcX),
                static_cast<unsigned int>(srcY));

            sColor& dstRef = pPixels[dstY * _Size.W + dstX];
            dstRef = srcColor.Blend(dstRef);
        }
    }
}

#include <png.h>
bool sPicture::LoadPNG(const char* p_path)
{
    FILE* fp = fopen(p_path, "rb");
    if (!fp) return false;

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) return false;

    png_infop info = png_create_info_struct(png);
    if (!info) return false;

    if (setjmp(png_jmpbuf(png))) return false;

    png_init_io(png, fp);
    png_read_info(png, info);

    sSize2D pic_size = { png_get_image_width(png, info), png_get_image_height(png, info) };
    Resize(pic_size);

    png_byte color_type = png_get_color_type(png, info);
    png_byte bit_depth = png_get_bit_depth(png, info);

    // преобразуем к RGBA 8-bit
    if (bit_depth == 16) png_set_strip_16(png);
    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png);

    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png);

    if (png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);

    if (color_type == PNG_COLOR_TYPE_RGB ||
        color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_PALETTE)
    {
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER); // добавляем альфа-канал
    }

    if (color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    {
        png_set_gray_to_rgb(png);
    }

    // *** This is the key line: swap R and B so we get BGRA instead of RGBA ***
    png_set_bgr(png);

    png_read_update_info(png, info);

    std::vector<png_bytep> rows(_Size.H);

    for (int y = 0; y < _Size.H; y++)
    {
        rows[y] = (png_bytep)((png_bytep)pPixels + y * _Size.W * 4);
    }
    png_read_image(png, rows.data());
    png_destroy_read_struct(&png, &info, NULL);
    fclose(fp);
    return true;
}



