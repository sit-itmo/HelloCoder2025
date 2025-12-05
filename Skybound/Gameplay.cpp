#include "Skybound.h"

int current_frame = 0;

bool sGameplay::Init()
{
    SKY_ASSETS()->addFont("UNICODE", "d:\\HelloCoder2025\\assets\\NotoSansJP-Regular.ttf", 18);
    SKY_ASSETS()->addPicture("MAIN", "c:\\Users\\user\\Desktop\\full3.png");
    
    SkyLevel *p_level = new SkyLevel_Level1();
    p_level->SetupLevel();
    Skybound::getSingleton()->SetCurrentLevel(p_level);
    
    return true;
}

bool sGameplay::Update(sTime curTime, sTime delta)
{
    current_frame = ((int)curTime) % 5;
    
    for (int i = 0; i < MAX_LAYERS; i++)
    {
        for (SkyEntity* p_obj : _Entities[i])
        {
            if (p_obj != nullptr)
            {
                p_obj->Update(curTime, delta);
            }
        }
    }
    
    return true;
}

void sGameplay::Render(sPicture* p_buffer)
{
    for (int i = 0; i < MAX_LAYERS; i++)
    {
        for (SkyEntity* p_obj : _Entities[i])
        {
            if (p_obj != nullptr)
            {
                p_obj->Draw(p_buffer);
            }
        }
    }


    sFont *p_font = Skybound::getSingleton()->GetAssets()->getFont("UNICODE");
    sPicture *p_pic = Skybound::getSingleton()->GetAssets()->getPicture("MAIN");
    
    char buf[32] = { 0 };
    snprintf(buf, sizeof(buf), "[%d]", ((int)Skybound::getSingleton()->GetPlatform()->GetTime()) % 5);
    p_font->PrintText(*p_buffer, sPos2D(50, 50), sColor(255, 0, 0), buf);
    p_buffer->DrawPicture(*p_pic, sPos2D(current_frame * 32, 0), sSize2D(32, 32), sPos2D(100, 100), sSize2D(32, 32));

}

sGameplay::~sGameplay()
{
    for (int i = 0; i < MAX_LAYERS; i++)
    {
        for (SkyEntity* p_obj : _Entities[i])
        {
            if (p_obj != nullptr)
            {
                delete p_obj;
            }
            p_obj = nullptr;
        }
        _Entities[i].clear();
    }
}

void sGameplay::AddEntity(SkyEntity* p_obj, int layer)
{
    _Entities[abs(layer) % MAX_LAYERS].push_back(p_obj);
}

bool SkyEntity::Intersects(const SkyEntity& other) const
{
    if (other._Enabled == false) return false;
    return Intersects(other._Position, other._Size);
}

bool SkyEntity::Intersects(const sVec2D& otherPos, const sSize2D& otherSize) const
{
    if (_Enabled == false) return false;
    return !(_Position.x > otherPos.x + (float)otherSize.W ||
        otherPos.x > _Position.x + (float)_Size.W ||
        _Position.y > otherPos.y + (float)otherSize.H ||
        otherPos.y > _Position.y + (float)_Size.H);
}

void SkyActiveEntity::DoMove(float dt)
{
    if (!_Enabled) return;

    _Position += _Vector * dt;
}

void SkyCharacterEntity::ApplyGravity(float dt)
{
    _Vector.y += SKY_LEVEL()->Gravity() * dt;
}

void SkyCharacterEntity::Reset()
{
    Stop();
    _OnGround = false;
    _Direction = 1;
    _Enabled = true;
    _Health = _MaxHealth;
    _ShootCooldown = 0.0f;
}


void SkyCharacterEntity::MoveAndCollide(float dt)
{
    // horizontal
    sVec2D new_pos = _Position + _Vector * dt;

    if (_Vector.x > 0.0f) // right
    {
        int txRight = (int)((new_pos.x + _Size.W - 1) / SKY_LEVEL()->SizeOfTile().W);
        int tyTop = (int)(_Position.y / SKY_LEVEL()->SizeOfTile().H);
        int tyBottom = (int)((_Position.y + _Size.H - 1) / SKY_LEVEL()->SizeOfTile().H);

        bool collide = false;
        for (int ty = tyTop; ty <= tyBottom; ++ty)
        {
            if (SKY_LEVEL()->IsSolid(txRight, ty))
            {
                collide = true;
                break;
            }
        }

        if (collide)
        {
            new_pos.x = txRight * SKY_LEVEL()->SizeOfTile().W - (float)_Size.W;
            _Vector.x = 0.0f;
        }
    }
    else if (_Vector.x < 0.0f) // left
    {
        int txLeft = (int)(new_pos.x / SKY_LEVEL()->SizeOfTile().W);
        int tyTop = (int)(_Position.y / SKY_LEVEL()->SizeOfTile().H);
        int tyBottom = (int)((_Position.y + _Size.H - 1) / SKY_LEVEL()->SizeOfTile().H);

        bool collide = false;
        for (int ty = tyTop; ty <= tyBottom; ++ty)
        {
            if (SKY_LEVEL()->IsSolid(txLeft, ty))
            {
                collide = true;
                break;
            }
        }

        if (collide)
        {
            new_pos.x = ((float)txLeft + 1) * SKY_LEVEL()->SizeOfTile().W;
            _Vector.x = 0.0f;
        }
    }

    _Position.x = new_pos.x;
    _OnGround = false;

    if (_Vector.y > 0.0f) // falling
    {
        int tyBottom = (int)((new_pos.y + _Size.H - 1) / SKY_LEVEL()->SizeOfTile().H);
        int txLeft = (int)(_Position.x / SKY_LEVEL()->SizeOfTile().W);
        int txRight = (int)((_Position.x + _Size.W - 1) / SKY_LEVEL()->SizeOfTile().W);

        bool collide = false;
        for (int tx = txLeft; tx <= txRight; ++tx)
        {
            if (SKY_LEVEL()->IsSolid(tx, tyBottom))
            {
                collide = true;
                break;
            }
        }

        if (collide)
        {
            new_pos.y = tyBottom * SKY_LEVEL()->SizeOfTile().H - (float)_Size.H;
            _Vector.y = 0.0f;
            _OnGround = true;
        }
    }
    else if (_Vector.y < 0.0f) // jumping up
    {
        int tyTop = (int)(new_pos.y / SKY_LEVEL()->SizeOfTile().H);
        int txLeft = (int)(_Position.x / SKY_LEVEL()->SizeOfTile().W);
        int txRight = (int)((_Position.x + _Size.W - 1) / SKY_LEVEL()->SizeOfTile().W);

        bool collide = false;
        for (int tx = txLeft; tx <= txRight; ++tx)
        {
            if (SKY_LEVEL()->IsSolid(tx, tyTop))
            {
                collide = true;
                break;
            }
        }

        if (collide)
        {
            new_pos.y = ((float)tyTop + 1) * SKY_LEVEL()->SizeOfTile().H;
            _Vector.y = 0.0f;
        }
    }

    _Position.y = new_pos.y;

    // basic screen bottom clamp (optionally keep)
    if (_Position.y + _Size.H >= SKY_LEVEL()->ScreenH())
    {
        _Position.y = (float)(SKY_LEVEL()->ScreenH() - _Size.H);
        _Vector.y = 0.0f;
        _OnGround = true;
    }
}

