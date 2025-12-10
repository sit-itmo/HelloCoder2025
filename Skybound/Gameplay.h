#pragma once
class sGameplay;


struct SkyPictureWithCamera : public IPicture
{
    IPicture* pPic = nullptr;
    sVec2D Camera = { 0, 0 };

    virtual void Clear(sColor color);
    virtual void PutPixel(unsigned int x, unsigned int y, sColor color);
    virtual sColor GetPixel(unsigned int x, unsigned int y) const;
    virtual void DrawPicture(const IPicture* src,
        const sPos2D& srcPos, const sSize2D& srcSize,
        const sPos2D& dstPos, const sSize2D& dstSize);
    virtual void DrawRect(const sPos2D& pos, const sSize2D& size, sColor color);
    virtual sSize2D GetSize() const;
    virtual unsigned int Width() const;
    virtual unsigned int Height() const;
    virtual sColor* Data();
    virtual const sColor* Data() const;
};

class SkyEntity : public sRefControl
{
    friend class sGameplay;
protected:
    int     _Layer = 0;
    sVec2D  _Position;
    sSize2D _Size;
    bool    _Enabled;
    bool    _NeedToDelete = false;

    void DrawTileRect(IPicture* p_pic, sColor color);

public:
    inline int Layer() const { return _Layer; }
    inline bool NeedToDelete() const { return _NeedToDelete; }
    inline bool Enabled() const { return _Enabled; }
    inline const sVec2D& Position() const { return _Position; }
    inline const sSize2D& Size() const { return _Size; }

    SkyEntity();

    virtual ~SkyEntity();

    virtual void Kill();
    virtual void Update(sTime curTime, sTime dt) = 0;
    virtual void Draw(IPicture* p_buffer) = 0;

    bool Intersects(const sVec2D& otherPos, const sSize2D& otherSize) const;
    bool Intersects(const SkyEntity& other) const;

};

class SkyActiveEntity : public SkyEntity
{
protected:
    sVec2D  _Vector;
    void DoMove(float dt);

public:
    inline bool IsMoving() const { return _Vector.x != 0.0f && _Vector.y != 0.0f; }
    inline void Stop() { _Vector.x = 0.0f; _Vector.y = 0.0f; }
    inline const sVec2D& Vector() const { return _Vector; }

    SkyActiveEntity() : SkyEntity(), _Vector() {}

    virtual ~SkyActiveEntity() {}

};

class SkyParticle : public SkyActiveEntity
{
protected:
    float _Lifetime = 0.0f;
    float _MaxLifeTime = 1.0f;
    sColor _Color = { 255, 255, 255 };

public:

    static void SpawnParticles(const sVec2D& pos, int count, sColor baseColor,
        float life, float minSpeed, float maxSpeed);

    void Init(const sVec2D& pos, sVec2D vec, float life, sColor col);
    SkyParticle(const sVec2D& pos, sVec2D vec, float life, sColor col);
    void Update(sTime curTime, sTime dt);
    void Draw(IPicture* p_buffer);
};

class SkyBullet : public SkyActiveEntity
{
protected:
    void Fire(int dir);
    void SpawnHitParticles(const sVec2D& pos) const
    {
        SkyParticle::SpawnParticles(pos, 16, { 255, 180, 60 }, 0.4f, 40.0f, 160.0f);
    }

public:
    SkyBullet(const sVec2D& pos, int direction);
    void Update(sTime curTime, sTime dt);
    void Draw(IPicture* p_buffer);
};

class SkyCharacterEntity : public SkyActiveEntity
{
protected:
    int _Health = 0;
    int _MaxHealth = 0;
    bool _OnGround = false;
    float _MoveSpeed = 200.0f;
    float _JumpSpeed = -450.0f;
    int   _Direction = 1; // 1 right, -1 left
    float _ShootCooldown = 0.0f;


    sVec2D _SpawnPosition;
    int     _SpawnHealth;
    float   _SpawnSpeed;

    void ApplyGravity(float dt);
    void MoveAndCollide(float dt);


    void CheckHazards();

public:
    void Spawn(const sVec2D& pos, int health, float speed);

