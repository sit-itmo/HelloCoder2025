#include <stdio.h>
#include <math.h>

//templates


void MagicPrint(int v)
{
    printf("\nMAGIC[0x%x=%d]", v, v);
}

void MagicPrint(long v)
{
    printf("\nMAGIC[0x%x=%d]", v, v);
}

void MagicPrint(short v)
{
    printf("\nMAGIC[0x%x=%d]", v, v);
}

void MagicPrint(float v)
{
    printf("\nMAGIC[%f]", v);
}

void MagicPrint(double v)
{
    printf("\nMAGIC[%f]", (float)v);
}


template<typename T>
void TemplatePrint(T v)
{
    printf("\nMAGIC[0x%x=%d]", (int)v, (int)v);
}

template<typename T>
struct Vec
{
    T x = (T)0, y = (T)0;

    Vec() : x(0), y(0) {}
    Vec(T _x, T _y) : x(_x), y(_y) {}
    Vec(const Vec& v) : x(v.x), y(v.y){}
    Vec operator=(const Vec& v)
    {
        x = v.x; y = v.y; return *this;
    }

    operator int() const
    {
        return (int)(x + y);
    }
    void Dump()
    {
        printf("(%d, %d)", (int)x, (int)y);
    }
};

template<typename T>
Vec<T> operator+(const Vec<T>& l, const Vec<T>& r)
{
    Vec<T> _new(l.x + r.x, l.y + r.y);
    return _new;
}

template<typename T>
Vec<T> operator+(const Vec<T>& l, int r)
{
    Vec<T> _new(l.x + r, l.y + r);
    return _new;
}

template<typename T>
Vec<T> operator+(int l, const Vec<T>& r)
{
    Vec<T> _new(l + r.x, l + r.y);
    return _new;
}

template<typename X, typename Y>
void MagicAdd(X x, Y y)
{
    int r = (int)(x + y);
    printf("\nADD[%d]", r);
}


template<typename X, typename Y>
void MagicAdd2(X x, Y y)
{
    auto r = x + y;
    r.Dump();
}

template<>
void MagicAdd2<int, int>(int x, int y)
{
    int r = x + y;
    printf("(%d)", r);
}


template<int BufSize, typename X>
void MagicPrinter(X x)
{
    char buf[BufSize] = { 0 };
    snprintf(buf, BufSize, ">>!!%d!!<<", (int)x);
    // Send buffer
}

int main_v3()
{
    MagicPrinter<100>(10);


    Vec v(10.0, 20.0);
    Vec v2 = v;

    MagicAdd(v, v2);
    MagicAdd2(v, v2);

    MagicAdd2(v, 20);
    MagicAdd2(10, 10);

    MagicAdd(10.5, 2.4);
    MagicAdd(10, 3.7);



    TemplatePrint(1000);
    TemplatePrint(2000);
    TemplatePrint(1000.0);
    //MagicPrint(100);
    //MagicPrint(100L);
    //MagicPrint(100.0f);
    //MagicPrint(100.0);
    //MagicPrint((short)100);


    return 0;
}

