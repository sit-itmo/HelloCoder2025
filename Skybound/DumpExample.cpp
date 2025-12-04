#include <iostream>
#include <string>
#include <vector>
#include <sstream>

// -----------------------------------------------------------------------------
// Dummy text style system (replace with your real implementation)
// -----------------------------------------------------------------------------

enum class TxtColor
{
    Default,
    White,
    Yellow,
    Cyan,
    Green,
    Magenta,
    Red,
    Blue
};

// In your engine these would be implemented elsewhere.
// Here they are no-ops so the example compiles and runs.
void TxtSetStyle(TxtColor /*fg*/, TxtColor /*bg*/, bool /*bold*/, bool /*italic*/)
{
    // no-op in demo
}

void TxtResetStyle()
{
    // no-op in demo
}

// -----------------------------------------------------------------------------
// Skybound-style gameplay data structures (no inheritance)
// -----------------------------------------------------------------------------

struct Player
{
    std::string name;
    int         health;
    float       moveSpeed;
    float       jumpForce;
};

struct Enemy
{
    std::string name;
    int         health;
    float       moveSpeed;
    float       patrolRange;
};

struct Platform
{
    std::string id;
    float       x;
    float       y;
    float       width;
    float       height;
    bool        oneWay;
};

struct Collectible
{
    std::string id;
    std::string type;   // e.g. "coin", "gem"
    int         value;
    float       x;
    float       y;
};

struct BackgroundLayer
{
    std::string name;
    float       parallaxFactor;
    std::string sprite;
};

struct GameplayLayer
{
    Player                  player;
    std::vector<Enemy>      enemies;
    std::vector<Platform>   platforms;
    std::vector<Collectible> collectibles;
};

struct Level
{
    std::string   name;
    float         gravity;
    BackgroundLayer background;
    GameplayLayer  gameplay;
};

// -----------------------------------------------------------------------------
// Helper: tree printing utilities
// -----------------------------------------------------------------------------

// Print a line for a "node" (struct object), with some style
void PrintNodeLine(const std::string& prefix, bool isLast, const std::string& label)
{
    std::cout << prefix << (isLast ? "*-- " : "|-- ");

    TxtSetStyle(TxtColor::Cyan, TxtColor::Default, true, false);
    std::cout << label;
    TxtResetStyle();

    std::cout << "\n";
}