void SkyCharacterEntity::CheckHazards()
{
    int left = (int)_Position.x;
    int right = (int)_Position.x + _Size.W - 1;
    int top = (int)_Position.y;
    int bottom = (int)_Position.y + _Size.H - 1;

    int tx0 = left / SKY_LEVEL()->SizeOfTile().W;
    int tx1 = right / SKY_LEVEL()->SizeOfTile().W;
    int ty0 = top / SKY_LEVEL()->SizeOfTile().H;
    int ty1 = bottom / SKY_LEVEL()->SizeOfTile().H;

    for (int ty = ty0; ty <= ty1; ++ty)
    {
        for (int tx = tx0; tx <= tx1; ++tx)
        {
            if (SKY_LEVEL()->IsHazard(tx, ty))
            {
                Reset();
                return;
            }
        }
    }
}

void SkyTile::Setup(sPos2D pos, sSize2D size)
{
    _Position = (sVec2D)pos;
    _Size = size;
}

void SkyTile::Update(sTime curTime, sTime prevTime)
{

}


void SkyTile::DrawTileRect(sPicture* p_pic, sColor color)
{
    for (int y = (int)_Position.y; y < (int)_Position.y + _Size.H; ++y)
    {
        for (int x = (int)_Position.x; x < (int)_Position.x + _Size.W; ++x)
        {
            p_pic->PutPixel(x, y, color);
        }
    }
}

void SkyDecorTile::Draw(sPicture* p_buffer)
{
    DrawTileRect(p_buffer, { 50, 200, 50 });
}

void SkyGroundTile::Draw(sPicture* p_buffer)
{
    DrawTileRect(p_buffer, { 100, 100, 255 });
}

void SkySpikeTile::Draw(sPicture* p_buffer)
{
    DrawTileRect(p_buffer, { 255, 50, 50 });

}

SkyLevel::SkyLevel(const sSize2D& sizeInTiles) : _SizeInTiles(sizeInTiles)
{
    _pLevelMesh = new SkyTile*[sizeInTiles.W * sizeInTiles.H];
    if (_pLevelMesh != nullptr)
    {
        memset(_pLevelMesh, 0, sizeof(SkyTile*) * sizeInTiles.W * sizeInTiles.H);
    }
}

SkyLevel::~SkyLevel()
{
    if (_pLevelMesh != nullptr)
    {
        delete[] _pLevelMesh;
        _pLevelMesh = nullptr;
    }
}




//void* GetPointer()
//{
//    return 0x1121232;
//}
//void test()
//{
//    char buf[128];
//
//    memcpy(buf, GetPointer, 128);
//}

void SkyLevel::AddTile(SkyTile* p_tile, int tx, int ty)
{
    if (_pLevelMesh != nullptr && tx >= 0 && ty >= 0 
        && tx < (int)_SizeInTiles.W && ty < (int)_SizeInTiles.H)
    {
        p_tile->Setup({ tx * (int)_SizeOfTile.W, ty * (int)_SizeOfTile.H }, _SizeOfTile);
        _pLevelMesh[ty * _SizeInTiles.W + tx] = p_tile;
        SKY_GAMEPLAY()->AddEntity(p_tile, 0);
    }
    return;
}

const SkyTile* SkyLevel::GetTile(int tx, int ty) const
{
    const SkyTile* p_res = nullptr;
    if (_pLevelMesh != nullptr && tx >= 0 && ty >= 0 
        && tx < (int)_SizeInTiles.W && ty < (int)_SizeInTiles.H)
    {
        p_res = _pLevelMesh[ty * _SizeInTiles.W + tx];
    }
    if (p_res == nullptr)
    {
        p_res = &_EmptyTile;
    }
    return p_res;
}
