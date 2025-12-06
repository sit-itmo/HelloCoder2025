
#include <iostream>
#include <string>
#include <vector>
#include <sstream>

#include "Skybound.h"
#include "ThorSerialize/JsonThor.h"
#include "ThorSerialize/SerUtil.h"

struct dPlayer
{
    std::string name;
    int         health;
    float       moveSpeed;
    float       jumpForce;
};
ThorsAnvil_MakeTrait(dPlayer, name, health, moveSpeed, jumpForce);

struct dEnemy
{
    std::string name;
    int         health;
    float       moveSpeed;
    float       patrolRange;
};
ThorsAnvil_MakeTrait(dEnemy, name, health, moveSpeed, patrolRange);

struct dPlatform
{
    std::string id;
    float       x;
    float       y;
    float       width;
    float       height;
    bool        oneWay;
};
ThorsAnvil_MakeTrait(dPlatform, id, width, height, oneWay, x, y);

struct dCollectible
{
    std::string id;
    std::string type;   // e.g. "coin", "gem"
    int         value;
    float       x;
    float       y;
};
ThorsAnvil_MakeTrait(dCollectible, id, type, value, x, y);


struct dBackgroundLayer
{
    std::string name;
    float       parallaxFactor;
    std::string sprite;
};
ThorsAnvil_MakeTrait(dBackgroundLayer, name, parallaxFactor, sprite);

struct dGameplayLayer
{
//    friend class ThorsAnvil::Serialize::Traits<dGameplayLayer>;
//private:
    dPlayer                  player;
    std::vector<dEnemy>      enemies;
    std::vector<dPlatform>   platforms;
    std::vector<dCollectible> collectibles;
};

ThorsAnvil_MakeTrait(dGameplayLayer, player, enemies, platforms, collectibles);

struct dLevel
{
    std::string   name;
    float         gravity;
    dBackgroundLayer background;
    dGameplayLayer  gameplay;
};
ThorsAnvil_MakeTrait(dLevel, name, gravity, background, gameplay);

struct SkyDump
{
    bool AllowPrint;
    bool UseColors;
    std::string Result;
};

//ThorsAnvil_ExpandTrait(b, t, g1);


// -----------------------------------------------------------------------------
// Helper: tree printing utilities
// -----------------------------------------------------------------------------

// Print a line for a "node" (struct object), with some style
void PrintNodeLine(SkyDump &dump, const std::string& prefix, bool isLast, const std::string& label)
{
    std::stringstream str;

    
    str << prefix << (isLast ? "*-- " : "|-- ");
    if (dump.UseColors)
    {
        str << ANSI::makeStyle(ANSI::Color::Green, ANSI::Color::Default, true, false);
    }

    str << label << "\n";

    if (dump.UseColors)
    {
        str << ANSI::reset();
    }

    dump.Result += str.str();
    if (dump.AllowPrint)
    {
        std::cout << str.str();
    }
}

// Print a line for a "node" (struct object), with some style
void PrintLevelLine(const dLevel& level, SkyDump& dump)
{
    std::stringstream str;

    if (dump.UseColors)
    {
        str << ANSI::makeStyle(ANSI::Color::Yellow, ANSI::Color::Default, true, false);
    }
    str << "dLevel: " << level.name << "\n";
    
    if (dump.UseColors)
    {
        str << ANSI::reset();
    }

    dump.Result += str.str();
    if (dump.AllowPrint)
    {
        std::cout << str.str();
    }
}

// Print a line for a "property" (leaf with name + value)
void PrintPropertyLine(SkyDump& dump, const std::string& prefix, bool isLast,
    const std::string& name, const std::string& value)
{
    std::stringstream str;


    str << prefix << (isLast ? "*-- " : "|-- ");
    if (dump.UseColors)
    {
        str << ANSI::makeStyle(ANSI::Color::Yellow, ANSI::Color::Default, false, false);
    }
    
    str << name;

    if (dump.UseColors)
    {
        str << ANSI::reset();
    }
    str << ": ";
    str << value << "\n";

    dump.Result += str.str();
    if (dump.AllowPrint)
    {
        std::cout << str.str();
    }
}

// Float formatting helper
std::string ToString(float v)
{
    std::ostringstream oss;
    oss.precision(3);
    oss << std::fixed << v;
    return oss.str();
}

// -----------------------------------------------------------------------------
// Dump functions for each specific type (no inheritance)
// -----------------------------------------------------------------------------

