#include "Skybound.h"

SkyLevel_Level1::SkyLevel_Level1() : SkyLevel({100, 18}) {}
bool SkyLevel_Level1::SetupLevel()
{
    // Низ — земля (4 строки)
    for (int y = _SizeInTiles.H - 4; y < _SizeInTiles.H; ++y)
        for (int x = 0; x < _SizeInTiles.W; ++x)
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

SkyLevel_Level2::SkyLevel_Level2() : SkyLevel({ 100, 18 }) {}
bool SkyLevel_Level2::SetupLevel()
{
    return true;
}

SkyLevel_Level3::SkyLevel_Level3() : SkyLevel({ 100, 18 }) {}
bool SkyLevel_Level3::SetupLevel()
{
    return true;
}