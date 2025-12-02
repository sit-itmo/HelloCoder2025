#pragma once

#pragma once
#include <cmath>    
#include <cstdint> 

struct Size2D
{
    unsigned int W = 0;
    unsigned int H = 0;

    constexpr Size2D() noexcept : W(0), H(0) {}
    constexpr Size2D(unsigned int w, unsigned int h) noexcept
        : W(w), H(h) {}
    explicit constexpr Size2D(unsigned int v) noexcept: W(v), H(v) {}
    explicit constexpr Size2D(const struct Pos2D& p) noexcept;
    explicit Size2D(const struct Vec2D& v) noexcept;
    
    constexpr bool IsZero() const noexcept { return W == 0 || H == 0; }

    Size2D& operator+=(const Size2D& rhs) noexcept
    {
        W += rhs.W;
        H += rhs.H;
        return *this;
    }

    Size2D& operator-=(const Size2D& rhs) noexcept
    {
        W -= rhs.W;
        H -= rhs.H;
        return *this;
    }

    Size2D& operator*=(float s) noexcept
    {
        W = static_cast<unsigned int>(W * s);
        H = static_cast<unsigned int>(H * s);
        return *this;
    }

    Size2D& operator/=(float s) noexcept
    {
        W = static_cast<unsigned int>(W / s);
        H = static_cast<unsigned int>(H / s);
        return *this;
    }

    explicit operator struct Vec2D() const noexcept;
    explicit operator struct Pos2D() const noexcept;
};

struct Pos2D
{
    int X = 0;
    int Y = 0;

    constexpr Pos2D() noexcept : X(0), Y(0) {}
    constexpr Pos2D(int x, int y) noexcept : X(x), Y(y) { }

    explicit constexpr Pos2D(int v) noexcept : X(v), Y(v) { }

    explicit constexpr Pos2D(const Size2D& s) noexcept
        : X(static_cast<int>(s.W))
        , Y(static_cast<int>(s.H)) {}

    explicit Pos2D(const struct Vec2D& v) noexcept;

    constexpr bool IsZero() const noexcept { return X == 0 && Y == 0; }

    Pos2D& operator+=(const Pos2D& rhs) noexcept
    {
        X += rhs.X;
        Y += rhs.Y;
        return *this;
    }

    Pos2D& operator-=(const Pos2D& rhs) noexcept
    {
        X -= rhs.X;
        Y -= rhs.Y;
        return *this;
    }

    Pos2D& operator+=(const Size2D& s) noexcept
    {
        X += static_cast<int>(s.W);
        Y += static_cast<int>(s.H);
        return *this;
    }

    Pos2D& operator-=(const Size2D& s) noexcept
    {
        X -= static_cast<int>(s.W);
        Y -= static_cast<int>(s.H);
        return *this;
    }

    Pos2D& operator+=(const struct Vec2D& v) noexcept;
    Pos2D& operator-=(const struct Vec2D& v) noexcept;

    Pos2D& operator*=(float s) noexcept
    {
        X = static_cast<int>(X * s);
        Y = static_cast<int>(Y * s);
        return *this;
    }

    Pos2D& operator/=(float s) noexcept
    {
        X = static_cast<int>(X / s);
        Y = static_cast<int>(Y / s);
        return *this;
    }

    explicit operator Vec2D() const noexcept;
    explicit operator Size2D() const noexcept;
};

struct Vec2D
{
    float x;
    float y;

    constexpr Vec2D() noexcept : x(0.0f), y(0.0f) {}
    constexpr Vec2D(float _x, float _y) noexcept : x(_x), y(_y) {}
    explicit constexpr Vec2D(float v) noexcept : x(v), y(v) {}

    constexpr Vec2D(int _x, int _y) noexcept
        : x(static_cast<float>(_x))
        , y(static_cast<float>(_y)) {}

    constexpr Vec2D(unsigned int _w, unsigned int _h) noexcept
        : x(static_cast<float>(_w))
        , y(static_cast<float>(_h)) {}

    explicit constexpr Vec2D(const Pos2D& p) noexcept
        : x(static_cast<float>(p.X))
        , y(static_cast<float>(p.Y)) {}

    explicit constexpr Vec2D(const Size2D& s) noexcept
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

    Vec2D& Normalize() noexcept
    {
        const float len = Length();
        if (len != 0.0f)
        {
            x /= len;
            y /= len;
        }
        return *this;
    }

