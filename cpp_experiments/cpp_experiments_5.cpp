#include <stdio.h>
#include <stdlib.h>

struct IDumpable
{
    virtual void Dump() const=0;
};

struct Wheel : public IDumpable
{
    Wheel() { printf("\n+[(0x%x)Wheel]", this); }
    ~Wheel() { printf("\n-[(0x%x)Wheel]", this); }
    void Dump() const { printf("\n![(0x%x)Wheel]::Dump", this); }
};

struct Car : public IDumpable
{
    Wheel Wheels[4];
    Car() { printf("\n+[(0x%x)Car]", this); }
    ~Car() { printf("\n-[(0x%x)Car]", this); }
    virtual void Dump() const
    {
        printf("\n![(0x%x)Car]::Dump", this);
        for (const auto& w : Wheels)
        {
            w.Dump();
        }
    }
};

struct Color : public IDumpable
{
    char R, G, B;
    Color() { printf("\n+[(0x%x)Color]", this); }
    ~Color() { printf("\n-[(0x%x)Color]", this); }
    void Dump() const { printf("\n![(0x%x)Color]::Dump", this); }
};

struct RivalCar : private Car
{
    Color RivalColor;
    RivalCar() { printf("\n+[(0x%x)RivalCar]", this); }
    ~RivalCar() { printf("\n-[(0x%x)RivalCar]", this); }
    void Dump()const
    {
        Car::Dump();
        printf("\n![(0x%x)RivalCar]::Dump", this);
        RivalColor.Dump();
    }
};

struct PlayerCar : public Car
{
    const char* pPlayrName;
    PlayerCar(const char* p_name) : pPlayrName(p_name) { printf("\n+[(0x%x)PlayerCar]", this); }
    ~PlayerCar() { printf("\n-[(0x%x)PlayerCar]", this); }
};

struct TrafficCar : public Car
{
    int Rank;
    TrafficCar() { Rank = rand(); printf("\n+[(0x%x)TrafficCar]", this); }
    ~TrafficCar() { printf("\n-[(0x%x)TrafficCar]", this); }
};

void TestCar(Car* p_car)
{
    printf("\n++++++++++++++++++++++++++++");
    p_car->Dump();
    printf("\n++++++++++++++++++++++++++++");
}

int main_v5()
{
    
    {
        Car c;
        Car cc;
        RivalCar rc;

        RivalCar *p_rc = &rc;
        //Car* p_c = &rc;


        //IDumpable* p_dumpable[] =
        //{
        //    &c, &c.Wheels[0], &rc, &rc.RivalColor
        //};
        //
        //for (auto* p : p_dumpable)
        //{
        //    p->Dump();
        //}


        //TestCar(p_c);
        printf("\n----------------------------");
        p_rc->Dump();
        //c.Wheels[0].Dump();
        //c.Dump();
        //((Car)c).Dump();
        //((Car&)c).Dump();
        //((Car*)&c)->Dump();
        printf("\n----------------------------");
    }

    return 0;
}


#if 0
struct Math
{
private:
    Math() {};
public:
    static float Cos(float x);
    float Sin(float x);
    static float Tan(float x);
    static float Sqrt(float x);

};

struct A
{
private:
    A() {}
    int x = 0;


public:
    static A* Create()
    {
        return new A();
    }
};


void test()
{
    //A a;
    A* p_a = A::Create();
}

struct Car
{
private:
    Wheel* Wheels[4];
public:
    Car()
    {
        Wheels[0] = new Wheel();
        Wheels[1] = new Wheel();
        Wheels[2] = new Wheel();
        Wheels[3] = new Wheel();
    }
    ~Car()
    {
        //if (Wheels[0] != nullptr) delete Wheels[0];
        //if (Wheels[1] != nullptr) delete Wheels[1];
        //if (Wheels[2] != nullptr) delete Wheels[2];
        //if (Wheels[3] != nullptr) delete Wheels[3];

        Wheels[0] ? (delete Wheels[0], 0) : 0;
        Wheels[1] ? (delete Wheels[1], 0) : 0;
        Wheels[2] ? (delete Wheels[2], 0) : 0;
        Wheels[3] ? (delete Wheels[3], 0) : 0;
    }

};
#endif

