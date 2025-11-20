#include <windows.h>
#include <vector>
#include <png.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <map>
#include <string>
#include <vector>
#include <stdint.h> // для uint32_t


//
// Простые настройки игры
//
const int SCREEN_W = 800;
const int SCREEN_H = 600;

const int TILE_SIZE = 32;

// Сделаем уровень пошире, чтобы был смысл в прокрутке
const int LEVEL_W = 100;
const int LEVEL_H = 18;

const float GRAVITY = 900.0f;  // пикселей/сек^2
const float MOVE_SPEED = 200.0f;  // пикселей/сек
const float JUMP_SPEED = -450.0f;

HWND g_hWnd = NULL;
bool g_running = true;

struct Glyph
{
    int width;
    int height;
    int bearingX;
    int bearingY;
    int advance; // в 1/64 пиксела, но мы сразу переведём в пиксели

    std::vector<unsigned char> bitmap; // 8-битный альфа-канал (серый)
};

FT_Library g_ftLib = nullptr;
FT_Face    g_ftFace = nullptr;

// Кэш глифов: по юникод-коду
std::map<uint32_t, Glyph> g_glyphs;

// буфер кадра (ARGB)
unsigned int* g_pixels = 0;
BITMAPINFO g_bmi;

struct Player
{
    float x;
    float y;
    float vx;
    float vy;
    bool  onGround;
};

struct Enemy
{
    float x;
    float y;
    float vx;
    float vy;
    bool  alive;
};

struct Bullet
{
    float x;
    float y;
    float vx;
    bool  active;
};

Player g_player;
float  g_camX = 0.0f;    // камера по X
int    g_playerDir = 1;  // направление игрока: 1 вправо, -1 влево

// Тайлы:
// 0 - пусто
// 1 - земля (твёрдый)
// 2 - шипы (убивают)
// 3 - декор (не твёрдый)
int g_level[LEVEL_H][LEVEL_W];

// Враги и пули
const int MAX_ENEMIES = 16;
Enemy g_enemies[MAX_ENEMIES];
int   g_enemyCount = 0;

const int MAX_BULLETS = 64;
Bullet g_bullets[MAX_BULLETS];
float  g_shootCooldown = 0.0f; // таймер перезарядки

// Утилита: цвет (r,g,b) -> 0x00BBGGRR для DIB
unsigned int MakeColor(unsigned char r, unsigned char g, unsigned char b)
{
    return (unsigned int)(b | (g << 8) | (r << 16));
}

// Наш "PutPixel" в буфер
void PutPixel(int x, int y, unsigned int color)
{
    if (x < 0 || x >= SCREEN_W || y < 0 || y >= SCREEN_H) return;
    g_pixels[y * SCREEN_W + x] = color;
}

bool InitFreeType(const char* fontPath, int pixelSize)
{
    if (FT_Init_FreeType(&g_ftLib))
        return false;

    if (FT_New_Face(g_ftLib, fontPath, 0, &g_ftFace))
        return false;

    // задаём размер в пикселях по высоте
    if (FT_Set_Pixel_Sizes(g_ftFace, 0, pixelSize))
        return false;

    return true;
}

bool LoadGlyph(uint32_t codepoint, Glyph& out)
{
    // Загружаем глиф
    if (FT_Load_Char(g_ftFace, codepoint, FT_LOAD_RENDER))
        return false;

    FT_GlyphSlot slot = g_ftFace->glyph;
    FT_Bitmap& bmp = slot->bitmap;

    out.width = bmp.width;
    out.height = bmp.rows;
    out.bearingX = slot->bitmap_left;
    out.bearingY = slot->bitmap_top;
    out.advance = slot->advance.x >> 6; // из 26.6 fixed в пиксели

    out.bitmap.assign(bmp.buffer, bmp.buffer + bmp.width * bmp.rows);
    return true;
}

void PreloadAsciiGlyphs()
{
    g_glyphs.clear();

    for (uint32_t cp = 0x20; cp <= 0x7E; ++cp)
    {
        Glyph g;
        if (LoadGlyph(cp, g))
        {
            g_glyphs[cp] = g;
        }
    }
}

void BlendPixel(int x, int y,
    unsigned char fr, unsigned char fg, unsigned char fb,
    float alpha)
{
    if (x < 0 || x >= SCREEN_W || y < 0 || y >= SCREEN_H) return;

    unsigned int dst = g_pixels[y * SCREEN_W + x];

    unsigned char db = dst & 0xFF;
    unsigned char dg = (dst >> 8) & 0xFF;
    unsigned char dr = (dst >> 16) & 0xFF;

    unsigned char rr = (unsigned char)(dr * (1.0f - alpha) + fr * alpha);
    unsigned char gg = (unsigned char)(dg * (1.0f - alpha) + fg * alpha);
    unsigned char bb = (unsigned char)(db * (1.0f - alpha) + fb * alpha);

    unsigned int out = (rr << 16) | (gg << 8) | bb;
    PutPixel(x, y, out);
}

