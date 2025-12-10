#include "Skybound.h"

int current_frame = 0;

bool sGameplay::Init()
{
    SKY_ASSETS().addFont("UNICODE", "d:\\HelloCoder2025\\assets\\NotoSansJP-Regular.ttf", 18);
    SKY_ASSETS().addPicture("MAIN", "c:\\Users\\user\\Desktop\\full3.png");
    
    ISkyLevelBuilder* p_builder = new SkyLevel_Level3();
    SkyLevel *p_level = new SkySpritedLevel(p_builder->GetSize());
    Skybound::getSingleton()->SetCurrentLevel(p_level);
    p_builder->SetupLevel(p_level);
    p_builder->SetupEnemies(p_level);
    p_builder->SetupPlayer(p_level);

    return true;
}

bool sGameplay::UpdateGui(sTime curTime, sTime delta)
{
    SKY_PROFSCOPE("sky_camera");
    //current_frame = ((int)curTime) % 5;

    if (_StatusFrameTime == 0)
    {
        _PrevSnapshot = SKY_PROFILER().GetAllSnapshots();
        _StatusFrameTime = curTime;
    }
    else if ((curTime - _StatusFrameTime) >= 1)
    {
        float fd = (curTime - _StatusFrameTime);
        auto snap = SKY_PROFILER().GetAllSnapshots();
        int64_t delta = (int64_t)snap["main_fps"].Count - (int64_t)_PrevSnapshot["main_fps"].Count;
        char buf[128] = { 0 };
        float tott = (float)(snap["main_fps"].TotalMs - (int64_t)_PrevSnapshot["main_fps"].TotalMs);
        float updt = (float)(snap["main_update"].TotalMs - (int64_t)_PrevSnapshot["main_update"].TotalMs);
        float drat = (float)(snap["main_draw"].TotalMs - (int64_t)_PrevSnapshot["main_draw"].TotalMs);

        snprintf(buf, sizeof(buf), "FPS: %f: upd=%f dra=%f", 
            (float)delta / fd, 100.0f * updt / tott, 100.0f * drat / tott);

        _StatusFrameLine = buf;
        _PrevSnapshot = snap;
        _StatusFrameTime = curTime;
    }
    return true;
}

bool sGameplay::Update(sTime curTime, sTime delta)
{
    {
        SKY_PROFSCOPE("sky_uadd");
        for (int i = 0; i < MAX_LAYERS; i++)
        {
            for (SkyEntity* p_obj : _ToAdd)
            {
                if (p_obj != nullptr)
                {
                    _Entities[abs(p_obj->Layer()) % MAX_LAYERS].push_back(p_obj);
                }
            }
            _ToAdd.clear();
        }
    }

    {
        SKY_PROFSCOPE("sky_update");
        for (int i = 0; i < MAX_LAYERS; i++)
        {
            _InsideLoop = true;
            for (SkyEntity* p_obj : _Entities[i])
            {
                SkyEntity* p_obj_saved = p_obj;
                if (p_obj != nullptr)
                {
                    p_obj->Update(curTime, delta);
                }
            }
            _InsideLoop = false;
        }
    }
#if 0
    for (int i = 0; i < MAX_LAYERS; i++)
    {
        for (auto it = _Entities[i].begin(); it != _Entities[i].end(); ++it)
        {
            SkyEntity* p_obj = *it;
            if (p_obj != nullptr)
            {
                if (p_obj->NeedToDelete())
                {
                    SKY_TRACE_EX("DEL: 0x%x", p_obj);
                    _ToDel.push_back(p_obj);
                }
            }
        }
    }
#endif
    {
        SKY_PROFSCOPE("sky_udel");
        for (SkyEntity* e : _ToDel)
        {
            delete e;
        }
        _ToDel.clear();
    }

    UpdateCamera();
    UpdateGui(curTime, delta);
    return true;
}

void sGameplay::UpdateCamera()
{
    SKY_PROFSCOPE("sky_camera");

    _Screen.Camera.x = SKY_LEVEL()->cPlayer()->Position().x + SKY_LEVEL()->cPlayer()->Size().W / 2 - SKY_PLATFORM().ScreenSize().W / 2;
    if (_Screen.Camera.x < 0) _Screen.Camera.x = 0;
    float maxCamX = (float)(SKY_LEVEL()->LevelW() - SKY_PLATFORM().ScreenSize().W);
    if (_Screen.Camera.x > maxCamX) _Screen.Camera.x = maxCamX;

}