void DumpPlayer(const dPlayer& p, SkyDump& dump, const std::string& prefix, bool isLast)
{
    PrintNodeLine(dump, prefix, isLast, "dPlayer");

    std::string childPrefix = prefix + (isLast ? "    " : "|   ");

    // 4 properties
    PrintPropertyLine(dump, childPrefix, false, "name", p.name);
    PrintPropertyLine(dump, childPrefix, false, "health", std::to_string(p.health));
    PrintPropertyLine(dump, childPrefix, false, "moveSpeed", ToString(p.moveSpeed));
    PrintPropertyLine(dump, childPrefix, true, "jumpForce", ToString(p.jumpForce));
}

void DumpEnemy(const dEnemy& e, SkyDump& dump, const std::string& prefix, bool isLast)
{
    PrintNodeLine(dump, prefix, isLast, "dEnemy: " + e.name);

    std::string childPrefix = prefix + (isLast ? "    " : "|   ");

    PrintPropertyLine(dump, childPrefix, false, "health", std::to_string(e.health));
    PrintPropertyLine(dump, childPrefix, false, "moveSpeed", ToString(e.moveSpeed));
    PrintPropertyLine(dump, childPrefix, true, "patrolRange", ToString(e.patrolRange));
}

void DumpPlatform(const dPlatform& p, SkyDump& dump, const std::string& prefix, bool isLast)
{
    PrintNodeLine(dump, prefix, isLast, "dPlatform: " + p.id);

    std::string childPrefix = prefix + (isLast ? "    " : "|   ");

    PrintPropertyLine(dump, childPrefix, false, "x", ToString(p.x));
    PrintPropertyLine(dump, childPrefix, false, "y", ToString(p.y));
    PrintPropertyLine(dump, childPrefix, false, "width", ToString(p.width));
    PrintPropertyLine(dump, childPrefix, false, "height", ToString(p.height));
    PrintPropertyLine(dump, childPrefix, true, "oneWay", p.oneWay ? "true" : "false");
}

void DumpCollectible(const dCollectible& c, SkyDump& dump, const std::string& prefix, bool isLast)
{
    PrintNodeLine(dump, prefix, isLast, "dCollectible: " + c.id);

    std::string childPrefix = prefix + (isLast ? "    " : "|   ");

    PrintPropertyLine(dump, childPrefix, false, "type", c.type);
    PrintPropertyLine(dump, childPrefix, false, "value", std::to_string(c.value));
    PrintPropertyLine(dump, childPrefix, false, "x", ToString(c.x));
    PrintPropertyLine(dump, childPrefix, true, "y", ToString(c.y));
}

void DumpBackgroundLayer(const dBackgroundLayer& bg, SkyDump& dump, const std::string& prefix, bool isLast)
{
    PrintNodeLine(dump, prefix, isLast, "dBackgroundLayer");

    std::string childPrefix = prefix + (isLast ? "    " : "|   ");

    PrintPropertyLine(dump, childPrefix, false, "name", bg.name);
    PrintPropertyLine(dump, childPrefix, false, "parallaxFactor", ToString(bg.parallaxFactor));
    PrintPropertyLine(dump, childPrefix, true, "sprite", bg.sprite);
}

void DumpGameplayLayer(const dGameplayLayer& g, SkyDump& dump, const std::string& prefix, bool isLast)
{
    PrintNodeLine(dump, prefix, isLast, "dGameplayLayer");

    std::string childPrefix = prefix + (isLast ? "    " : "|   ");

    // Children of dGameplayLayer: dPlayer, Enemies group, Platforms group, Collectibles group
    // We'll print all four groups in fixed order for this demo.

    // 1) dPlayer
    DumpPlayer(g.player, dump, childPrefix, false);

    // 2) Enemies group
    {
        bool groupIsLast = false; // we still have Platforms and Collectibles after this

        PrintNodeLine(dump, childPrefix, groupIsLast, "Enemies");
        std::string enemiesPrefix = childPrefix + (groupIsLast ? "    " : "|   ");

        for (size_t i = 0; i < g.enemies.size(); ++i)
        {
            bool lastEnemy = (i + 1 == g.enemies.size());
            DumpEnemy(g.enemies[i], dump, enemiesPrefix, lastEnemy);
        }
    }

    // 3) Platforms group
    {
        bool groupIsLast = false; // we still have Collectibles after this

        PrintNodeLine(dump, childPrefix, groupIsLast, "Platforms");
        std::string platformsPrefix = childPrefix + (groupIsLast ? "    " : "|   ");

        for (size_t i = 0; i < g.platforms.size(); ++i)
        {
            bool lastPlatform = (i + 1 == g.platforms.size());
            DumpPlatform(g.platforms[i], dump, platformsPrefix, lastPlatform);
        }
    }

    // 4) Collectibles group (last child of dGameplayLayer)
    {
        bool groupIsLast = true;

        PrintNodeLine(dump, childPrefix, groupIsLast, "Collectibles");
        std::string colPrefix = childPrefix + (groupIsLast ? "    " : "|   ");

        for (size_t i = 0; i < g.collectibles.size(); ++i)
        {
            bool lastC = (i + 1 == g.collectibles.size());
            DumpCollectible(g.collectibles[i], dump, colPrefix, lastC);
        }
    }
}

