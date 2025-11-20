#include <stdio.h>
#include <string.h>
#include <malloc.h>

#define TRACE_FUNC {printf("\n%4d:0x%08x:%s", __LINE__, (unsigned int)this, __FUNCSIG__); fflush(stdout);}
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define ENABLE_MOVE 1

class Buf
{
private:
    unsigned long Size = 0;
    void* pData = nullptr;

public:
    inline unsigned long getSize() const { return Size; }
    inline const char* getsDataPtr() const { return (char *)pData; }

    ~Buf();
    Buf();
    Buf(const Buf& other);
    Buf(unsigned long size);
    Buf(unsigned long size, int val);

    void Fill(int v);
    void Print() const;
    void Reset();
    void Realloc(unsigned long size);

    // malloc -> free
    // new -> delete
    // new[] -> delete[]

    void CopyFrom(const Buf& other);
    void operator=(const Buf& other);
#if ENABLE_MOVE
    Buf(Buf&& other);
    void operator=(Buf&& other);
#endif
};