// Возвращает смещение по X (advance), чтобы можно было писать строки
int DrawGlyph(uint32_t codepoint, int penX, int baselineY,
    unsigned char r, unsigned char g, unsigned char b,
    float globalAlpha = 1.0f)
{
    auto it = g_glyphs.find(codepoint);
    if (it == g_glyphs.end())
    {
        // если глиф не предзагружен — попробуем загрузить на лету
        Glyph g;
        if (!LoadGlyph(codepoint, g))
            return 0; // ничего не рисуем

        g_glyphs[codepoint] = g;
        it = g_glyphs.find(codepoint);
    }

    const Glyph& glyph = it->second;

    // левый верхний угол глифа
    int x0 = penX + glyph.bearingX;
    int y0 = baselineY - glyph.bearingY; // FT отсчитывает от базовой линии вверх

    for (int y = 0; y < glyph.height; ++y)
    {
        for (int x = 0; x < glyph.width; ++x)
        {
            unsigned char a = glyph.bitmap[y * glyph.width + x];
            if (a == 0) continue;

            float alpha = (a / 255.0f) * globalAlpha;
            if (alpha <= 0.01f) continue;

            int sx = x0 + x;
            int sy = y0 + y;
            BlendPixel(sx, sy, r, g, b, alpha);
        }
    }

    return glyph.advance;
}

void DrawAsciiText(const std::string& text, int x, int y,
    unsigned char r, unsigned char g, unsigned char b,
    float globalAlpha = 1.0f)
{
    int penX = x;
    for (unsigned char ch : text)
    {
        if (ch == '\n')
        {
            y += 20;        // простой переход на новую строку
            penX = x;
            continue;
        }

        penX += DrawGlyph((uint32_t)ch, penX, y, r, g, b, globalAlpha);
    }
}

// Декодирует первый UTF-8 символ из строки s, возвращает его код
// и количество использованных байтов (outLen).
uint32_t DecodeUTF8Char(const char* s, int& outLen)
{
    unsigned char c = (unsigned char)s[0];
    if (c < 0x80)
    {
        outLen = 1;
        return c;
    }
    else if ((c & 0xE0) == 0xC0)
    {
        outLen = 2;
        return ((c & 0x1F) << 6) |
            ((unsigned char)s[1] & 0x3F);
    }
    else if ((c & 0xF0) == 0xE0)
    {
        outLen = 3;
        return ((c & 0x0F) << 12) |
            (((unsigned char)s[1] & 0x3F) << 6) |
            ((unsigned char)s[2] & 0x3F);
    }
    else if ((c & 0xF8) == 0xF0)
    {
        outLen = 4;
        return ((c & 0x07) << 18) |
            (((unsigned char)s[1] & 0x3F) << 12) |
            (((unsigned char)s[2] & 0x3F) << 6) |
            ((unsigned char)s[3] & 0x3F);
    }
    else
    {
        outLen = 1;
        return 0xFFFD; // replacement character
    }
}

void DrawUtf8Char(const char* utf8,
    int x, int baselineY,
    unsigned char r, unsigned char g, unsigned char b,
    float globalAlpha = 1.0f)
{
    int len = 0;
    uint32_t cp = DecodeUTF8Char(utf8, len);
    DrawGlyph(cp, x, baselineY, r, g, b, globalAlpha);
}


struct PngImage
{
    int width;
    int height;
    std::vector<unsigned char> pixels; // RGBA: 4 байта на пиксель
};


PngImage TestImage;

bool LoadPNG(const char* filename, PngImage& out)
{
    FILE* fp = fopen(filename, "rb");
    if (!fp) return false;

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) return false;

    png_infop info = png_create_info_struct(png);
    if (!info) return false;

    if (setjmp(png_jmpbuf(png))) return false;

    png_init_io(png, fp);
    png_read_info(png, info);

    out.width = png_get_image_width(png, info);
    out.height = png_get_image_height(png, info);

    png_byte color_type = png_get_color_type(png, info);
    png_byte bit_depth = png_get_bit_depth(png, info);

    // преобразуем к RGBA 8-bit
    if (bit_depth == 16) png_set_strip_16(png);
    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png);

    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png);

    if (png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);

    if (color_type == PNG_COLOR_TYPE_RGB ||
        color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_PALETTE)
    {
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER); // добавляем альфа-канал
    }

    if (color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    {
        png_set_gray_to_rgb(png);
    }

    png_read_update_info(png, info);

    // читаем построчно
    out.pixels.resize(out.width * out.height * 4);
    std::vector<png_bytep> rows(out.height);

    for (int y = 0; y < out.height; y++)
        rows[y] = (png_bytep)(out.pixels.data() + y * out.width * 4);

    png_read_image(png, rows.data());
    png_destroy_read_struct(&png, &info, NULL);
    fclose(fp);
    return true;
}

