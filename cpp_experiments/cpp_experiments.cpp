#include <stdio.h>

#include <queue>
#include <map>
#include <string>
#include <vector>
#include <ios>
#include <iostream>

//STL
class T
{
    std::vector<int> test;
};

class A {};
class X {};
class C {};


class Magic : public X, public C {};

//template <typename T> void Test() {}
//template <> void Test<Magic>()
//{
//}
//template <> void Test<C>()
//{
//}



template <typename T, typename B>
struct IsInherit
{
    static int Test(B*);
    static char Test(...);
    enum { Value = sizeof(Test((T*)0)) > 1 };
};

int main()
{
    std::cout << "Hello!" << 10;
    return 0;
}

using namespace std;

int test_map()
{
    const int result = IsInherit<Magic, X>::Value;
    std::map<std::string, int> Values;

    Values["Hello!"] = 1;
    Values["Test"] = 2;

    int y = Values["Test"];
    //std::map<std::string, int>::iterator g = Values.find("Hello!");
    //auto g = Values.find("Hello!");
    map<string, int>::iterator g = Values.find("Hello!");
    if (g == Values.end())
    {
        // Not found
    }
    else
    {
        int yy = g->second;
    }

    return 0;

}

int test_string()
{
    std::string test = "sdfsdfds";

    test += "!!!!";
    test = "[" + test + "]";

    printf("Hello %s!", test.c_str());
    
    return 0;
}


int test_vector()
{
    std::vector<int> test;

    test.push_back(1);
    test.push_back(2);
    test.push_back(3);
    test.push_back(4);
   
    std::vector<int>::reverse_iterator it;
    for (it = test.rbegin(); it != test.rend(); it++)
    {
        int t = *it;
        *it = *it + 1;
        printf("\n%d:= %d!", it - test.rbegin(), *it);
    }

    for (int i = 0; i < test.size(); i++)
    {
        int t = test[i];
        printf("\n%d:= %d!", i, t);
    }

    test.clear();




    return 0;
}



