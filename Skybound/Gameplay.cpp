#include "Skybound.h"


bool sGameplay::Init()
{
    Skybound::getSingleton()->GetAssets()->addFont("UNICODE", "d:\\HelloCoder2025\\assets\\NotoSansJP-Regular.ttf", 18);
    Skybound::getSingleton()->GetAssets()->addPicture("MAIN", "c:\\Users\\user\\Desktop\\full3.png");
    return true;
}

bool sGameplay::Update(sTime curTime, sTime prevTime)
{
    return true;
}

void sGameplay::Render(sPicture* p_buffer)
{
    sFont *p_font = Skybound::getSingleton()->GetAssets()->getFont("UNICODE");
    sPicture *p_pic = Skybound::getSingleton()->GetAssets()->getPicture("MAIN");

    p_font->PrintText(*p_buffer, sPos2D(50, 50), 0xFF0000FF, "Hello from new gameplay!");
    p_buffer->DrawPicture(*p_pic, sPos2D(0, 0), sSize2D(32, 32), sPos2D(100, 100), sSize2D(32, 32));

}


