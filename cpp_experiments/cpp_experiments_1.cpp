#include <stdio.h>
#include "buf.h"

struct S
{
    int x;
    //S();
    //S(const S&);
    //~S();
    //S operator=(const S&);

    //S operator=(const S&&);
    //S(const S&&);
};


struct Vector3D
{
    float X, Y, Z;

    int Sum();
    int Sum(int);

    void Printer() 
    {
        printf("\n0x%08x:Vector3D(%f, %f, %f)", this, X, Y, Z);
    }

    //Vector3D &other
    //Vector3D *other

    Vector3D operator=(const Vector3D& other)
    {
        X = other.X;
        Y = other.Y;
        Z = other.Z;
        return *this;
    }


    Vector3D(const Vector3D &other) 
    {
        X = other.X;
        Y = other.Y;
        Z = other.Z;
        printf("\n0x%08x:Vector3D[Vector3D(other)]", this); 
    }

    Vector3D() { printf("\n0x%08x:Vector3D[Vector3D()]", this); }
    Vector3D(float x, float y, float z);
    Vector3D(const char* p_tag) { printf("Tag: %s", p_tag); }
    ~Vector3D() { printf("\n0x%08x:~Vector3D[DESTRUCTOR]", this); }
    
};

Vector3D::Vector3D(float x, float y, float z)
{
    printf("\n0x%08x:Vector3D[Vector3D(x,y,z)]", this);
    X = x;
    Y = y;
    Z = z;
}

int Vector3D::Sum(int u)
{
    return (X + Y + Z) * u;
}

int Vector3D::Sum()
{
    int X = 10;
    //this ===> Vector3D*
    return this->X + Y + this->Z;
}

int TestArg(Vector3D vec)
{
    vec.Printer();
    vec.X += 1000;
    vec.Printer();
    return 0;
}

int TestArgRef(Vector3D &vec)
{
    vec.Printer();
    vec.X += 1000;
    vec.Printer();
    return 0;
}

int TestArgPtr(Vector3D* vec)
{
    vec->Printer();
    vec->X += 1000;
    vec->Printer();
    return 0;
}

int main_v1(int argc, char* argv[])
{
    Vector3D vv1 = {1, 2, 3};
    Vector3D vv2 = vv1;
    Vector3D vv3;
    vv1.X += 10;
    vv1.Y += 10;
    vv1.Z += 10;

    vv3 = vv2 = vv1;

    Vector3D v1 = { 10, 10, 10 };
    Vector3D v2 = { 20, 20, 20 };
    Vector3D& rv = v1;
    Vector3D* pv = &rv;

    v1.Printer();
    TestArg(v1);
    v1.Printer();

    TestArgRef(v1);
    v1.Printer();

    //TestArgRef(10);
    v1.Printer();

    //other++;

    Vector3D *other = &v1;
    other->X += 10;
    (*other).X += 10;
    
    Vector3D &other_ref = v1;

    (*other).X += 10;
    other_ref.X += 10;
    
    (*other) = v2;
    other_ref = v2;

    rv = v2;
    
    v1.~Vector3D();

    int x = 42, y = 13;
    int& ref = x; // ссылка на x
    ref += 100;
    ref = y;
    ref += 100;

    v1.Printer();
    TestArg(v1);
    v1.Printer();

    TestArgRef(v1);
    v1.Printer();

    //TestArgRef(10);
    v1.Printer();




#if 0
    register int x = 10;
    Vector3D v2 = { "Hello!" };
    SumTest((int)0xFFFFFFFFFFULL, 0xFFFFFFFFFFULL);
    SumTest(55.6f, 57.4f);
    SumTest(10, 11);

    Test_c(1);

    v1.X = 10;
    v1.Y = 10;
    v1.Z = 10;

    float f = v1.Sum(10);
    {
        Vector3D v2;
        v2.X = 20;
        v2.Y = 20;
        v2.Z = 20;
        float f = v2.Sum();

    }
#endif

    return 0;
}

extern "C" void Test_c(int);

extern "C" int SumTest(int x, int y)
{
    printf("[%d + %d = %d]", x, y, x + y);
    return x + y;
}

float SumTest(float x, float y)
{
    printf("[%f + %f = %f]", x, y, x + y);
    return x + y;
}