void DrawPNG(
    const PngImage& img,
    float dstX, float dstY,
    float scale,
    float globalAlpha = 1.0f  // 0..1
)
{
    int newW = (int)(img.width * scale);
    int newH = (int)(img.height * scale);

    for (int y = 0; y < newH; ++y)
    {
        float srcYf = (float)y / scale;
        int srcY = (int)srcYf;

        if (srcY < 0 || srcY >= img.height) continue;

        for (int x = 0; x < newW; ++x)
        {
            float srcXf = (float)x / scale;
            int srcX = (int)srcXf;

            if (srcX < 0 || srcX >= img.width) continue;

            unsigned char* p =
                (unsigned char*)&img.pixels[(srcY * img.width + srcX) * 4];

            unsigned char r = p[0];
            unsigned char g = p[1];
            unsigned char b = p[2];
            unsigned char a = p[3];

            // итоговая альфа
            float alpha = (a / 255.0f) * globalAlpha;
            if (alpha <= 0.01f) continue;

            // позиция на экране
            int dx = (int)(dstX + x);
            int dy = (int)(dstY + y);

            // читаем текущий пиксель экрана
            extern unsigned int* g_pixels; // ваш основной буфер
            if (dx < 0 || dx >= SCREEN_W || dy < 0 || dy >= SCREEN_H) continue;

            unsigned int dstColor = g_pixels[dy * SCREEN_W + dx];

            unsigned char db = dstColor & 0xFF;
            unsigned char dg = (dstColor >> 8) & 0xFF;
            unsigned char dr = (dstColor >> 16) & 0xFF;

            // альфа-композитинг
            unsigned char rr = (unsigned char)(dr * (1 - alpha) + r * alpha);
            unsigned char gg = (unsigned char)(dg * (1 - alpha) + g * alpha);
            unsigned char bb = (unsigned char)(db * (1 - alpha) + b * alpha);

            unsigned int out = (rr << 16) | (gg << 8) | bb;
            PutPixel(dx, dy, out);
        }
    }
}

// Очистка экрана
void ClearScreen(unsigned int color)
{
    int count = SCREEN_W * SCREEN_H;
    for (int i = 0; i < count; i++)
        g_pixels[i] = color;
}

// Инициализация уровня
void BuildLevel()
{
    // Всё пустое
    for (int y = 0; y < LEVEL_H; ++y)
    {
        for (int x = 0; x < LEVEL_W; ++x)
        {
            g_level[y][x] = 0;
        }
    }

    // Нижние 4 строки — земля
    for (int y = LEVEL_H - 4; y < LEVEL_H; ++y)
    {
        for (int x = 0; x < LEVEL_W; ++x)
        {
            g_level[y][x] = 1;
        }
    }

    // Пара "островков" платформ
    for (int x = 10; x < 15; ++x) g_level[12][x] = 1;
    for (int x = 25; x < 30; ++x) g_level[10][x] = 1;
    for (int x = 40; x < 46; ++x) g_level[8][x] = 1;
    for (int x = 60; x < 70; ++x) g_level[11][x] = 1;
    for (int x = 80; x < 90; ++x) g_level[9][x] = 1;

    // Шипы (ряд на полу и на одной из платформ)
    for (int x = 20; x < 30; ++x) g_level[LEVEL_H - 5][x] = 2;
    for (int x = 42; x < 45; ++x) g_level[7][x] = 2;

    // Декор (просто зелёные блоки)
    for (int x = 5; x < 10; ++x) g_level[LEVEL_H - 5][x] = 3;
    for (int x = 50; x < 55; ++x) g_level[13][x] = 3;
}

// 0 - пусто
// 1 - земля (твёрдый)
// 2 - шипы
// 3 - декор

void ClearLevel()
{
    for (int y = 0; y < LEVEL_H; ++y)
        for (int x = 0; x < LEVEL_W; ++x)
            g_level[y][x] = 0;
}

