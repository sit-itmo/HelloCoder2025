#pragma once

#pragma once
#include <cmath>    
#include <cstdint> 

struct sSize2D
{
    unsigned int W = 0;
    unsigned int H = 0;

    constexpr sSize2D() noexcept : W(0), H(0) {}
    constexpr sSize2D(unsigned int w, unsigned int h) noexcept
        : W(w), H(h) {}
    explicit constexpr sSize2D(unsigned int v) noexcept: W(v), H(v) {}
    explicit constexpr sSize2D(const struct sPos2D& p) noexcept;
    explicit sSize2D(const struct sVec2D& v) noexcept;
    
    constexpr bool IsZero() const noexcept { return W == 0 || H == 0; }

    sSize2D& operator+=(const sSize2D& rhs) noexcept
    {
        W += rhs.W;
        H += rhs.H;
        return *this;
    }

    sSize2D& operator-=(const sSize2D& rhs) noexcept
    {
        W -= rhs.W;
        H -= rhs.H;
        return *this;
    }

    sSize2D& operator*=(float s) noexcept
    {
        W = static_cast<unsigned int>(W * s);
        H = static_cast<unsigned int>(H * s);
        return *this;
    }

    sSize2D& operator/=(float s) noexcept
    {
        W = static_cast<unsigned int>(W / s);
        H = static_cast<unsigned int>(H / s);
        return *this;
    }

    explicit operator struct sVec2D() const noexcept;
    explicit operator struct sPos2D() const noexcept;
};

struct sPos2D
{
    int X = 0;
    int Y = 0;

    constexpr sPos2D() noexcept : X(0), Y(0) {}
    constexpr sPos2D(int x, int y) noexcept : X(x), Y(y) { }

    explicit constexpr sPos2D(int v) noexcept : X(v), Y(v) { }

    explicit constexpr sPos2D(const sSize2D& s) noexcept
        : X(static_cast<int>(s.W))
        , Y(static_cast<int>(s.H)) {}

    explicit sPos2D(const struct sVec2D& v) noexcept;

    constexpr bool IsZero() const noexcept { return X == 0 && Y == 0; }

    sPos2D& operator+=(const sPos2D& rhs) noexcept
    {
        X += rhs.X;
        Y += rhs.Y;
        return *this;
    }

    sPos2D& operator-=(const sPos2D& rhs) noexcept
    {
        X -= rhs.X;
        Y -= rhs.Y;
        return *this;
    }

    sPos2D& operator+=(const sSize2D& s) noexcept
    {
        X += static_cast<int>(s.W);
        Y += static_cast<int>(s.H);
        return *this;
    }

    sPos2D& operator-=(const sSize2D& s) noexcept
    {
        X -= static_cast<int>(s.W);
        Y -= static_cast<int>(s.H);
        return *this;
    }

    sPos2D& operator+=(const struct sVec2D& v) noexcept;
    sPos2D& operator-=(const struct sVec2D& v) noexcept;

    sPos2D& operator*=(float s) noexcept
    {
        X = static_cast<int>(X * s);
        Y = static_cast<int>(Y * s);
        return *this;
    }

    sPos2D& operator/=(float s) noexcept
    {
        X = static_cast<int>(X / s);
        Y = static_cast<int>(Y / s);
        return *this;
    }

    explicit operator sVec2D() const noexcept;
    explicit operator sSize2D() const noexcept;
};

struct sVec2D
{
    float x;
    float y;

    constexpr sVec2D() noexcept : x(0.0f), y(0.0f) {}
    constexpr sVec2D(float _x, float _y) noexcept : x(_x), y(_y) {}
    explicit constexpr sVec2D(float v) noexcept : x(v), y(v) {}

    constexpr sVec2D(int _x, int _y) noexcept
        : x(static_cast<float>(_x))
        , y(static_cast<float>(_y)) {}

    constexpr sVec2D(unsigned int _w, unsigned int _h) noexcept
        : x(static_cast<float>(_w))
        , y(static_cast<float>(_h)) {}

    explicit constexpr sVec2D(const sPos2D& p) noexcept
        : x(static_cast<float>(p.X))
        , y(static_cast<float>(p.Y)) {}

    explicit constexpr sVec2D(const sSize2D& s) noexcept
        : x(static_cast<float>(s.W))
        , y(static_cast<float>(s.H)) {}

    float Length() const noexcept
    {
        return std::sqrt(x * x + y * y);
    }

    float LengthSq() const noexcept
    {
        return x * x + y * y;
    }

    sVec2D& Normalize() noexcept
    {
        const float len = Length();
        if (len != 0.0f)
        {
            x /= len;
            y /= len;
        }
        return *this;
    }