void DumpLevel(const dLevel& level, SkyDump& dump)
{
    

    // Root-level properties as a small tree
    // Make "gravity" appear as a first child node
    PrintPropertyLine(dump, "", false, "gravity", ToString(level.gravity));

    // Children: dBackgroundLayer, dGameplayLayer
    DumpBackgroundLayer(level.background, dump, "", false);
    DumpGameplayLayer(level.gameplay, dump, "", true);
}

// -----------------------------------------------------------------------------
// Demo structure creation
// -----------------------------------------------------------------------------

dLevel CreateDemoSkyboundLevel()
{
    dLevel lvl;
    lvl.name = "Skybound Demo dLevel";
    lvl.gravity = 9.81f;

    // Background
    lvl.background.name = "Distant Clouds";
    lvl.background.parallaxFactor = 0.35f;
    lvl.background.sprite = "bg_clouds_strip.png";

    // dPlayer
    lvl.gameplay.player.name = "Skybound Hero";
    lvl.gameplay.player.health = 100;
    lvl.gameplay.player.moveSpeed = 6.5f;
    lvl.gameplay.player.jumpForce = 14.0f;

    // Enemies
    dEnemy e1;
    e1.name = "Slime_Green";
    e1.health = 35;
    e1.moveSpeed = 3.0f;
    e1.patrolRange = 5.0f;

    dEnemy e2;
    e2.name = "Bat_Purple";
    e2.health = 20;
    e2.moveSpeed = 4.5f;
    e2.patrolRange = 7.5f;

    lvl.gameplay.enemies.push_back(e1);
    lvl.gameplay.enemies.push_back(e2);

    // Platforms
    dPlatform p1;
    p1.id = "Platform_01";
    p1.x = 0.0f;
    p1.y = -2.0f;
    p1.width = 6.0f;
    p1.height = 0.5f;
    p1.oneWay = false;

    dPlatform p2;
    p2.id = "Platform_02";
    p2.x = 8.0f;
    p2.y = 1.0f;
    p2.width = 4.0f;
    p2.height = 0.5f;
    p2.oneWay = true;

    lvl.gameplay.platforms.push_back(p1);
    lvl.gameplay.platforms.push_back(p2);

    // Collectibles
    dCollectible c1;
    c1.id = "Coin_001";
    c1.type = "coin";
    c1.value = 10;
    c1.x = 1.5f;
    c1.y = 0.5f;

    dCollectible c2;
    c2.id = "Gem_001";
    c2.type = "gem";
    c2.value = 50;
    c2.x = 9.0f;
    c2.y = 2.5f;

    lvl.gameplay.collectibles.push_back(c1);
    lvl.gameplay.collectibles.push_back(c2);

    return lvl;
}

// -----------------------------------------------------------------------------
// main
// -----------------------------------------------------------------------------
int Dump_RunTest()
{
    SkyDump dump = {false, true, ""};

    dLevel demo = CreateDemoSkyboundLevel();

    std::stringstream str_json;
    str_json << ThorsAnvil::Serialize::jsonExport(demo) << std::endl;
    std::string str_json_base = str_json.str();
    SKY_PRINTLN(str_json_base.c_str());

    std::istringstream input(str_json_base);
    dLevel demo2;
    input >> ThorsAnvil::Serialize::jsonImport(demo2);

    std::cout << "Skybound 2D Platformer Hierarchy Dump (Tree View)\n\n";
    DumpLevel(demo2, dump);
    //std::cout << dump.Result;

    SKY_PRINTLN(dump.Result.c_str());

    return 0;
}
