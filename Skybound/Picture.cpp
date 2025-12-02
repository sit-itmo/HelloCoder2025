#include "Skybound.h"

#pragma once
#include <cstring>     // memcpy, memset
#include <algorithm>   // std::fill, std::min
#include <utility>     // std::swap

typedef unsigned int sColor; // 0xAARRGGBB

// Helper: alpha blend src over dst (both 0xAARRGGBB)
sColor Picture::AlphaBlend(sColor dst, sColor src)
{
    unsigned int srcA = (src >> 24) & 0xFF;
    if (srcA == 0)   return dst; // fully transparent
    if (srcA == 255) return src; // fully opaque

    unsigned int dstA = (dst >> 24) & 0xFF;

    unsigned int srcR = (src >> 16) & 0xFF;
    unsigned int srcG = (src >> 8) & 0xFF;
    unsigned int srcB = (src) & 0xFF;

    unsigned int dstR = (dst >> 16) & 0xFF;
    unsigned int dstG = (dst >> 8) & 0xFF;
    unsigned int dstB = (dst) & 0xFF;

    unsigned int invA = 255 - srcA;

    unsigned int outA = srcA + (dstA * invA) / 255;
    unsigned int outR = (srcR * srcA + dstR * invA) / 255;
    unsigned int outG = (srcG * srcA + dstG * invA) / 255;
    unsigned int outB = (srcB * srcA + dstB * invA) / 255;

    return (outA << 24) | (outR << 16) | (outG << 8) | outB;
}

Picture::Picture()
    : _Size(0, 0)
    , pPixels(nullptr)
{
}

Picture::Picture(Size2D size)
    : _Size(size)
{
    if (_Size.W > 0 && _Size.H > 0)
        pPixels = new sColor[_Size.W * _Size.H];
    else
        pPixels = nullptr;
}

Picture::Picture(unsigned int w, unsigned int h)
    : Picture(Size2D(w, h))
{
}

Picture::Picture(const Picture& other)
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

Picture::Picture(Picture&& other) noexcept
    : _Size(other._Size)
    , pPixels(other.pPixels)
{
    other._Size = Size2D(0, 0);
    other.pPixels = nullptr;
}

Picture& Picture::operator=(const Picture& other)
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

Picture& Picture::operator=(Picture&& other) noexcept
{
    if (this == &other)
        return *this;

    delete[] pPixels;

    _Size = other._Size;
    pPixels = other.pPixels;

    other._Size = Size2D(0, 0);
    other.pPixels = nullptr;

    return *this;
}

Picture::~Picture()
{
    if (pPixels != nullptr)
    {
        delete[] pPixels;
    }
    pPixels = nullptr;
}

void Picture::Resize(Size2D newSize)
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

void Picture::Clear(sColor color)
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
void Picture::DrawPicture(const Picture& src,
    const Pos2D& srcPos, const Size2D& srcSize,
    const Pos2D& dstPos, const Size2D& dstSize)
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
            dstRef = AlphaBlend(dstRef, srcColor);
        }
    }
}