// ===== Уровень 1: базовый, похож на предыдущий =====
void BuildLevel_1()
{
    ClearLevel();

    // Низ — земля (4 строки)
    for (int y = LEVEL_H - 4; y < LEVEL_H; ++y)
        for (int x = 0; x < LEVEL_W; ++x)
            g_level[y][x] = 1;

    // Плавающие платформы
    for (int x = 10; x < 15; ++x) g_level[12][x] = 1;
    for (int x = 25; x < 30; ++x) g_level[10][x] = 1;
    for (int x = 40; x < 46; ++x) g_level[8][x] = 1;
    for (int x = 60; x < 70; ++x) g_level[11][x] = 1;
    for (int x = 80; x < 90; ++x) g_level[9][x] = 1;

    // Шипы
    for (int x = 20; x < 30; ++x) g_level[LEVEL_H - 5][x] = 2;
    for (int x = 42; x < 45; ++x) g_level[7][x] = 2;

    // Декор
    for (int x = 5; x < 10; ++x) g_level[LEVEL_H - 5][x] = 3;
    for (int x = 50; x < 55; ++x) g_level[13][x] = 3;
}

// ===== Уровень 2: «пещера» с туннелем и ловушками =====
void BuildLevel_2()
{
    ClearLevel();

    // Весь низ — земля
    for (int y = LEVEL_H - 3; y < LEVEL_H; ++y)
        for (int x = 0; x < LEVEL_W; ++x)
            g_level[y][x] = 1;

    // «потолок» вверху
    for (int x = 0; x < LEVEL_W; ++x)
        g_level[2][x] = 1;

    // Столбы, образующие туннели
    for (int y = 3; y < LEVEL_H - 3; ++y)
    {
        if (y % 4 == 0)
        {
            // стены
            g_level[y][15] = 1;
            g_level[y][35] = 1;
            g_level[y][55] = 1;
            g_level[y][75] = 1;
        }
    }

    // Ямы с шипами в полу
    for (int x = 8; x < 12; ++x) g_level[LEVEL_H - 3][x] = 0;
    for (int x = 8; x < 12; ++x) g_level[LEVEL_H - 4][x] = 2;

    for (int x = 30; x < 34; ++x) g_level[LEVEL_H - 3][x] = 0;
    for (int x = 30; x < 34; ++x) g_level[LEVEL_H - 4][x] = 2;

    for (int x = 60; x < 64; ++x) g_level[LEVEL_H - 3][x] = 0;
    for (int x = 60; x < 64; ++x) g_level[LEVEL_H - 4][x] = 2;

    // Немного декора на «потолке»
    for (int x = 5; x < 10; ++x) g_level[3][x] = 3;
    for (int x = 40; x < 45; ++x) g_level[3][x] = 3;
    for (int x = 70; x < 75; ++x) g_level[3][x] = 3;
}

// ===== Уровень 3: серия ступенек с шипами снизу =====
void BuildLevel_3()
{
    ClearLevel();

    // По диагонали поднимающиеся платформы
    int baseY = LEVEL_H - 4;
    for (int step = 0; step < 10; ++step)
    {
        int y = baseY - step;   // каждая ступень выше
        int xStart = 5 + step * 6;
        int xEnd = xStart + 5;
        if (y < 0) break;

        for (int x = xStart; x <= xEnd && x < LEVEL_W; ++x)
            g_level[y][x] = 1;
    }

    // Сплошные шипы в самом низу (наказание за падение)
    for (int x = 0; x < LEVEL_W; ++x)
        g_level[LEVEL_H - 1][x] = 2;

    // Немного земли в самом начале и в конце, чтобы было где стоять
    for (int x = 0; x < 8; ++x)
        g_level[LEVEL_H - 2][x] = 1;
    for (int x = LEVEL_W - 8; x < LEVEL_W; ++x)
        g_level[LEVEL_H - 2][x] = 1;

    // Декор на заднем плане
    for (int x = 15; x < 20; ++x) g_level[LEVEL_H - 6][x] = 3;
    for (int x = 40; x < 45; ++x) g_level[LEVEL_H - 8][x] = 3;
    for (int x = 70; x < 75; ++x) g_level[LEVEL_H - 10][x] = 3;
}

// Получение значения тайла
int GetTile(int tx, int ty)
{
    if (tx < 0 || tx >= LEVEL_W || ty < 0 || ty >= LEVEL_H) return 0;
    return g_level[ty][tx];
}

// Является ли тайл твёрдым
bool IsSolid(int tx, int ty)
{
    int t = GetTile(tx, ty);
    return (t == 1); // земля
}

// Рисуем прямоугольник в экранных координатах
void DrawTileRect(int x0, int y0, int w, int h, unsigned int color)
{
    for (int y = y0; y < y0 + h; ++y)
    {
        for (int x = x0; x < x0 + w; ++x)
        {
            PutPixel(x, y, color);
        }
    }
}