// Print a line for a "property" (leaf with name + value)
void PrintPropertyLine(const std::string& prefix, bool isLast,
    const std::string& name, const std::string& value)
{
    std::cout << prefix << (isLast ? "*-- " : "|-- ");

    TxtSetStyle(TxtColor::White, TxtColor::Default, false, false);
    std::cout << name << ": ";
    TxtResetStyle();

    std::cout << value << "\n";
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

void DumpPlayer(const Player& p, const std::string& prefix, bool isLast)
{
    PrintNodeLine(prefix, isLast, "Player");

    std::string childPrefix = prefix + (isLast ? "    " : "|   ");

    // 4 properties
    PrintPropertyLine(childPrefix, false, "name", p.name);
    PrintPropertyLine(childPrefix, false, "health", std::to_string(p.health));
    PrintPropertyLine(childPrefix, false, "moveSpeed", ToString(p.moveSpeed));
    PrintPropertyLine(childPrefix, true, "jumpForce", ToString(p.jumpForce));
}

void DumpEnemy(const Enemy& e, const std::string& prefix, bool isLast)
{
    PrintNodeLine(prefix, isLast, "Enemy: " + e.name);

    std::string childPrefix = prefix + (isLast ? "    " : "|   ");

    PrintPropertyLine(childPrefix, false, "health", std::to_string(e.health));
    PrintPropertyLine(childPrefix, false, "moveSpeed", ToString(e.moveSpeed));
    PrintPropertyLine(childPrefix, true, "patrolRange", ToString(e.patrolRange));
}

void DumpPlatform(const Platform& p, const std::string& prefix, bool isLast)
{
    PrintNodeLine(prefix, isLast, "Platform: " + p.id);

    std::string childPrefix = prefix + (isLast ? "    " : "|   ");

    PrintPropertyLine(childPrefix, false, "x", ToString(p.x));
    PrintPropertyLine(childPrefix, false, "y", ToString(p.y));
    PrintPropertyLine(childPrefix, false, "width", ToString(p.width));
    PrintPropertyLine(childPrefix, false, "height", ToString(p.height));
    PrintPropertyLine(childPrefix, true, "oneWay", p.oneWay ? "true" : "false");
}

void DumpCollectible(const Collectible& c, const std::string& prefix, bool isLast)
{
    PrintNodeLine(prefix, isLast, "Collectible: " + c.id);

    std::string childPrefix = prefix + (isLast ? "    " : "|   ");

    PrintPropertyLine(childPrefix, false, "type", c.type);
    PrintPropertyLine(childPrefix, false, "value", std::to_string(c.value));
    PrintPropertyLine(childPrefix, false, "x", ToString(c.x));
    PrintPropertyLine(childPrefix, true, "y", ToString(c.y));
}

void DumpBackgroundLayer(const BackgroundLayer& bg, const std::string& prefix, bool isLast)
{
    PrintNodeLine(prefix, isLast, "BackgroundLayer");

    std::string childPrefix = prefix + (isLast ? "    " : "|   ");

    PrintPropertyLine(childPrefix, false, "name", bg.name);
    PrintPropertyLine(childPrefix, false, "parallaxFactor", ToString(bg.parallaxFactor));
    PrintPropertyLine(childPrefix, true, "sprite", bg.sprite);
}

void DumpGameplayLayer(const GameplayLayer& g, const std::string& prefix, bool isLast)
{
    PrintNodeLine(prefix, isLast, "GameplayLayer");

    std::string childPrefix = prefix + (isLast ? "    " : "|   ");

    // Children of GameplayLayer: Player, Enemies group, Platforms group, Collectibles group
    // We'll print all four groups in fixed order for this demo.

    // 1) Player
    DumpPlayer(g.player, childPrefix, false);

    // 2) Enemies group
    {
        bool groupIsLast = false; // we still have Platforms and Collectibles after this

        PrintNodeLine(childPrefix, groupIsLast, "Enemies");
        std::string enemiesPrefix = childPrefix + (groupIsLast ? "    " : "|   ");

        for (size_t i = 0; i < g.enemies.size(); ++i)
        {
            bool lastEnemy = (i + 1 == g.enemies.size());
            DumpEnemy(g.enemies[i], enemiesPrefix, lastEnemy);
        }
    }

    // 3) Platforms group
    {
        bool groupIsLast = false; // we still have Collectibles after this

        PrintNodeLine(childPrefix, groupIsLast, "Platforms");
        std::string platformsPrefix = childPrefix + (groupIsLast ? "    " : "|   ");

        for (size_t i = 0; i < g.platforms.size(); ++i)
        {
            bool lastPlatform = (i + 1 == g.platforms.size());
            DumpPlatform(g.platforms[i], platformsPrefix, lastPlatform);
        }
    }

    // 4) Collectibles group (last child of GameplayLayer)
    {
        bool groupIsLast = true;

        PrintNodeLine(childPrefix, groupIsLast, "Collectibles");
        std::string colPrefix = childPrefix + (groupIsLast ? "    " : "|   ");

        for (size_t i = 0; i < g.collectibles.size(); ++i)
        {
            bool lastC = (i + 1 == g.collectibles.size());
            DumpCollectible(g.collectibles[i], colPrefix, lastC);
        }
    }
}

void DumpLevel(const Level& level)
{
    // Root line (no tree branches here)
    TxtSetStyle(TxtColor::Yellow, TxtColor::Default, true, false);
    std::cout << "Level: " << level.name << "\n";
    TxtResetStyle();

    // Root-level properties as a small tree
    // Make "gravity" appear as a first child node
    PrintPropertyLine("", false, "gravity", ToString(level.gravity));

    // Children: BackgroundLayer, GameplayLayer
    DumpBackgroundLayer(level.background, "", false);
    DumpGameplayLayer(level.gameplay, "", true);
}

// -----------------------------------------------------------------------------
// Demo structure creation
// -----------------------------------------------------------------------------

Level CreateDemoSkyboundLevel()
{
    Level lvl;
    lvl.name = "Skybound Demo Level";
    lvl.gravity = 9.81f;

    // Background
    lvl.background.name = "Distant Clouds";
    lvl.background.parallaxFactor = 0.35f;
    lvl.background.sprite = "bg_clouds_strip.png";

    // Player
    lvl.gameplay.player.name = "Skybound Hero";
    lvl.gameplay.player.health = 100;
    lvl.gameplay.player.moveSpeed = 6.5f;
    lvl.gameplay.player.jumpForce = 14.0f;

    // Enemies
    Enemy e1;
    e1.name = "Slime_Green";
    e1.health = 35;
    e1.moveSpeed = 3.0f;
    e1.patrolRange = 5.0f;

    Enemy e2;
    e2.name = "Bat_Purple";
    e2.health = 20;
    e2.moveSpeed = 4.5f;
    e2.patrolRange = 7.5f;

    lvl.gameplay.enemies.push_back(e1);
    lvl.gameplay.enemies.push_back(e2);

    // Platforms
    Platform p1;
    p1.id = "Platform_01";
    p1.x = 0.0f;
    p1.y = -2.0f;
    p1.width = 6.0f;
    p1.height = 0.5f;
    p1.oneWay = false;

    Platform p2;
    p2.id = "Platform_02";
    p2.x = 8.0f;
    p2.y = 1.0f;
    p2.width = 4.0f;
    p2.height = 0.5f;
    p2.oneWay = true;

    lvl.gameplay.platforms.push_back(p1);
    lvl.gameplay.platforms.push_back(p2);

    // Collectibles
    Collectible c1;
    c1.id = "Coin_001";
    c1.type = "coin";
    c1.value = 10;
    c1.x = 1.5f;
    c1.y = 0.5f;

    Collectible c2;
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
    Level demo = CreateDemoSkyboundLevel();

    std::cout << "Skybound 2D Platformer Hierarchy Dump (Tree View)\n\n";
    DumpLevel(demo);

    return 0;
}