void sGameplay::Render(sPicture* p_buffer)
{
    {
        SKY_PROFSCOPE("sky_draw");
        _Screen.pPic = p_buffer;
        _InsideLoop = true;
        for (int i = 0; i < MAX_LAYERS; i++)
        {
            for (SkyEntity* p_obj : _Entities[i])
            {
                if (p_obj != nullptr)
                {
                    p_obj->Draw(&_Screen);
                }
            }
        }
        _InsideLoop = false;
    }

    RenderGui(p_buffer);
}

void sGameplay::RenderGui(sPicture* p_buffer)
{
    SKY_PROFSCOPE("sky_gui");

    sFont* p_font = SKY_ASSETS().getFont("UNICODE");

    char buf[128] = { 0 };
    snprintf(buf, sizeof(buf), "Ent[%d:%d:%d:%d]", 
        _Entities[0].size(), _Entities[1].size(), 
        _Entities[2].size(), _Entities[3].size());
    _StatusStatLine = buf;

    std::string text = _StatusFrameLine + "\n" + _StatusStatLine;
    p_font->PrintText(*p_buffer, sPos2D(10, 20), sColor(255, 0, 0), text.c_str());
    //IPicture* p_pic = SKY_ASSETS().getPicture("MAIN");
    //p_buffer->DrawPicture(p_pic, sPos2D(current_frame * 32, 0), sSize2D(32, 32), sPos2D(100, 100), sSize2D(32, 32));
}