// Отрисовка уровня с учётом камеры
void DrawLevel()
{
    unsigned int colGround = MakeColor(100, 100, 255); // синий
    unsigned int colSpike = MakeColor(255, 50, 50);   // красный
    unsigned int colDecor = MakeColor(50, 200, 50);   // зелёный

    for (int ty = 0; ty < LEVEL_H; ++ty)
    {
        for (int tx = 0; tx < LEVEL_W; ++tx)
        {
            int tile = g_level[ty][tx];
            if (tile == 0) continue;

            int worldX = tx * TILE_SIZE;
            int worldY = ty * TILE_SIZE;
            int sx = worldX - (int)g_camX;
            int sy = worldY; // по Y камера не двигается

            // если совсем вне экрана — пропускаем
            if (sx + TILE_SIZE < 0 || sx >= SCREEN_W) continue;
            if (sy + TILE_SIZE < 0 || sy >= SCREEN_H) continue;

            unsigned int col = colGround;
            if (tile == 2) col = colSpike;
            if (tile == 3) col = colDecor;

            DrawTileRect(sx, sy, TILE_SIZE, TILE_SIZE, col);
        }
    }
}

// Отрисовка игрока
void DrawPlayer()
{
    const int pw = 24;
    const int ph = 32;
    unsigned int col = MakeColor(255, 200, 50); // жёлто-оранжевый

    int sx = (int)g_player.x - (int)g_camX;
    int sy = (int)g_player.y;

    DrawTileRect(sx, sy, pw, ph, col);
}

// Враги
void DrawEnemies()
{
    const int ew = 24;
    const int eh = 32;
    unsigned int col = MakeColor(200, 50, 255); // фиолетовый

    for (int i = 0; i < g_enemyCount; ++i)
    {
        if (!g_enemies[i].alive) continue;

        int sx = (int)g_enemies[i].x - (int)g_camX;
        int sy = (int)g_enemies[i].y;

        if (sx + ew < 0 || sx >= SCREEN_W) continue;
        if (sy + eh < 0 || sy >= SCREEN_H) continue;

        DrawTileRect(sx, sy, ew, eh, col);
    }
}

// Пули
void DrawBullets()
{
    const int bw = 8;
    const int bh = 4;
    unsigned int col = MakeColor(255, 255, 255); // белый

    for (int i = 0; i < MAX_BULLETS; ++i)
    {
        if (!g_bullets[i].active) continue;

        int sx = (int)g_bullets[i].x - (int)g_camX;
        int sy = (int)g_bullets[i].y;

        if (sx + bw < 0 || sx >= SCREEN_W) continue;
        if (sy + bh < 0 || sy >= SCREEN_H) continue;

        DrawTileRect(sx, sy, bw, bh, col);
    }
}

// Прямоугольники пересекаются?
bool RectsOverlap(float x1, float y1, int w1, int h1,
    float x2, float y2, int w2, int h2)
{
    if (x1 > x2 + w2) return false;
    if (x2 > x1 + w1) return false;
    if (y1 > y2 + h2) return false;
    if (y2 > y1 + h1) return false;
    return true;
}

// Сброс игрока при смерти
void ResetPlayer()
{
    g_player.x = 50.0f;
    g_player.y = 100.0f;
    g_player.vx = 0.0f;
    g_player.vy = 0.0f;
    g_player.onGround = false;
    g_camX = 0.0f;
}

// Проверка, стоим ли на шипах / внутри шипов
void CheckPlayerHazards()
{
    const int pw = 24;
    const int ph = 32;

    int left = (int)g_player.x;
    int right = (int)g_player.x + pw - 1;
    int top = (int)g_player.y;
    int bottom = (int)g_player.y + ph - 1;

    int tx0 = left / TILE_SIZE;
    int tx1 = right / TILE_SIZE;
    int ty0 = top / TILE_SIZE;
    int ty1 = bottom / TILE_SIZE;

    for (int ty = ty0; ty <= ty1; ++ty)
    {
        for (int tx = tx0; tx <= tx1; ++tx)
        {
            int tile = GetTile(tx, ty);
            if (tile == 2) // шипы
            {
                ResetPlayer();
                return;
            }
        }
    }
}

