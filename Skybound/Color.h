#pragma once

//typedef unsigned int sColor; // assumed 32-bit: 0xAARRGGBB
struct sColor
{
    union
    {
        unsigned int value;       // full 32-bit color value

        struct
        {
            // NOTE: layout assumes 0xAARRGGBB in memory on a little-endian machine
            uint8_t b;      // Blue
            uint8_t g;      // Green
            uint8_t r;      // Red
            uint8_t a;      // Alpha
        } comp;
    };

    // ----- Constructors -----

    sColor() : value(0) {} // default: transparent black

    sColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
    {
        comp.r = r;
        comp.g = g;
        comp.b = b;
        comp.a = a;
    }

    sColor(unsigned int v)
    {
        value = v;
    }

    // ----- Cast operators -----

    // Explicit cast to unsigned int (same as sColor)
    explicit operator unsigned int() const
    {
        return value;
    }

    // Assignment from sColor
    sColor& operator=(unsigned int v)
    {
        value = v;
        return *this;
    }

    // ----- Convenience getters/setters for components -----

    uint8_t R() const { return comp.r; }
    uint8_t G() const { return comp.g; }
    uint8_t B() const { return comp.b; }
    uint8_t A() const { return comp.a; }

    void SetR(uint8_t r_) { comp.r = r_; }
    void SetG(uint8_t g_) { comp.g = g_; }
    void SetB(uint8_t b_) { comp.b = b_; }
    void SetA(uint8_t a_) { comp.a = a_; }

    // ----- Alpha blend -----
    //
    // This color is taken as "source" (with alpha),
    // 'dst' is the background color.
    //
    // Result = src OVER dst:
    //   out = src * a + dst * (1 - a)
    //
    sColor Blend(const sColor& dst) const
    {
        // normalize alpha to [0..255]
        uint32_t a = comp.a;
        uint32_t na = 255u - a;

        sColor out;
        out.comp.a = 255; // usually result alpha is fully opaque, or a + dst.a*na/255 if you want

        out.comp.r = static_cast<uint8_t>(
            (comp.r * a + dst.comp.r * na) / 255u
            );
        out.comp.g = static_cast<uint8_t>(
            (comp.g * a + dst.comp.g * na) / 255u
            );
        out.comp.b = static_cast<uint8_t>(
            (comp.b * a + dst.comp.b * na) / 255u
            );

        return out;
    }

    // Static version using raw sColor values
    static sColor Blend(sColor src, sColor dst)
    {
        sColor s(src);
        sColor d(dst);
        sColor o = s.Blend(d);
        return o.value;
    }
};