    inline int Health() const { return _Health; }
    inline int MaxHealth() const { return _MaxHealth; }
    inline bool OnGround() const { return _OnGround; }
    inline float MoveSpeed() const { return _MoveSpeed; }
    inline float JumpSpeed() const { return _JumpSpeed; }
    inline int   Direction() const { return _Direction; }
    inline bool IsDead() const { return _Health <= 0; }

    void TakeDamage(int dmg);

    SkyCharacterEntity() : SkyActiveEntity() {}

    virtual void Reset();
    virtual ~SkyCharacterEntity();

};

class SkyEnemie : public SkyCharacterEntity
{
private:
    int _Lives = 3;
public:
    void Kill();
    SkyEnemie();
    ~SkyEnemie();
    SkyEnemie(const sVec2D& pos, int health, float speed);
    void Update(sTime curTime, sTime dt);
    void Draw(IPicture* p_buffer);

};

class SkyPlayer : public SkyCharacterEntity
{
protected:
    void FireBullet();
    void SpawnMuzzleParticles(const sVec2D& pos) const
    {
        SkyParticle::SpawnParticles(pos, 8, { 255, 220, 100 }, 0.2f, 60.0f, 120.0f);
    }

public:
    SkyPlayer();
    SkyPlayer(int health);
    virtual void Reset();
    virtual void Update(sTime curTime, sTime delta) override;
    virtual void Draw(IPicture* p_buffer) override;
};

class SkyTile : public SkyEntity
{
    friend class SkyLevel;

private:
    void Setup(sPos2D pos, sSize2D size);

protected:
    SkyTile() : SkyEntity() {}

public:
    inline int Tx() const { return (int)_Position.x / _Size.W; }
    inline int Ty() const { return (int)_Position.y / _Size.H; }

    ~SkyTile();
    virtual void Update(sTime curTime, sTime delta);
    virtual bool IsSolid() const = 0;
    virtual bool IsHazard() const = 0;
};

class SkySpritedEntity : public SkyEntity
{
    bool _Deleting = false;
    SkyEntity* _pObj = nullptr;
    sSprite* _pSprite = nullptr;
public:
    inline bool Deleting() { return _Deleting; }
    inline SkyEntity* pObj() { return _pObj; }
    inline sSprite* pSprite() { return _pSprite; }

    SkySpritedEntity(SkyEntity* p_obj, sSprite* p_sprite);
    ~SkySpritedEntity();

    virtual void Update(sTime curTime, sTime delta);
    virtual void Draw(IPicture* p_buffer);
};


class SkyEmptyTile : public SkyTile
{
public:
    virtual void Update(sTime curTime, sTime delta) {}
    virtual void Draw(IPicture* p_buffer) {}
    virtual bool IsSolid() const { return false; }
    virtual bool IsHazard() const { return false; }
};

class SkyGroundTile : public SkyTile
{
public:
    virtual void Draw(IPicture* p_buffer);
    virtual bool IsSolid() const { return true; }
    virtual bool IsHazard() const { return false; }
};

class SkyDecorTile : public SkyTile
{
public:
    virtual void Draw(IPicture* p_buffer);
    virtual bool IsSolid() const { return false; }
    virtual bool IsHazard() const { return false; }
};

class SkySpikeTile : public SkyTile
{
public:
    virtual void Draw(IPicture* p_buffer);
    virtual bool IsSolid() const { return false; }
    virtual bool IsHazard() const { return true; }
};

class SkyLevel
{
protected:
    virtual void RegisterEntity(SkyEntity* p_e, int layer);

protected:
    SkyLevel() {}
    float _Gravity = 900.0f;
    sSize2D _SizeOfTile = { 32, 32 };
    const sSize2D _SizeInTiles;
    SkyTile** _pLevelMesh = nullptr;
    std::vector<SkyCharacterEntity*> _Enemies;
    SkyEmptyTile _EmptyTile;
    SkyPlayer* _pPlayer;

public:
    virtual ~SkyLevel();
    SkyLevel(const sSize2D& sizeInTiles);
    inline const SkyTile* pEmptyTile() const { return &_EmptyTile; }
    inline std::vector<SkyCharacterEntity*>& Enemies() { return _Enemies; }
    inline float Gravity() const { return _Gravity; }
    inline const SkyPlayer* cPlayer() const { return _pPlayer; }
    inline const sSize2D& SizeOfTile() const { return _SizeOfTile; }
    inline const sSize2D& SizeInTiles() const { return _SizeInTiles; }
    inline const int LevelH() const { return _SizeOfTile.H * _SizeInTiles.H; }
    inline const int LevelW() const { return _SizeOfTile.W * _SizeInTiles.W; }