sGameplay::~sGameplay()
{
    for (int i = 0; i < MAX_LAYERS; i++)
    {
        while (_Entities->empty() == false)
        {
            SkyEntity* p_obj = _Entities[i][0];
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
    if (p_obj == nullptr)
    {
        return;
    }
    p_obj->RefsAdd();
    p_obj->_Layer = layer;
    if (_InsideLoop == true)
    {
        _ToAdd.push_back(p_obj);
    }
    else
    {
        _Entities[abs(p_obj->Layer()) % MAX_LAYERS].push_back(p_obj);
    }
}

void sGameplay::DelEntity(SkyEntity* p_obj, int layer)
{
    if (p_obj == nullptr)
    {
        return;
    }
    if (_InsideLoop == true)
    {
        _ToDel.push_back(p_obj);
    }
    else
    {

        auto do_del = [](std::vector<SkyEntity*>& entities, SkyEntity* p_obj)
            {
                entities.erase(
                    std::remove_if(entities.begin(), entities.end(),
                        [p_obj](SkyEntity* e) {
                            if (e == p_obj)
                            {
                                p_obj->RefsDel();
                                return true;
                            }
                            return false;
                        }),
                    entities.end()
                );
            };
        if (layer < 0)
        {
            layer = p_obj->Layer();
        }

        do_del(_ToAdd, p_obj);
        if (layer >= 0)
        {
            do_del(_Entities[abs(layer) % MAX_LAYERS], p_obj);
        }
        else
        {
            for (int i = 0; i < MAX_LAYERS; i++)
            {
                do_del(_Entities[i], p_obj);
            }
        }
    }
}

SkyEntity::SkyEntity() : _Position(), _Size(), _Enabled(true)
{
    SKY_TRACE_EX("ADD: 0x%x", this);
}

SkyEntity::~SkyEntity()
{
    SKY_GAMEPLAY().DelEntity(this);
}

void SkyEntity::Kill() 
{ 
    _Enabled = false; 
    _NeedToDelete = true;
    SKY_GAMEPLAY().DelEntity(this);
};

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

SkyCharacterEntity::~SkyCharacterEntity()
{
}

SkyEnemie::~SkyEnemie()
{
    SKY_LEVEL()->DelEnemie(this);
}

void SkyCharacterEntity::Reset()
{
    Stop();
    _OnGround = false;
    _Direction = 1;
    _Enabled = true;
    _Health = _MaxHealth;
    _ShootCooldown = 0.0f;
    _Size = { 32, 64 };
}


void SkyCharacterEntity::Spawn(const sVec2D& pos, int health, float speed)
{
    _SpawnHealth = _Health = _MaxHealth = (health > 0 ? health : 1);
    _SpawnPosition = _Position = pos;
    _SpawnSpeed = _MoveSpeed = speed;
    _Vector.x = (_Direction >= 0) ? _MoveSpeed : -_MoveSpeed;
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
            if (SKY_LEVEL()->IsHazard(txRight, ty))
            {
                TakeDamage(1);
                return;
            }
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
            if (SKY_LEVEL()->IsHazard(txLeft, ty))
            {
                TakeDamage(1);
                return;
            }
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
            if (SKY_LEVEL()->IsHazard(tx, tyBottom))
            {
                TakeDamage(1);
                return;
            }
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
            if (SKY_LEVEL()->IsHazard(tx, tyTop))
            {
                TakeDamage(1);
                return;
            }
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
    if (_Position.y + _Size.H >= SKY_LEVEL()->LevelH())
    {
        _Position.y = (float)(SKY_LEVEL()->LevelH() - _Size.H);
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
                TakeDamage(1);
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

void SkyEntity::DrawTileRect(IPicture* p_pic, sColor color)
{
    if (p_pic)
    {
        p_pic->DrawRect((sPos2D)_Position, _Size, color);
    }
}

SkyTile::~SkyTile()
{
    const SkyTile* p_cur = SKY_LEVEL()->GetTile(this->Tx(), this->Ty());
    if (SKY_LEVEL()->pEmptyTile() != p_cur)
    {
        SKY_ASSERT(p_cur == this);
        SKY_LEVEL()->DelTile(this->Tx(), this->Ty());
    }
}

void SkyDecorTile::Draw(IPicture* p_buffer)
{
    DrawTileRect(p_buffer, { 50, 200, 50 });
}

void SkyGroundTile::Draw(IPicture* p_buffer)
{
    DrawTileRect(p_buffer, { 100, 100, 255 });
}

void SkySpikeTile::Draw(IPicture* p_buffer)
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
    if (_pPlayer != nullptr)
    {
        _pPlayer->RefsDel();
        delete _pPlayer;
    }
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

void SkyLevel::RegisterEntity(SkyEntity* p_e, int layer)
{
    if (p_e)
    {
        SKY_GAMEPLAY().AddEntity(p_e, layer);
    }
}

bool SkyLevel::AddTile(SkyTile* p_tile, int tx, int ty)
{
    if (_pLevelMesh != nullptr && tx >= 0 && ty >= 0 
        && tx < (int)_SizeInTiles.W && ty < (int)_SizeInTiles.H)
    {
        p_tile->Setup({ tx * (int)_SizeOfTile.W, ty * (int)_SizeOfTile.H }, _SizeOfTile);
        SkyTile* p_old_tile = _pLevelMesh[ty * _SizeInTiles.W + tx];
        if (p_old_tile != nullptr)
        {
            DelTile(tx, ty);
        }
        p_tile->RefsAdd();
        _pLevelMesh[ty * _SizeInTiles.W + tx] = p_tile;
        RegisterEntity(p_tile, 0);
        return true;
    }
    return false;
}

SkyTile* SkyLevel::GetTile(int tx, int ty)
{
    SkyTile* p_res = nullptr;
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

const SkyTile* SkyLevel::cGetTile(int tx, int ty) const
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

void SkyLevel::DelTile(int tx, int ty)
{
    SkyTile* p_res = nullptr;
    if (_pLevelMesh != nullptr && tx >= 0 && ty >= 0
        && tx < (int)_SizeInTiles.W && ty < (int)_SizeInTiles.H)
    {
        p_res = _pLevelMesh[ty * _SizeInTiles.W + tx];
        _pLevelMesh[ty * _SizeInTiles.W + tx] = nullptr;
        p_res->RefsDel();
        SKY_GAMEPLAY().DelEntity(p_res);
    }
}

bool SkyLevel::AddEnemie(SkyCharacterEntity* p_enemie, const sVec2D& pos, int health, float speed)
{
    if (p_enemie == nullptr)
    {
        return false;
    }
    p_enemie->Reset();
    p_enemie->Spawn(pos, health, speed);
    p_enemie->RefsAdd();
    _Enemies.push_back(p_enemie);
    RegisterEntity(p_enemie, 1);
    return true;
}

bool SkyLevel::SetPlayer(SkyPlayer* p_player, const sVec2D& pos, int health, float speed)
{
    if (p_player == nullptr)
    {
        return false;
    }
    p_player->Reset();
    p_player->Spawn(pos, health, speed);
    if (_pPlayer != nullptr)
    {
        _pPlayer->RefsDel();
        delete _pPlayer;
    }
    _pPlayer = p_player;
    RegisterEntity(p_player, 1);
    return true;
}

void SkyLevel::DelEnemie(SkyCharacterEntity* p_enemie)
{
    p_enemie->RefsDel();
    auto new_end = std::remove(_Enemies.begin(), _Enemies.end(), p_enemie);
    _Enemies.erase(new_end, _Enemies.end());
}

void SkyCharacterEntity::TakeDamage(int dmg)
{
    _Health -= dmg;
    if (_Health <= 0)
    {
        _Health = 0;
        Kill();
    }
}

SkyPlayer::SkyPlayer()
{
    SKY_TRACE_EX("PLAYER: 0x%x", this);
}

SkyPlayer::SkyPlayer(int health) 
{
    _Size = {32, 64};
    _MaxHealth = _Health = health; 
}

void SkyPlayer::Reset()
{
    _Position = { 50.0f, 100.0f };
    SkyCharacterEntity::Reset();
}

void SkyPlayer::FireBullet()
{
    sVec2D pos = _Position + _Size * 0.5f;
    SKY_GAMEPLAY().AddEntity(new SkyBullet(pos, _Direction), 2);
    SpawnMuzzleParticles(pos);
}

void SkyPlayer::Update(sTime curTime, sTime delta)
{
    // --- movement input ---
    _Vector.x = 0.0f;

    if (SKY_PLATFORM().GetKeyState(sPlatform::KeyCode_Left))
    {
        _Vector.x = -_MoveSpeed;
        _Direction = -1;
    }
    if (SKY_PLATFORM().GetKeyState(sPlatform::KeyCode_Right))
    {
        _Vector.x = _MoveSpeed;
        _Direction = 1;
    }

    // --- jump ---
    if (SKY_PLATFORM().GetKeyState(sPlatform::KeyCode_Space) && _OnGround)
    {
        _Vector.y = _JumpSpeed;
        _OnGround = false;
    }

    // --- shoot (Z) ---
    _ShootCooldown -= delta;
    if (_ShootCooldown < 0.0f)
        _ShootCooldown = 0.0f;

    if (SKY_PLATFORM().GetKeyState(sPlatform::KeyCode_Fire) && _ShootCooldown <= 0.0f)
    {
        FireBullet();
        _ShootCooldown = 0.25f; // 4 shots per second
    }

    // physics
    ApplyGravity(delta);
    MoveAndCollide(delta);
    CheckHazards();

    for (auto* e : SKY_LEVEL()->Enemies())
    {
        if (e != nullptr && e->Enabled() && Enabled())
        {
            if (this->Intersects(*e))
            {
                TakeDamage(1);
                Reset(); 
            }
            break;
        }
    }
}

void SkyPlayer::Draw(IPicture* p_buffer)
{
    DrawTileRect(p_buffer, {255, 200, 50});
}

void SkyBullet::Fire(int dir)
{
    _Vector = { (dir >= 0) ? 400.0f : -400.0f, 0.0f };
    _Enabled = true;
}

SkyBullet::SkyBullet(const sVec2D& pos, int direction)
{
    SKY_TRACE_EX("BULLET: 0x%x", this);
    _Size = {8, 4};
    _Position = pos;
    Fire(direction);
}

void SkyBullet::Update(sTime curTime, sTime dt)
{
    _Position += _Vector * dt;

    // out of level bounds
    if (SKY_LEVEL()->CheckOutOfScreen(_Position))
    {
        Kill();
        return;
    }

    // tile collision
    sPos2D t = SKY_LEVEL()->GetTilePos(_Position);
    if (SKY_LEVEL()->IsSolid(t.X, t.Y))
    {
        SpawnHitParticles({ t.X * SKY_LEVEL()->SizeOfTile().W, t.Y * SKY_LEVEL()->SizeOfTile().H });
        Kill();
        return;
    }

    if (SKY_LEVEL())
    {
        // enemy collision
        for (auto* e : SKY_LEVEL()->Enemies())
        {
            if (e == nullptr) continue;
            if (e->IsDead()) continue;

            if (e->Intersects(*this))
            {
                this->Kill();
                e->TakeDamage(1);
                SpawnHitParticles((e->Position() + e->Size() * 0.5f));
                break;
            }
        }
    }
}

void SkyBullet::Draw(IPicture* p_buffer)
{
   DrawTileRect(p_buffer, {255, 255, 255});
}

SkyParticle::SkyParticle(const sVec2D& pos, sVec2D vec, float life, sColor col)
{
    SKY_TRACE_EX("PARTICLE: 0x%x", this);
    Init(pos, vec, life, col);
}

void SkyParticle::Init(const sVec2D& pos, sVec2D vec, float life, sColor col)
{
    _Size = {4, 4};
    _Position = pos;
    _Vector = vec;
    _Lifetime = 0.0f;
    _MaxLifeTime = life;
    _Color = col;
    _Enabled = true;
}

void SkyParticle::Update(sTime curTime, sTime dt)
{
    if (!_Enabled) return;

    _Lifetime += dt;
    if (_Lifetime >= _MaxLifeTime)
    {
        Kill();
        return;
    }

    _Position += _Vector * dt;
}

void SkyParticle::Draw(IPicture* p_buffer)
{
    if (!_Enabled) return;
    DrawTileRect(p_buffer, _Color);
}

void SkyParticle::SpawnParticles(const sVec2D& pos, int count, sColor baseColor,
    float life, float minSpeed, float maxSpeed)
{
    for (int n = 0; n < count; ++n)
    {
        float angle = (float)(rand() % 360) * 3.14159265f / 180.0f;
        float t = (rand() / (float)RAND_MAX); // 0..1
        float speed = minSpeed + (maxSpeed - minSpeed) * t;

        float pvx = cosf(angle) * speed;
        float pvy = sinf(angle) * speed - 40.0f; // a bit upwards

        // you can slightly randomize color if you want

        SkyParticle* p = new SkyParticle({ pos.x, pos.y }, { pvx, pvy }, life, baseColor);
        SKY_GAMEPLAY().AddEntity(p, 2);
    }
}

SkyEnemie::SkyEnemie()
{
    SKY_TRACE_EX("ENEMIE: 0x%x", this);
}

SkyEnemie::SkyEnemie(const sVec2D& pos, int health, float speed)
{
    Spawn(pos, health, speed);
}

void SkyEnemie::Kill()
{
    _Lives--;
    if (_Lives <= 0)
    {
        SkyCharacterEntity::Kill();
    }
    else
    {
        int direction = _Direction;
        Spawn(_SpawnPosition, _SpawnHealth, _SpawnSpeed);
        _Direction = -direction;
    }
}

void SkyEnemie::Update(sTime curTime, sTime dt)
{
    ApplyGravity(dt);
    MoveAndCollide(dt);

    // If hit wall horizontally, MoveAndCollide will zero vx,
    // so we flip direction there
    if (_Vector.x == 0.0f)
    {
        _Direction = -_Direction;
        _Vector.x = (_Direction >= 0) ? _MoveSpeed : -_MoveSpeed;
    }
}

void SkyEnemie::Draw(IPicture* p_buffer)
{
    if (!_Enabled) return;
    DrawTileRect(p_buffer, {200, 50, 255});
}

///////////////////////////////////////////////////////////////////
void SkyPictureWithCamera::Clear(sColor color)
{
    if (pPic) { pPic->Clear(color); }
}
void SkyPictureWithCamera::PutPixel(unsigned int x, unsigned int y, sColor color)
{
    if (pPic) { pPic->PutPixel((unsigned int)(x - Camera.x), (unsigned int)(y - Camera.y), color); }
}
sColor SkyPictureWithCamera::GetPixel(unsigned int x, unsigned int y) const
{
    if (pPic) { return pPic->GetPixel((unsigned int)(x - Camera.x), (unsigned int)(y - Camera.y)); } return { 0, 0, 0 };
}
void SkyPictureWithCamera::DrawPicture(const IPicture* src,
    const sPos2D& srcPos, const sSize2D& srcSize,
    const sPos2D& dstPos, const sSize2D& dstSize)
{
    if (pPic) { pPic->DrawPicture(src, srcPos, srcSize, (sPos2D)(dstPos - Camera), dstSize); }
}
void SkyPictureWithCamera::DrawRect(const sPos2D& pos, const sSize2D& size, sColor color)
{
    if (pPic) { pPic->DrawRect((sPos2D)(pos - Camera), size, color); }
}
sSize2D SkyPictureWithCamera::GetSize() const
{
    if (pPic) { return pPic->GetSize(); } return { 0, 0 };
}
unsigned int SkyPictureWithCamera::Width() const
{
    if (pPic) { return pPic->Width(); } return 0;
}
unsigned int SkyPictureWithCamera::Height() const
{
    if (pPic) { return pPic->Height(); } return 0;
}
sColor* SkyPictureWithCamera::Data()
{
    if (pPic) { return pPic->Data(); } return nullptr;
}
const sColor* SkyPictureWithCamera::Data() const
{
    if (pPic) { return pPic->Data(); } return nullptr;
}

SkySpritedEntity::SkySpritedEntity(SkyEntity* p_obj, sSprite* p_sprite) : _pObj(p_obj), _pSprite(p_sprite) 
{
    _pObj->RefsAdd();
}
SkySpritedEntity::~SkySpritedEntity()
{
    _Deleting = true;
    if (_pObj)
    {
        SkyEntity* p_ent = _pObj;
        _pObj = nullptr;
        p_ent->RefsDel();
        delete p_ent;
    }
    if (_pSprite)
    {
        delete _pSprite;
        _pSprite = nullptr;
    }
}

void SkySpritedEntity::Update(sTime curTime, sTime delta)
{
    _pObj->Update(curTime, delta);
    _pSprite->Update(curTime, delta);
}

void SkySpritedEntity::Draw(IPicture* p_buffer)
{
    _pSprite->Draw(p_buffer, (sPos2D)_pObj->Position(), _pObj->Size());
}


void SkySpritedLevel::RegisterEntity(SkyEntity* p_e, int layer)
{
    //nothing
}
SkySpritedLevel::~SkySpritedLevel()
{
}

void SkySpritedLevel::DelTile(int tx, int ty)
{
    SkyTile* p_tile = GetTile(tx, ty);
    SkySpritedEntity* p_ent = _DelMap[p_tile];
    p_ent->RefsDel();
    p_tile->RefsDel();
    SkyLevel::DelTile(tx, ty);
    if (p_ent->Deleting() == false)
    {
        delete p_ent;
    }
}

bool SkySpritedLevel::AddTile(SkyTile* p_tile, int tx, int ty)
{
    sSprite* p_sprite = new sSprite();
    sPos2D src{ 0, 0 };
    bool anim = false;
    if (p_tile->IsHazard())
    {
        src = { 1 * 32, 0 };
    }
    else
    {
        if (p_tile->IsSolid())
        {
            src = { 3 * 32, 0 };
        }
        else
        {
           anim = true;
           src = { 0 * 32, 0 };
        }
    }
    p_sprite->Init(SKY_ASSETS().getPicture("MAIN"), src, { 32, 32 },
        anim ? sSprite::Vertical : sSprite::None, 0.5, 4);
    if (anim)
    {
        p_sprite->AnimationStart(SKY_PLATFORM().GetTime());
    }
    SkySpritedEntity* p_sce = new SkySpritedEntity(p_tile, p_sprite);
    if (SkyLevel::AddTile(p_tile, tx, ty))
    {
        p_sce->RefsAdd();
        p_tile->RefsAdd();
        _DelMap[p_tile] = p_sce;
        SkyLevel::RegisterEntity(p_sce, 0);
        return true;
    }
    return false;
}

bool SkySpritedLevel::AddEnemie(SkyCharacterEntity* p_enemie, const sVec2D& pos, int health, float speed)
{
    if (SkyLevel::AddEnemie(p_enemie, pos, health, speed))
    {
        SkyLevel::RegisterEntity(p_enemie, 1);
        return true;
    }
    return false;
}

bool SkySpritedLevel::SetPlayer(SkyPlayer* p_player, const sVec2D& pos, int health, float speed)
{
    if (SkyLevel::SetPlayer(p_player, pos, health, speed))
    {
        SkyLevel::RegisterEntity(p_player, 1);
        return true;
    }
    return false;
}
