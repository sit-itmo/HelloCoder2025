#include "Skybound.h"

int current_frame = 0;

bool sGameplay::Init()
{
    Skybound::getSingleton()->GetAssets()->addFont("UNICODE", "d:\\HelloCoder2025\\assets\\NotoSansJP-Regular.ttf", 18);
    Skybound::getSingleton()->GetAssets()->addPicture("MAIN", "c:\\Users\\user\\Desktop\\full3.png");
    return true;
}

bool sGameplay::Update(sTime curTime, sTime prevTime)
{
    current_frame = ((int)curTime) % 5;
    return true;
}

void sGameplay::Render(sPicture* p_buffer)
{
    sFont *p_font = Skybound::getSingleton()->GetAssets()->getFont("UNICODE");
    sPicture *p_pic = Skybound::getSingleton()->GetAssets()->getPicture("MAIN");
    
    char buf[32] = { 0 };
    snprintf(buf, sizeof(buf), "[%d]", ((int)Skybound::getSingleton()->GetPlatform()->GetTime()) % 5);
    p_font->PrintText(*p_buffer, sPos2D(50, 50), sColor(255, 0, 0), buf);
    p_buffer->DrawPicture(*p_pic, sPos2D(0, current_frame * 32), sSize2D(32, 32), sPos2D(100, 100), sSize2D(32, 32));

}