    Vec2D& operator+=(const Vec2D& rhs) noexcept
    {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    Vec2D& operator-=(const Vec2D& rhs) noexcept
    {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    Vec2D& operator*=(float s) noexcept
    {
        x *= s;
        y *= s;
        return *this;
    }

    Vec2D& operator/=(float s) noexcept
    {
        x /= s;
        y /= s;
        return *this;
    }

    Vec2D& operator+=(const Pos2D& p) noexcept
    {
        x += static_cast<float>(p.X);
        y += static_cast<float>(p.Y);
        return *this;
    }

    Vec2D& operator+=(const Size2D& s) noexcept
    {
        x += static_cast<float>(s.W);
        y += static_cast<float>(s.H);
        return *this;
    }

    explicit operator Pos2D() const noexcept
    {
        return Pos2D(static_cast<int>(x), static_cast<int>(y));
    }

    explicit operator Size2D() const noexcept
    {
        return Size2D(
            static_cast<unsigned int>(x),
            static_cast<unsigned int>(y)
        );
    }
};

inline constexpr Size2D::Size2D(const Pos2D& p) noexcept
    : W(static_cast<unsigned int>(p.X))
    , H(static_cast<unsigned int>(p.Y))
{}

inline Size2D::Size2D(const Vec2D& v) noexcept
    : W(static_cast<unsigned int>(v.x))
    , H(static_cast<unsigned int>(v.y))
{}

inline Size2D::operator Vec2D() const noexcept
{
    return Vec2D(static_cast<float>(W), static_cast<float>(H));
}

inline Size2D::operator Pos2D() const noexcept
{
    return Pos2D(static_cast<int>(W), static_cast<int>(H));
}

inline Pos2D::Pos2D(const Vec2D& v) noexcept
    : X(static_cast<int>(v.x))
    , Y(static_cast<int>(v.y))
{}

inline Pos2D& Pos2D::operator+=(const Vec2D& v) noexcept
{
    X += static_cast<int>(v.x);
    Y += static_cast<int>(v.y);
    return *this;
}

inline Pos2D& Pos2D::operator-=(const Vec2D& v) noexcept
{
    X -= static_cast<int>(v.x);
    Y -= static_cast<int>(v.y);
    return *this;
}

inline Pos2D::operator Vec2D() const noexcept
{
    return Vec2D(static_cast<float>(X), static_cast<float>(Y));
}

inline Pos2D::operator Size2D() const noexcept
{
    return Size2D(
        static_cast<unsigned int>(X),
        static_cast<unsigned int>(Y)
    );
}

inline bool operator==(const Size2D& a, const Size2D& b) noexcept
{
    return a.W == b.W && a.H == b.H;
}

inline bool operator!=(const Size2D& a, const Size2D& b) noexcept
{
    return !(a == b);
}

inline bool operator==(const Pos2D& a, const Pos2D& b) noexcept
{
    return a.X == b.X && a.Y == b.Y;
}

inline bool operator!=(const Pos2D& a, const Pos2D& b) noexcept
{
    return !(a == b);
}

inline bool operator==(const Vec2D& a, const Vec2D& b) noexcept
{
    return a.x == b.x && a.y == b.y;
}

inline bool operator!=(const Vec2D& a, const Vec2D& b) noexcept
{
    return !(a == b);
}

inline Size2D operator+(Size2D lhs, const Size2D& rhs) noexcept
{
    lhs += rhs;
    return lhs;
}

inline Size2D operator-(Size2D lhs, const Size2D& rhs) noexcept
{
    lhs -= rhs;
    return lhs;
}

inline Size2D operator*(Size2D lhs, float s) noexcept
{
    lhs *= s;
    return lhs;
}

inline Size2D operator*(float s, Size2D rhs) noexcept
{
    rhs *= s;
    return rhs;
}

inline Size2D operator/(Size2D lhs, float s) noexcept
{
    lhs /= s;
    return lhs;
}

inline Pos2D operator+(Pos2D lhs, const Pos2D& rhs) noexcept
{
    lhs += rhs;
    return lhs;
}

inline Pos2D operator-(Pos2D lhs, const Pos2D& rhs) noexcept
{
    lhs -= rhs;
    return lhs;
}

inline Pos2D operator+(Pos2D lhs, const Size2D& rhs) noexcept
{
    lhs += rhs;
    return lhs;
}

inline Pos2D operator-(Pos2D lhs, const Size2D& rhs) noexcept
{
    lhs -= rhs;
    return lhs;
}

inline Vec2D operator+(const Pos2D& p, const Vec2D& v) noexcept
{
    return Vec2D(static_cast<float>(p.X) + v.x,
        static_cast<float>(p.Y) + v.y);
}

inline Vec2D operator+(const Vec2D& v, const Pos2D& p) noexcept
{
    return Vec2D(v.x + static_cast<float>(p.X),
        v.y + static_cast<float>(p.Y));
}

inline Vec2D operator-(const Pos2D& p, const Vec2D& v) noexcept
{
    return Vec2D(static_cast<float>(p.X) - v.x,
        static_cast<float>(p.Y) - v.y);
}

inline Pos2D operator*(Pos2D lhs, float s) noexcept
{
    lhs *= s;
    return lhs;
}

inline Pos2D operator*(float s, Pos2D rhs) noexcept
{
    rhs *= s;
    return rhs;
}

inline Pos2D operator/(Pos2D lhs, float s) noexcept
{
    lhs /= s;
    return lhs;
}

inline Vec2D operator+(Vec2D lhs, const Vec2D& rhs) noexcept
{
    lhs += rhs;
    return lhs;
}

inline Vec2D operator-(Vec2D lhs, const Vec2D& rhs) noexcept
{
    lhs -= rhs;
    return lhs;
}

inline Vec2D operator*(Vec2D lhs, float s) noexcept
{
    lhs *= s;
    return lhs;
}

inline Vec2D operator*(float s, Vec2D rhs) noexcept
{
    rhs *= s;
    return rhs;
}

inline Vec2D operator/(Vec2D lhs, float s) noexcept
{
    lhs /= s;
    return lhs;
}

inline Vec2D operator+(Vec2D lhs, const Size2D& rhs) noexcept
{
    lhs += rhs;
    return lhs;
}

inline Vec2D operator+(Vec2D lhs, const Pos2D& rhs) noexcept
{
    lhs += rhs;
    return lhs;
}

inline Vec2D operator-(const Vec2D& v, const Pos2D& p) noexcept
{
    return Vec2D(v.x - static_cast<float>(p.X),
        v.y - static_cast<float>(p.Y));
}

inline Vec2D operator-(const Vec2D& v, const Size2D& s) noexcept
{
    return Vec2D(v.x - static_cast<float>(s.W),
        v.y - static_cast<float>(s.H));
}