// Движение и коллизии игрока
void MovePlayer(float dt)
{
    const int pw = 24;
    const int ph = 32;

    // Обработка ввода
    g_player.vx = 0.0f;

    if (GetAsyncKeyState(VK_LEFT) & 0x8000)
    {
        g_player.vx = -MOVE_SPEED;
        g_playerDir = -1;
    }
    if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
    {
        g_player.vx = MOVE_SPEED;
        g_playerDir = 1;
    }

    // Прыжок
    if ((GetAsyncKeyState(VK_SPACE) & 0x8000) && g_player.onGround)
    {
        g_player.vy = JUMP_SPEED;
        g_player.onGround = false;
    }

    // Гравитация
    g_player.vy += GRAVITY * dt;

    // --- движение по X с проверкой столкновений ---
    float newX = g_player.x + g_player.vx * dt;
    float newY = g_player.y;

    if (g_player.vx > 0) // вправо
    {
        int txRight = (int)((newX + pw - 1) / TILE_SIZE);
        int tyTop = (int)(newY / TILE_SIZE);
        int tyBottom = (int)((newY + ph - 1) / TILE_SIZE);

        bool collide = false;
        for (int ty = tyTop; ty <= tyBottom; ++ty)
        {
            if (IsSolid(txRight, ty))
            {
                collide = true;
                break;
            }
        }

        if (collide)
        {
            newX = txRight * TILE_SIZE - pw;
            g_player.vx = 0;
        }
    }
    else if (g_player.vx < 0) // влево
    {
        int txLeft = (int)(newX / TILE_SIZE);
        int tyTop = (int)(newY / TILE_SIZE);
        int tyBottom = (int)((newY + ph - 1) / TILE_SIZE);

        bool collide = false;
        for (int ty = tyTop; ty <= tyBottom; ++ty)
        {
            if (IsSolid(txLeft, ty))
            {
                collide = true;
                break;
            }
        }

        if (collide)
        {
            newX = (txLeft + 1) * TILE_SIZE;
            g_player.vx = 0;
        }
    }

    g_player.x = newX;

    // --- движение по Y с проверкой столкновений ---
    newY = g_player.y + g_player.vy * dt;
    g_player.onGround = false;

    if (g_player.vy > 0) // падение вниз
    {
        int tyBottom = (int)((newY + ph - 1) / TILE_SIZE);
        int txLeft = (int)(g_player.x / TILE_SIZE);
        int txRight = (int)((g_player.x + pw - 1) / TILE_SIZE);

        bool collide = false;
        for (int tx = txLeft; tx <= txRight; ++tx)
        {
            if (IsSolid(tx, tyBottom))
            {
                collide = true;
                break;
            }
        }

        if (collide)
        {
            newY = tyBottom * TILE_SIZE - ph;
            g_player.vy = 0;
            g_player.onGround = true;
        }
    }
    else if (g_player.vy < 0) // прыжок вверх
    {
        int tyTop = (int)(newY / TILE_SIZE);
        int txLeft = (int)(g_player.x / TILE_SIZE);
        int txRight = (int)((g_player.x + pw - 1) / TILE_SIZE);

        bool collide = false;
        for (int tx = txLeft; tx <= txRight; ++tx)
        {
            if (IsSolid(tx, tyTop))
            {
                collide = true;
                break;
            }
        }

        if (collide)
        {
            newY = (tyTop + 1) * TILE_SIZE;
            g_player.vy = 0;
        }
    }

    g_player.y = newY;

    // Простейшие вертикальные границы экрана
    if (g_player.y + ph >= SCREEN_H)
    {
        g_player.y = (float)(SCREEN_H - ph);
        g_player.vy = 0;
        g_player.onGround = true;
    }

    // Проверка шипов
    CheckPlayerHazards();
}

// Движение одного врага (гравитация + простые коллизии)
void MoveEnemy(Enemy& e, float dt)
{
    if (!e.alive) return;

    const int ew = 24;
    const int eh = 32;

    // гравитация
    e.vy += GRAVITY * dt;

    // движение по X
    float newX = e.x + e.vx * dt;
    float newY = e.y;

    if (e.vx > 0) // вправо
    {
        int txRight = (int)((newX + ew - 1) / TILE_SIZE);
        int tyTop = (int)(newY / TILE_SIZE);
        int tyBottom = (int)((newY + eh - 1) / TILE_SIZE);

        bool collide = false;
        for (int ty = tyTop; ty <= tyBottom; ++ty)
        {
            if (IsSolid(txRight, ty))
            {
                collide = true;
                break;
            }
        }

        if (collide)
        {
            newX = txRight * TILE_SIZE - ew;
            e.vx = -e.vx; // разворот при ударе
        }
    }
    else if (e.vx < 0) // влево
    {
        int txLeft = (int)(newX / TILE_SIZE);
        int tyTop = (int)(newY / TILE_SIZE);
        int tyBottom = (int)((newY + eh - 1) / TILE_SIZE);

        bool collide = false;
        for (int ty = tyTop; ty <= tyBottom; ++ty)
        {
            if (IsSolid(txLeft, ty))
            {
                collide = true;
                break;
            }
        }

        if (collide)
        {
            newX = (txLeft + 1) * TILE_SIZE;
            e.vx = -e.vx; // разворот
        }
    }

    e.x = newX;

    // движение по Y
    newY = e.y + e.vy * dt;

    if (e.vy > 0) // падение вниз
    {
        int tyBottom = (int)((newY + eh - 1) / TILE_SIZE);
        int txLeft = (int)(e.x / TILE_SIZE);
        int txRight = (int)((e.x + ew - 1) / TILE_SIZE);

        bool collide = false;
        for (int tx = txLeft; tx <= txRight; ++tx)
        {
            if (IsSolid(tx, tyBottom))
            {
                collide = true;
                break;
            }
        }

        if (collide)
        {
            newY = tyBottom * TILE_SIZE - eh;
            e.vy = 0;
        }
    }
    else if (e.vy < 0) // вверх
    {
        int tyTop = (int)(newY / TILE_SIZE);
        int txLeft = (int)(e.x / TILE_SIZE);
        int txRight = (int)((e.x + ew - 1) / TILE_SIZE);

        bool collide = false;
        for (int tx = txLeft; tx <= txRight; ++tx)
        {
            if (IsSolid(tx, tyTop))
            {
                collide = true;
                break;
            }
        }

        if (collide)
        {
            newY = (tyTop + 1) * TILE_SIZE;
            e.vy = 0;
        }
    }

    e.y = newY;
}