    inline bool CheckOutOfScreen(const sVec2D& pos) const
    {
        return (pos.x < 0.0f || pos.y < 0.0 || pos.x >= LevelW() || pos.y >= LevelH());
    }

    inline sPos2D GetTilePos(const sVec2D& pos) const
    {
        return { (int)pos.x / (int)_SizeOfTile.W, (int)pos.y / (int)_SizeOfTile.H };
    }

    inline bool IsSolid(int tx, int ty) const { return cGetTile(tx, ty)->IsSolid(); }
    inline bool IsHazard(int tx, int ty) const { return cGetTile(tx, ty)->IsHazard(); }

    virtual bool AddTile(SkyTile* p_tile, int tx, int ty);
    virtual const SkyTile* cGetTile(int tx, int ty) const;
    virtual SkyTile* GetTile(int tx, int ty);
    virtual void DelTile(int tx, int ty);
    virtual bool AddEnemie(SkyCharacterEntity* p_enemie, const sVec2D& pos, int health, float speed);
    virtual void DelEnemie(SkyCharacterEntity* p_enemie);
    virtual bool SetPlayer(SkyPlayer* p_player, const sVec2D& pos, int health, float speed);

    inline void AddGroundTile(int tx, int ty) { AddTile(new SkyGroundTile(), tx, ty); }
    inline void AddSpikeTile(int tx, int ty) { AddTile(new SkySpikeTile(), tx, ty); }
    inline void AddDecorTile(int tx, int ty) { AddTile(new SkyDecorTile(), tx, ty); }
};

class SkySpritedLevel : public SkyLevel
{
protected:
    std::map<SkyTile*, SkySpritedEntity*> _DelMap;
    virtual void RegisterEntity(SkyEntity* p_e, int layer);
public:
    virtual ~SkySpritedLevel();

    SkySpritedLevel(const sSize2D& sizeInTiles) : SkyLevel(sizeInTiles) {}
    virtual void DelTile(int tx, int ty);
    virtual bool AddTile(SkyTile* p_tile, int tx, int ty);
    virtual bool AddEnemie(SkyCharacterEntity* p_enemie, const sVec2D& pos, int health, float speed);
    virtual bool SetPlayer(SkyPlayer* p_player, const sVec2D& pos, int health, float speed);
};

struct ISkyLevelBuilder
{
    virtual sSize2D GetSize() = 0;
    virtual bool SetupLevel(SkyLevel *p_level) = 0;
    virtual bool SetupEnemies(SkyLevel* p_level) = 0;
    virtual bool SetupPlayer(SkyLevel* p_level) = 0;
};

class SkyLevel_Level1 : public ISkyLevelBuilder
{
public:
    virtual sSize2D GetSize();
    virtual bool SetupLevel(SkyLevel* p_level);
    virtual bool SetupEnemies(SkyLevel* p_level);
    virtual bool SetupPlayer(SkyLevel* p_level);
};

class SkyLevel_Level2 : public ISkyLevelBuilder
{
public:
    virtual sSize2D GetSize();
    virtual bool SetupLevel(SkyLevel* p_level);
    virtual bool SetupEnemies(SkyLevel* p_level);
    virtual bool SetupPlayer(SkyLevel* p_level);
};

class SkyLevel_Level3 : public ISkyLevelBuilder
{
public:
    virtual sSize2D GetSize();
    virtual bool SetupLevel(SkyLevel* p_level);
    virtual bool SetupEnemies(SkyLevel* p_level);
    virtual bool SetupPlayer(SkyLevel* p_level);
};



