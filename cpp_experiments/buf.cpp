#include "buf.h"

Buf::Buf(unsigned long size)
{
    TRACE_FUNC;
    Realloc(size);
}

Buf::Buf(unsigned long size, int val)
{
    TRACE_FUNC;
    Realloc(size);
    Fill(val);
}

Buf::Buf()
{
    TRACE_FUNC;
}

void Buf::Fill(int v)
{
    TRACE_FUNC;
    if (Size > sizeof(v))
    {
        for (unsigned int i = 0; i < (Size / sizeof(v)); i++)
        {
            ((int*)pData)[i] = v;
        }
    }
}
    
void Buf::Print() const
{
    printf("\n(0x%08x)0x%08x:[", (unsigned int)this, (unsigned int)this->pData);
    for (unsigned int i = 0; i < Size; i++)
    {
        printf("%02X", ((unsigned char*)pData)[i]);
    }
    printf("]");
    fflush(stdout);
}

void Buf::Reset()
{
    TRACE_FUNC;
    if (pData)
    {
        delete[] pData;
        pData = nullptr;
    }
}

void Buf::Realloc(unsigned long size)
{
    TRACE_FUNC;
    if (size == Size)
    {
        return;
    }
    if (size == 0)
    {
        Reset();
        return;
    }
    void *p_new = new char[size];
    memcpy(p_new, pData, MIN(size, Size));

    delete[] pData;
    pData = p_new;
    Size = size;
}

void Buf::CopyFrom(const Buf& other)
{
    TRACE_FUNC;

    Size = other.Size;
    if (Size == 0 || other.pData == nullptr)
    {
        pData = nullptr;
    }
    else
    {
        pData = new char[Size];
        memcpy(pData, other.pData, other.Size);
    }
}

Buf::Buf(const Buf& other)
{
    TRACE_FUNC;

    CopyFrom(other);
}

Buf::~Buf()
{
    TRACE_FUNC;

    Reset();
}


void Buf::operator=(const Buf& other)
{
    TRACE_FUNC;

    CopyFrom(other);
    //return *this;
}

#if ENABLE_MOVE
void Buf::operator=(Buf&& other)
{
    TRACE_FUNC;
    Reset();
    pData = other.pData;
    Size = other.Size;
    other.pData = nullptr;
    other.Size = 0;
    //return *this;
}

Buf::Buf(Buf&& other)
{
    TRACE_FUNC;
    pData = other.pData;
    Size = other.Size;
    other.pData = nullptr;
    other.Size = 0;
}
#endif

