#include "Skybound.h"

sSize2D SkyLevel_Level1::GetSize() { return { 100, 18 }; }
bool SkyLevel_Level1::SetupLevel(SkyLevel* p_level)
{
    // Низ — земля (4 строки)
    for (int y = (int)p_level->SizeInTiles().H - 4; y < (int)p_level->SizeInTiles().H; ++y)
        for (int x = 0; x < (int)p_level->SizeInTiles().W; ++x)
            p_level->AddGroundTile(x, y);

    // Плавающие платформы
    for (int x = 10; x < 15; ++x) p_level->AddGroundTile(x, 12);
    for (int x = 25; x < 30; ++x) p_level->AddGroundTile(x, 10);
    for (int x = 40; x < 46; ++x) p_level->AddGroundTile(x, 8);
    for (int x = 60; x < 70; ++x) p_level->AddGroundTile(x, 11);
    for (int x = 80; x < 90; ++x) p_level->AddGroundTile(x, 9);

    // Шипы
    for (int x = 20; x < 30; ++x) p_level->AddSpikeTile(x, p_level->SizeInTiles().H - 5);
    for (int x = 42; x < 45; ++x) p_level->AddSpikeTile(x, 7);

    // Декор
    for (int x = 5; x < 10; ++x) p_level->AddDecorTile(x, p_level->SizeInTiles().H - 5);
    for (int x = 50; x < 55; ++x) p_level->AddDecorTile(x, 13);
    return true;
}
bool SkyLevel_Level1::SetupEnemies(SkyLevel* p_level)
{
    p_level->AddEnemie(new SkyEnemie(), { 12 * p_level->SizeOfTile().W, 11 * p_level->SizeOfTile().H }, 3, 60.0f);
    p_level->AddEnemie(new SkyEnemie(), { 27 * p_level->SizeOfTile().W, 9 * p_level->SizeOfTile().H }, 3, 60.0f);
    p_level->AddEnemie(new SkyEnemie(), { 65 * p_level->SizeOfTile().W, 10 * p_level->SizeOfTile().H }, 3, 80.0f);
    return true;
}
bool SkyLevel_Level1::SetupPlayer(SkyLevel* p_level)
{
    p_level->SetPlayer(new SkyPlayer(), { 50.0f, 100.0f }, 4, 100.0f);
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
sSize2D SkyLevel_Level2::GetSize() { return { 100, 18 }; }
bool SkyLevel_Level2::SetupLevel(SkyLevel* p_level)
{
    // bottom ground
    for (int y = (int)p_level->SizeInTiles().H - 3; y < (int)p_level->SizeInTiles().H; ++y)
        for (int x = 0; x < (int)p_level->SizeInTiles().W; ++x)
            p_level->AddGroundTile(x, y);

    // top ceiling
    for (int x = 0; x < (int)p_level->SizeInTiles().W; ++x)
        p_level->AddGroundTile(x, 2);

    // pillars
    for (int y = 3; y < (int)p_level->SizeInTiles().H - 3; ++y)
    {
        if (y % 4 == 0)
        {
            p_level->AddGroundTile(15, y);
            p_level->AddGroundTile(35, y);
            p_level->AddGroundTile(55, y);
            p_level->AddGroundTile(75, y);
        }
    }

    // spike pits
    auto MakePit = [this, p_level](int x0, int x1)
        {
            for (int x = x0; x < x1; ++x)
            {
                delete p_level->GetTile(x, p_level->SizeInTiles().H - 3);
            }
            for (int x = x0; x < x1; ++x)
                p_level->AddSpikeTile(x, p_level->SizeInTiles().H - 4);
        };

    MakePit(8, 12);
    MakePit(30, 34);
    MakePit(60, 64);

    // decor on ceiling
    for (int x = 5; x < 10; ++x) p_level->AddDecorTile(x, 3);
    for (int x = 40; x < 45; ++x) p_level->AddDecorTile(x, 3);
    for (int x = 70; x < 75; ++x) p_level->AddDecorTile(x, 3);

    return true;
}
bool SkyLevel_Level2::SetupEnemies(SkyLevel* p_level)
{
    p_level->AddEnemie(new SkyEnemie(), { 12 * p_level->SizeOfTile().W, 11 * p_level->SizeOfTile().H }, 3, 60.0f);
    p_level->AddEnemie(new SkyEnemie(), { 27 * p_level->SizeOfTile().W, 9 * p_level->SizeOfTile().H }, 3, 60.0f);
    p_level->AddEnemie(new SkyEnemie(), { 65 * p_level->SizeOfTile().W, 10 * p_level->SizeOfTile().H }, 3, 80.0f);
    return true;
}
bool SkyLevel_Level2::SetupPlayer(SkyLevel* p_level)
{
    p_level->SetPlayer(new SkyPlayer(), { 50.0f, 100.0f }, 4, 100.0f);
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
sSize2D SkyLevel_Level3::GetSize() { return { 100, 18 }; }
bool SkyLevel_Level3::SetupLevel(SkyLevel* p_level)
{
    int baseY = p_level->SizeInTiles().H - 4;
    for (int step = 0; step < 10; ++step)
    {
        int y = baseY - step;
        int xStart = 5 + step * 6;
        int xEnd = xStart + 5;
        if (y < 0) break;

        for (int x = xStart; x <= xEnd && x < (int)p_level->SizeInTiles().W; ++x)
            p_level->AddGroundTile(x, y);
    }

    // bottom spikes
    for (int x = 0; x < (int)p_level->SizeInTiles().W; ++x)
        p_level->AddSpikeTile(x, p_level->SizeInTiles().H - 1);

    // safe ground at start/end
    for (int x = 0; x < 8; ++x)
        p_level->AddGroundTile(x, p_level->SizeInTiles().H - 2);
    for (int x = (int)p_level->SizeInTiles().W - 8; x < (int)p_level->SizeInTiles().W; ++x)
        p_level->AddGroundTile(x, p_level->SizeInTiles().H - 2);

    // decor
    for (int x = 15; x < 20; ++x) p_level->AddDecorTile(x, p_level->SizeInTiles().H - 6);
    for (int x = 40; x < 45; ++x) p_level->AddDecorTile(x, p_level->SizeInTiles().H - 8);
    for (int x = 70; x < 75; ++x) p_level->AddDecorTile(x, p_level->SizeInTiles().H - 10);
    return true;
}
bool SkyLevel_Level3::SetupEnemies(SkyLevel* p_level)
{
    p_level->AddEnemie(new SkyEnemie(), { 12 * p_level->SizeOfTile().W, 11 * p_level->SizeOfTile().H }, 3, 60.0f);
    p_level->AddEnemie(new SkyEnemie(), { 27 * p_level->SizeOfTile().W, 9 * p_level->SizeOfTile().H }, 3, 60.0f);
    p_level->AddEnemie(new SkyEnemie(), { 65 * p_level->SizeOfTile().W, 10 * p_level->SizeOfTile().H }, 3, 80.0f);
    return true;
}
bool SkyLevel_Level3::SetupPlayer(SkyLevel* p_level)
{
    p_level->SetPlayer(new SkyPlayer(), { 50.0f, 100.0f }, 4, 100.0f);
    return true;
}