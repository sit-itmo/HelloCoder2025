#include <stdio.h>
#include <malloc.h>



void* operator new(size_t sz, const char *p_file, int line)
{
    void* ptr = malloc(sz);
    printf("\n%s:%d:ALLC %d -> 0x%x!", p_file, line, sz, ptr);
    return ptr;
}

void operator delete(void* ptr, const char* p_file, int line)
{
    printf("\n%s:%d:FREE 0x%x!", p_file, line, ptr);
    free(ptr);
}

#define new new(__FILE__, __LINE__) 

struct Magic
{
    int buf[1000];
    Magic()
    {
        int y = 10;
        y -= 10;
        int x = 100;
        throw 15;
        x /= y;
    }

};

int main_v4()
{
    Magic *p_test = new Magic();
    Magic* p_test2 = new Magic();

    delete p_test;
    return 0;
}