    sVec2D& operator+=(const sVec2D& rhs) noexcept
    {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    sVec2D& operator-=(const sVec2D& rhs) noexcept
    {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    sVec2D& operator*=(float s) noexcept
    {
        x *= s;
        y *= s;
        return *this;
    }

    sVec2D& operator/=(float s) noexcept
    {
        x /= s;
        y /= s;
        return *this;
    }

    sVec2D& operator+=(const sPos2D& p) noexcept
    {
        x += static_cast<float>(p.X);
        y += static_cast<float>(p.Y);
        return *this;
    }

    sVec2D& operator+=(const sSize2D& s) noexcept
    {
        x += static_cast<float>(s.W);
        y += static_cast<float>(s.H);
        return *this;
    }

    explicit operator sPos2D() const noexcept
    {
        return sPos2D(static_cast<int>(x), static_cast<int>(y));
    }

    explicit operator sSize2D() const noexcept
    {
        return sSize2D(
            static_cast<unsigned int>(x),
            static_cast<unsigned int>(y)
        );
    }
};

inline constexpr sSize2D::sSize2D(const sPos2D& p) noexcept
    : W(static_cast<unsigned int>(p.X))
    , H(static_cast<unsigned int>(p.Y))
{}

inline sSize2D::sSize2D(const sVec2D& v) noexcept
    : W(static_cast<unsigned int>(v.x))
    , H(static_cast<unsigned int>(v.y))
{}

inline sSize2D::operator sVec2D() const noexcept
{
    return sVec2D(static_cast<float>(W), static_cast<float>(H));
}

inline sSize2D::operator sPos2D() const noexcept
{
    return sPos2D(static_cast<int>(W), static_cast<int>(H));
}

inline sPos2D::sPos2D(const sVec2D& v) noexcept
    : X(static_cast<int>(v.x))
    , Y(static_cast<int>(v.y))
{}

inline sPos2D& sPos2D::operator+=(const sVec2D& v) noexcept
{
    X += static_cast<int>(v.x);
    Y += static_cast<int>(v.y);
    return *this;
}

inline sPos2D& sPos2D::operator-=(const sVec2D& v) noexcept
{
    X -= static_cast<int>(v.x);
    Y -= static_cast<int>(v.y);
    return *this;
}

inline sPos2D::operator sVec2D() const noexcept
{
    return sVec2D(static_cast<float>(X), static_cast<float>(Y));
}

inline sPos2D::operator sSize2D() const noexcept
{
    return sSize2D(
        static_cast<unsigned int>(X),
        static_cast<unsigned int>(Y)
    );
}

inline bool operator==(const sSize2D& a, const sSize2D& b) noexcept
{
    return a.W == b.W && a.H == b.H;
}

inline bool operator!=(const sSize2D& a, const sSize2D& b) noexcept
{
    return !(a == b);
}

inline bool operator==(const sPos2D& a, const sPos2D& b) noexcept
{
    return a.X == b.X && a.Y == b.Y;
}

inline bool operator!=(const sPos2D& a, const sPos2D& b) noexcept
{
    return !(a == b);
}

inline bool operator==(const sVec2D& a, const sVec2D& b) noexcept
{
    return a.x == b.x && a.y == b.y;
}

inline bool operator!=(const sVec2D& a, const sVec2D& b) noexcept
{
    return !(a == b);
}

inline sSize2D operator+(sSize2D lhs, const sSize2D& rhs) noexcept
{
    lhs += rhs;
    return lhs;
}

inline sSize2D operator-(sSize2D lhs, const sSize2D& rhs) noexcept
{
    lhs -= rhs;
    return lhs;
}

inline sSize2D operator*(sSize2D lhs, float s) noexcept
{
    lhs *= s;
    return lhs;
}

inline sSize2D operator*(float s, sSize2D rhs) noexcept
{
    rhs *= s;
    return rhs;
}

inline sSize2D operator/(sSize2D lhs, float s) noexcept
{
    lhs /= s;
    return lhs;
}

inline sPos2D operator+(sPos2D lhs, const sPos2D& rhs) noexcept
{
    lhs += rhs;
    return lhs;
}

inline sPos2D operator-(sPos2D lhs, const sPos2D& rhs) noexcept
{
    lhs -= rhs;
    return lhs;
}

inline sPos2D operator+(sPos2D lhs, const sSize2D& rhs) noexcept
{
    lhs += rhs;
    return lhs;
}

inline sPos2D operator-(sPos2D lhs, const sSize2D& rhs) noexcept
{
    lhs -= rhs;
    return lhs;
}

inline sVec2D operator+(const sPos2D& p, const sVec2D& v) noexcept
{
    return sVec2D(static_cast<float>(p.X) + v.x,
        static_cast<float>(p.Y) + v.y);
}

inline sVec2D operator+(const sVec2D& v, const sPos2D& p) noexcept
{
    return sVec2D(v.x + static_cast<float>(p.X),
        v.y + static_cast<float>(p.Y));
}

inline sVec2D operator-(const sPos2D& p, const sVec2D& v) noexcept
{
    return sVec2D(static_cast<float>(p.X) - v.x,
        static_cast<float>(p.Y) - v.y);
}

inline sPos2D operator*(sPos2D lhs, float s) noexcept
{
    lhs *= s;
    return lhs;
}

inline sPos2D operator*(float s, sPos2D rhs) noexcept
{
    rhs *= s;
    return rhs;
}

inline sPos2D operator/(sPos2D lhs, float s) noexcept
{
    lhs /= s;
    return lhs;
}

inline sVec2D operator+(sVec2D lhs, const sVec2D& rhs) noexcept
{
    lhs += rhs;
    return lhs;
}

inline sVec2D operator-(sVec2D lhs, const sVec2D& rhs) noexcept
{
    lhs -= rhs;
    return lhs;
}

inline sVec2D operator*(sVec2D lhs, float s) noexcept
{
    lhs *= s;
    return lhs;
}

inline sVec2D operator*(float s, sVec2D rhs) noexcept
{
    rhs *= s;
    return rhs;
}

inline sVec2D operator/(sVec2D lhs, float s) noexcept
{
    lhs /= s;
    return lhs;
}

inline sVec2D operator+(sVec2D lhs, const sSize2D& rhs) noexcept
{
    lhs += rhs;
    return lhs;
}

inline sVec2D operator+(sVec2D lhs, const sPos2D& rhs) noexcept
{
    lhs += rhs;
    return lhs;
}

inline sVec2D operator-(const sVec2D& v, const sPos2D& p) noexcept
{
    return sVec2D(v.x - static_cast<float>(p.X),
        v.y - static_cast<float>(p.Y));
}

inline sVec2D operator-(const sVec2D& v, const sSize2D& s) noexcept
{
    return sVec2D(v.x - static_cast<float>(s.W),
        v.y - static_cast<float>(s.H));
}
