#include "Skybound.h"

SkyLevel_Level1::SkyLevel_Level1() : SkyLevel({100, 18}) {}
bool SkyLevel_Level1::SetupLevel()
{
    // Низ — земля (4 строки)
    for (int y = (int)_SizeInTiles.H - 4; y < (int)_SizeInTiles.H; ++y)
        for (int x = 0; x < (int)_SizeInTiles.W; ++x)
            AddGroundTile(x, y);

    // Плавающие платформы
    for (int x = 10; x < 15; ++x) AddGroundTile(x, 12);
    for (int x = 25; x < 30; ++x) AddGroundTile(x, 10);
    for (int x = 40; x < 46; ++x) AddGroundTile(x, 8);
    for (int x = 60; x < 70; ++x) AddGroundTile(x, 11);
    for (int x = 80; x < 90; ++x) AddGroundTile(x, 9);

    // Шипы
    for (int x = 20; x < 30; ++x) AddSpikeTile(x, _SizeInTiles.H - 5);
    for (int x = 42; x < 45; ++x) AddSpikeTile(x, 7);

    // Декор
    for (int x = 5; x < 10; ++x) AddDecorTile(x, _SizeInTiles.H - 5);
    for (int x = 50; x < 55; ++x) AddDecorTile(x, 13);
    return true;
}
bool SkyLevel_Level1::SetupEnemies()
{
    AddEnemie(new SkyEnemie(), { 12 * _SizeOfTile.W, 11 * _SizeOfTile.H }, 3, 60.0f);
    AddEnemie(new SkyEnemie(), { 27 * _SizeOfTile.W, 9 * _SizeOfTile.H }, 3, 60.0f);
    AddEnemie(new SkyEnemie(), { 65 * _SizeOfTile.W, 10 * _SizeOfTile.H }, 3, 80.0f);
    return true;
}
bool SkyLevel_Level1::SetupPlayer()
{
    SetPlayer(new SkyPlayer(), { 50.0f, 100.0f }, 4, 100.0f);
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SkyLevel_Level2::SkyLevel_Level2() : SkyLevel({ 100, 18 }) {}
bool SkyLevel_Level2::SetupLevel()
{
    // bottom ground
    for (int y = (int)_SizeInTiles.H - 3; y < (int)_SizeInTiles.H; ++y)
        for (int x = 0; x < (int)_SizeInTiles.W; ++x)
            AddGroundTile(x, y);

    // top ceiling
    for (int x = 0; x < (int)_SizeInTiles.W; ++x)
        AddGroundTile(x, 2);

    // pillars
    for (int y = 3; y < (int)_SizeInTiles.H - 3; ++y)
    {
        if (y % 4 == 0)
        {
            AddGroundTile(15, y);
            AddGroundTile(35, y);
            AddGroundTile(55, y);
            AddGroundTile(75, y);
        }
    }

    // spike pits
    auto MakePit = [this](int x0, int x1)
        {
            for (int x = x0; x < x1; ++x)
            {
                delete GetTile(x, _SizeInTiles.H - 3);
            }
            for (int x = x0; x < x1; ++x)
                AddSpikeTile(x, _SizeInTiles.H - 4);
        };

    MakePit(8, 12);
    MakePit(30, 34);
    MakePit(60, 64);

    // decor on ceiling
    for (int x = 5; x < 10; ++x) AddDecorTile(x, 3);
    for (int x = 40; x < 45; ++x) AddDecorTile(x, 3);
    for (int x = 70; x < 75; ++x) AddDecorTile(x, 3);

    return true;
}
bool SkyLevel_Level2::SetupEnemies()
{
    AddEnemie(new SkyEnemie(), { 12 * _SizeOfTile.W, 11 * _SizeOfTile.H }, 3, 60.0f);
    AddEnemie(new SkyEnemie(), { 27 * _SizeOfTile.W, 9 * _SizeOfTile.H }, 3, 60.0f);
    AddEnemie(new SkyEnemie(), { 65 * _SizeOfTile.W, 10 * _SizeOfTile.H }, 3, 80.0f);
    return true;
}
bool SkyLevel_Level2::SetupPlayer()
{
    SetPlayer(new SkyPlayer(), { 50.0f, 100.0f }, 4, 100.0f);
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SkyLevel_Level3::SkyLevel_Level3() : SkyLevel({ 100, 18 }) {}
bool SkyLevel_Level3::SetupLevel()
{
    int baseY = _SizeInTiles.H - 4;
    for (int step = 0; step < 10; ++step)
    {
        int y = baseY - step;
        int xStart = 5 + step * 6;
        int xEnd = xStart + 5;
        if (y < 0) break;

        for (int x = xStart; x <= xEnd && x < (int)_SizeInTiles.W; ++x)
            AddGroundTile(x, y);
    }

    // bottom spikes
    for (int x = 0; x < (int)_SizeInTiles.W; ++x)
        AddSpikeTile(x, _SizeInTiles.H - 1);

    // safe ground at start/end
    for (int x = 0; x < 8; ++x)
        AddGroundTile(x, _SizeInTiles.H - 2);
    for (int x = (int)_SizeInTiles.W - 8; x < (int)_SizeInTiles.W; ++x)
        AddGroundTile(x, _SizeInTiles.H - 2);

    // decor
    for (int x = 15; x < 20; ++x) AddDecorTile(x, _SizeInTiles.H - 6);
    for (int x = 40; x < 45; ++x) AddDecorTile(x, _SizeInTiles.H - 8);
    for (int x = 70; x < 75; ++x) AddDecorTile(x, _SizeInTiles.H - 10);
    return true;
}
bool SkyLevel_Level3::SetupEnemies()
{
    AddEnemie(new SkyEnemie(), { 12 * _SizeOfTile.W, 11 * _SizeOfTile.H }, 3, 60.0f);
    //AddEnemie(new SkyEnemie(), { 27 * _SizeOfTile.W, 9 * _SizeOfTile.H }, 3, 60.0f);
    //AddEnemie(new SkyEnemie(), { 65 * _SizeOfTile.W, 10 * _SizeOfTile.H }, 3, 80.0f);
    return true;
}
bool SkyLevel_Level3::SetupPlayer()
{
    SetPlayer(new SkyPlayer(), { 50.0f, 100.0f }, 4, 100.0f);
    return true;
}