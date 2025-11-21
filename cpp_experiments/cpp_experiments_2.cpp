#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include <string>

#include "buf.h"

void Test0Arg(Buf&& b1)
{
    b1.Print();
}
void Test1Arg(Buf b1)
{
    b1.Print();
}

void Test2Arg(Buf* b1)
{
    b1->Print();
}

void Test3Arg(Buf& b1)
{
    b1.Print();
}

Buf Test1Ret(unsigned long size, int v)
{
    Buf b{size, v};
    b.Print();
    return b;
}

Buf *Test2Ret(unsigned long size, int v)
{
    Buf b{ size, v };
    return &b;
}

Buf& Test3Ret(unsigned long size, int v)
{
    Buf b{ size, v };
    return b;
}

std::string TestStr(const char* p_name)
{
    std::string y = p_name;
    y = "[" + y + "]";
    return y;
}


int main_v2()
{
    Buf big{100, 11};
    Buf b00;
    Buf b0;
    b0 = Test1Ret(20, 0xAA);
    b0.Print();
    b0 = b00;
    b0.Print();
    
    b0 = (Buf&&)(big);
    b0.Print();
    b00 = (Buf&&)(big);
    b00.Print();

    std::string bbb;
    bbb = TestStr("Hello");
    //b0.pData = "asdasdas";

    Buf b1;
    Buf b2(10, 0xAA);
    //Buf b3 = { 10, 0xBB };
    //Buf b4{10, 0xDD };
    Buf bcc = Test1Ret(20, 0xAA);
    bcc.Print();

    bcc = Test1Ret(20, 0xAA);
    bcc.Print();

    Test1Arg(Test1Ret(20, 0xAA));
    Test0Arg(Test1Ret(20, 0xAA));

    //std::move();

    //b1.Print();
    //b2.Print();
    //b3.Print();
    //b4.Print();


    //Test1Arg(b2);
    //Test2Arg(&b2);
    //Test3Arg(b2);

    //Test2Arg(&Test1Ret(20, 0xAA)); // bad ptr on temp obj
    //Test3Arg(Test1Ret(20, 0xAA)); // Error ref to r-value


    {
        Buf b5 = b2;
        b5.Print();
    }



    return 0;
}

struct S
{
    int x;

    int operator&(const char* p_str)
    {
        return x + strlen(p_str);
    }
};


S test2()
{
    S x;
    x.x = 10;
    return x;
}

int main_test1()
{
    //r-value
    //l-value
    
    //l-value = r-value

    int x = 10;
    int&& ref10 = 10;

    S s = test2();
    
    S& s_ref = s;
    S* s_ptr = &s;

    int y1 = s & "Hello!";
    int y2 = s_ref & "Hello!";
    int y3 = *s_ptr & "Hello!";

    return 0;
}



