#pragma once
class sGameplay;
class SkyEntity : public sRefControl
{
    friend class sGameplay;
protected:
    int     _Layer = 0;
    sVec2D  _Position;
    sSize2D _Size;
    bool    _Enabled;
    bool    _NeedToDelete = false;

    void DrawTileRect(sPicture* p_pic, sColor color);

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
    virtual void Draw(sPicture* p_buffer) = 0;

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
    void Draw(sPicture* p_buffer);
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
    void Draw(sPicture* p_buffer);
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
    SkyEnemie(const sVec2D& pos, int health, float speed);
    void Update(sTime curTime, sTime dt);
    void Draw(sPicture* p_buffer);

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
    virtual void Draw(sPicture* p_buffer) override;
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

class SkyEmptyTile : public SkyTile
{
public:
    virtual void Update(sTime curTime, sTime delta) {}
    virtual void Draw(sPicture* p_buffer) {}
    virtual bool IsSolid() const { return false; }
    virtual bool IsHazard() const { return false; }
};

class SkyGroundTile : public SkyTile
{
public:
    virtual void Draw(sPicture* p_buffer);
    virtual bool IsSolid() const { return true; }
    virtual bool IsHazard() const { return false; }
};

class SkyDecorTile : public SkyTile
{
public:
    virtual void Draw(sPicture* p_buffer);
    virtual bool IsSolid() const { return false; }
    virtual bool IsHazard() const { return false; }
};

class SkySpikeTile : public SkyTile
{
public:
    virtual void Draw(sPicture* p_buffer);
    virtual bool IsSolid() const { return false; }
    virtual bool IsHazard() const { return true; }
};

class SkyLevel
{
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

    virtual bool SetupLevel() = 0;
    virtual bool SetupEnemies() = 0;
    virtual bool SetupPlayer() = 0;

    inline bool IsSolid(int tx, int ty) const { return cGetTile(tx, ty)->IsSolid(); }
    inline bool IsHazard(int tx, int ty) const { return cGetTile(tx, ty)->IsHazard(); }

    void AddTile(SkyTile* p_tile, int tx, int ty);
    const SkyTile* cGetTile(int tx, int ty) const;
    SkyTile* GetTile(int tx, int ty);
    void DelTile(int tx, int ty);
    void AddEnemie(SkyCharacterEntity* p_enemie, const sVec2D& pos, int health, float speed);
    void DelEnemie(SkyCharacterEntity* p_enemie);
    void SetPlayer(SkyPlayer* p_player, const sVec2D& pos, int health, float speed);

    inline void AddGroundTile(int tx, int ty) { AddTile(new SkyGroundTile(), tx, ty); }
    inline void AddSpikeTile(int tx, int ty) { AddTile(new SkySpikeTile(), tx, ty); }
    inline void AddDecorTile(int tx, int ty) { AddTile(new SkyDecorTile(), tx, ty); }
};

class SkyLevel_Level1 : public SkyLevel
{
public:
    SkyLevel_Level1();
    virtual bool SetupLevel();
    virtual bool SetupEnemies();
    virtual bool SetupPlayer();
};

class SkyLevel_Level2 : public SkyLevel
{
public:
    SkyLevel_Level2();
    virtual bool SetupLevel();
    virtual bool SetupEnemies();
    virtual bool SetupPlayer();
};

class SkyLevel_Level3 : public SkyLevel
{
public:
    SkyLevel_Level3();
    virtual bool SetupLevel();
    virtual bool SetupEnemies();
    virtual bool SetupPlayer();
};



