#pragma once

class SkyEntity : public sRefControl
{
protected:
    sVec2D  _Position;
    sSize2D _Size;
    bool    _Enabled;
    bool    _NeedToDelete = false;

    void DrawTileRect(sPicture* p_pic, sColor color);

public:
    inline bool NeedToDelete() const { return _NeedToDelete; }
    inline bool Enabled() const { return _Enabled; }
    inline const sVec2D& Position() const { return _Position; }
    inline const sSize2D& Size() const { return _Size; }

    SkyEntity() : _Position(), _Size(), _Enabled(true) {}

    virtual ~SkyEntity() {}

    virtual void Kill() { _Enabled = false; _NeedToDelete = true; };
    virtual void Update(sTime curTime, sTime prevTime) = 0;
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

class SkyBullet : public SkyActiveEntity
{
protected:
    void Fire(int dir);
public:
    SkyBullet(const sVec2D& pos, int direction);
    void Update(sTime curTime, sTime prevTime);
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

    void ApplyGravity(float dt);
    void MoveAndCollide(float dt);


    void CheckHazards();

public:

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
    virtual ~SkyCharacterEntity() {}

};

class SkyTile : public SkyEntity
{
    friend class SkyLevel;

private:
    void Setup(sPos2D pos, sSize2D size);

protected:
    SkyTile() : SkyEntity() {}

public:
    ~SkyTile() {};
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

    SkyEmptyTile _EmptyTile;

public:
    virtual ~SkyLevel();

    SkyLevel(const sSize2D& sizeInTiles);
    inline float Gravity() const { return _Gravity; }
    inline const sSize2D& SizeOfTile() const { return _SizeOfTile; }
    inline const sSize2D& SizeInTiles() const { return _SizeInTiles; }
    inline const int ScreenH() const { return _SizeOfTile.H * _SizeInTiles.H; }
    inline const int ScreenW() const { return _SizeOfTile.W * _SizeInTiles.W; }

    inline bool CheckOutOfScreen(const sVec2D& pos) const
    {
        return (pos.x < 0.0f || pos.y < 0.0 || pos.x >= ScreenW() || pos.y >= ScreenH());
    }

    inline sPos2D GetTilePos(const sVec2D& pos) const
    {
        return { (int)pos.x / (int)_SizeOfTile.W, (int)pos.y / (int)_SizeOfTile.H };
    }

    virtual bool SetupLevel() = 0;

    inline bool IsSolid(int tx, int ty) const { return GetTile(tx, ty)->IsSolid(); }
    inline bool IsHazard(int tx, int ty) const { return GetTile(tx, ty)->IsHazard(); }

    void AddTile(SkyTile* p_tile, int tx, int ty);
    const SkyTile* GetTile(int tx, int ty) const;

    inline void AddGroundTile(int tx, int ty) { AddTile(new SkyGroundTile(), tx, ty); }
    inline void AddSpikeTile(int tx, int ty) { AddTile(new SkySpikeTile(), tx, ty); }
    inline void AddDecorTile(int tx, int ty) { AddTile(new SkyDecorTile(), tx, ty); }
};

class SkyLevel_Level1 : public SkyLevel
{
public:
    SkyLevel_Level1();
    virtual bool SetupLevel();
};

class SkyLevel_Level2 : public SkyLevel
{
public:
    SkyLevel_Level2();
    virtual bool SetupLevel();
};

class SkyLevel_Level3 : public SkyLevel
{
public:
    SkyLevel_Level3();
    virtual bool SetupLevel();
};


class SkyPlayer : public SkyCharacterEntity
{
protected:
    void FireBullet();

public:
    SkyPlayer(int health);
    virtual void Reset();
    virtual void Update(sTime curTime, sTime delta) override;
    virtual void Draw(sPicture* p_buffer) override;
};