// Спавн врагов на некоторых платформах
void InitEnemies()
{
    g_enemyCount = 0;

    // несколько врагов
    // враг 0
    g_enemies[g_enemyCount].x = 12 * TILE_SIZE;
    g_enemies[g_enemyCount].y = 11 * TILE_SIZE;
    g_enemies[g_enemyCount].vx = -60.0f;
    g_enemies[g_enemyCount].vy = 0.0f;
    g_enemies[g_enemyCount].alive = true;
    g_enemyCount++;

    // враг 1
    g_enemies[g_enemyCount].x = 27 * TILE_SIZE;
    g_enemies[g_enemyCount].y = 9 * TILE_SIZE;
    g_enemies[g_enemyCount].vx = 60.0f;
    g_enemies[g_enemyCount].vy = 0.0f;
    g_enemies[g_enemyCount].alive = true;
    g_enemyCount++;

    // враг 2
    g_enemies[g_enemyCount].x = 65 * TILE_SIZE;
    g_enemies[g_enemyCount].y = 10 * TILE_SIZE;
    g_enemies[g_enemyCount].vx = -80.0f;
    g_enemies[g_enemyCount].vy = 0.0f;
    g_enemies[g_enemyCount].alive = true;
    g_enemyCount++;
}

// Обновление врагов и проверка столкновения с игроком
void UpdateEnemies(float dt)
{
    const int pw = 24;
    const int ph = 32;
    const int ew = 24;
    const int eh = 32;

    for (int i = 0; i < g_enemyCount; ++i)
    {
        if (!g_enemies[i].alive) continue;

        MoveEnemy(g_enemies[i], dt);

        // проверка столкновения с игроком
        if (RectsOverlap(g_player.x, g_player.y, pw, ph,
            g_enemies[i].x, g_enemies[i].y, ew, eh))
        {
            ResetPlayer();
            return;
        }
    }
}

// Спавн пули
void SpawnBullet()
{
    // найдём свободный слот
    for (int i = 0; i < MAX_BULLETS; ++i)
    {
        if (!g_bullets[i].active)
        {
            g_bullets[i].active = true;
            g_bullets[i].vx = (g_playerDir >= 0 ? 400.0f : -400.0f);

            // начальная позиция пули — примерно из середины игрока
            const int pw = 24;
            const int ph = 32;
            g_bullets[i].x = g_player.x + pw / 2;
            g_bullets[i].y = g_player.y + ph / 2;
            break;
        }
    }
}

// Обновление пуль и проверка попаданий
void UpdateBullets(float dt)
{
    const int bw = 8;
    const int bh = 4;
    const int ew = 24;
    const int eh = 32;

    for (int i = 0; i < MAX_BULLETS; ++i)
    {
        if (!g_bullets[i].active) continue;

        g_bullets[i].x += g_bullets[i].vx * dt;

        // если пуля ушла далеко за границы уровня — отключаем
        if (g_bullets[i].x < 0 || g_bullets[i].x > LEVEL_W * TILE_SIZE)
        {
            g_bullets[i].active = false;
            continue;
        }

        // столкновение с твёрдым блоком
        int tx = (int)(g_bullets[i].x / TILE_SIZE);
        int ty = (int)(g_bullets[i].y / TILE_SIZE);
        if (IsSolid(tx, ty))
        {
            g_bullets[i].active = false;
            continue;
        }

        // попадание во врага
        for (int e = 0; e < g_enemyCount; ++e)
        {
            if (!g_enemies[e].alive) continue;

            if (RectsOverlap(g_bullets[i].x, g_bullets[i].y, bw, bh,
                g_enemies[e].x, g_enemies[e].y, ew, eh))
            {
                g_bullets[i].active = false;
                g_enemies[e].alive = false;
                break;
            }
        }
    }
}

// Обработка стрельбы (клавиша Z)
void HandleShooting(float dt)
{
    g_shootCooldown -= dt;
    if (g_shootCooldown < 0.0f) g_shootCooldown = 0.0f;

    // Z — 0x5A
    if ((GetAsyncKeyState(0x5A) & 0x8000) && g_shootCooldown <= 0.0f)
    {
        SpawnBullet();
        g_shootCooldown = 0.25f; // 4 выстрела в секунду максимум
    }
}

// Обновление камеры (центрируем на игроке)
void UpdateCamera()
{
    const int pw = 24;

    g_camX = g_player.x + pw / 2 - SCREEN_W / 2;

    if (g_camX < 0) g_camX = 0;

    float maxCamX = (float)(LEVEL_W * TILE_SIZE - SCREEN_W);
    if (g_camX > maxCamX) g_camX = maxCamX;
}

// Рендер кадра на окно
void PresentFrame(HDC hdc)
{
    StretchDIBits(
        hdc,
        0, 0, SCREEN_W, SCREEN_H,
        0, 0, SCREEN_W, SCREEN_H,
        g_pixels,
        &g_bmi,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY:
        g_running = false;
        PostQuitMessage(0);
        return 0;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            g_running = false;
            PostQuitMessage(0);
            return 0;
        }
        break;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

bool InitWindow(HINSTANCE hInstance, int nCmdShow)
{
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "SimplePlatformerClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClass(&wc))
        return false;

    RECT rc = { 0, 0, SCREEN_W, SCREEN_H };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    int winW = rc.right - rc.left;
    int winH = rc.bottom - rc.top;

    g_hWnd = CreateWindowEx(
        0,
        wc.lpszClassName,
        "Simple Platformer",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        winW, winH,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!g_hWnd)
        return false;

    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);
    return true;
}

bool InitGraphics()
{
    g_pixels = new unsigned int[SCREEN_W * SCREEN_H];
    if (!g_pixels) return false;

    ZeroMemory(&g_bmi, sizeof(g_bmi));
    g_bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    g_bmi.bmiHeader.biWidth = SCREEN_W;
    g_bmi.bmiHeader.biHeight = -SCREEN_H; // top-down
    g_bmi.bmiHeader.biPlanes = 1;
    g_bmi.bmiHeader.biBitCount = 32;
    g_bmi.bmiHeader.biCompression = BI_RGB;

    return true;
}

void InitGame()
{
    BuildLevel_3();
    ResetPlayer();
    InitEnemies();

    // обнулим пули
    for (int i = 0; i < MAX_BULLETS; ++i)
    {
        g_bullets[i].active = false;
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    if (!InitWindow(hInstance, nCmdShow))
        return 0;

    if (!InitGraphics())
        return 0;

    InitGame();


    InitFreeType("d:\\HelloCoder2025\\assets\\NotoSansJP-Regular.ttf", 18);
    PreloadAsciiGlyphs();
    LoadPNG("d:\\HelloCoder2025\\assets\\example.png", TestImage);
    
    DWORD prevTime = GetTickCount();

    MSG msg;
    while (g_running)
    {
        // обработка сообщений Windows
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                g_running = false;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // расчёт dt
        DWORD currTime = GetTickCount();
        float dt = (currTime - prevTime) / 1000.0f;
        if (dt > 0.05f) dt = 0.05f; // ограничим шаг
        prevTime = currTime;

        // логика
        MovePlayer(dt);
        UpdateEnemies(dt);
        HandleShooting(dt);
        UpdateBullets(dt);
        UpdateCamera();

        // рендер
        ClearScreen(MakeColor(50, 50, 80)); // фон
        DrawLevel();
        DrawEnemies();
        DrawBullets();
        DrawPlayer();

        DrawGlyph(0x2740, 200, 80, 255, 0, 0, 0.9f);

        DrawAsciiText("Hello, Skybound!", 20, 40, 255, 255, 255, 0.95f);
        //DrawPNG(TestImage, 100, 100, 2.0f, 0.8f);

        // вывод на экран
        HDC hdc = GetDC(g_hWnd);
        PresentFrame(hdc);
        ReleaseDC(g_hWnd, hdc);

        Sleep(1);
    }

    delete[] g_pixels;
    return 0;
}
