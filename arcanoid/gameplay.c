#include "arcanoid.h"
#include <math.h>

#define AG_MAX_ROWS 10
#define AG_MAX_COLS 14

AG_Brick g_bricks[AG_MAX_ROWS * AG_MAX_COLS];
int g_rows = 8, g_cols = 12;

float g_paddleX = 0, g_paddleY = 0, g_paddleW = 100, g_paddleH = 16;
float g_ballX = 0, g_ballY = 0, g_ballVX = 0, g_ballVY = 0, g_ballR = 8;
int   g_score = 0, g_lives = 3, g_level = 1;
int  g_running = 0;
int g_keyLeft = 0, g_keyRight = 0, g_keyA = 0, g_keyD = 0;
unsigned long g_prev_ticks = 0;

AG_Rect  g_playArea = { 0 };
static AG_BackBuffer g_AG_RenderBuffer;

AG_BackBuffer* AG_GetBackBuffer()
{
    return &g_AG_RenderBuffer;
}

void AG_LayoutPlayArea(void) 
{
    int margin = 20;
    
    AG_ASSERT((g_AG_RenderBuffer.Width * g_AG_RenderBuffer.Height) > 100);

    g_playArea.left = margin;
    g_playArea.top = margin + 40;  // место под HUD
    g_playArea.right = g_AG_RenderBuffer.Width - margin;
    g_playArea.bottom = g_AG_RenderBuffer.Height - margin;

    AG_ASSERT(((g_playArea.right - g_playArea.left) * (g_playArea.bottom - g_playArea.top)) > 100);
}

void AG_ResetPaddle(void)
{
    AG_ASSERT(((g_playArea.right - g_playArea.left) * (g_playArea.bottom - g_playArea.top)) > 100);

    g_paddleW = (float)((g_playArea.right - g_playArea.left) / 6);
    if (g_paddleW < 60) g_paddleW = 60;
    g_paddleH = 16.0f;
    g_paddleX = (float)((g_playArea.left + g_playArea.right - (int)g_paddleW) / 2);
    g_paddleY = (float)(g_playArea.bottom - g_paddleH - 8);
}

void AG_ResetBall(int attach)
{
    AG_ASSERT(((g_playArea.right - g_playArea.left) * (g_playArea.bottom - g_playArea.top)) > 100);

    g_ballR = 8.0f;
    g_ballX = g_paddleX + g_paddleW * 0.5f;
    g_ballY = g_paddleY - g_ballR - 2.0f;
    float speed = 6.0f + (float)g_level * 0.8f;
    float angle = (float)(-M_PI * 0.35); // немного влево вверх
    g_ballVX = cosf(angle) * speed;
    g_ballVY = sinf(angle) * speed;
    if (attach) g_running = 0;
}

void AG_BuildBricks(void)
{
    AG_ASSERT(((g_playArea.right - g_playArea.left) * (g_playArea.bottom - g_playArea.top)) > 100);
    
    int areaW = g_playArea.right - g_playArea.left;
    int areaH = g_playArea.bottom - g_playArea.top;

    g_cols = (areaW >= 800) ? 12 : (areaW >= 600 ? 10 : 8);
    if (g_cols > AG_MAX_COLS) g_cols = AG_MAX_COLS;
    g_rows = (areaH >= 600) ? 9 : 7;
    if (g_rows > AG_MAX_ROWS) g_rows = AG_MAX_ROWS;

    float cellW = (float)areaW / g_cols;
    float cellH = (float)areaH / (g_rows + 8); // верхн€€ часть под кирпичи

    for (int r = 0; r < g_rows; ++r)
    {
        for (int c = 0; c < g_cols; ++c)
        {
            AG_Brick* b = &g_bricks[r * g_cols + c];
            float pad = 3.f;
            b->x = g_playArea.left + c * cellW + pad;
            b->y = g_playArea.top + r * cellH + pad;
            b->w = cellW - pad * 2;
            b->h = cellH - pad * 2;
            b->alive = 1;
            b->hp = 1;
            AG_Color color;
            color.A = 0xFF;
            color.R = (120 + (r * 15) % 135);
            color.G = (80 + (c * 13) % 150);
            color.B = (180 - (r * 10) % 100);
            b->color.ARGB = color.ARGB;
        }
    }
}

void AG_NewGame(void)
{
    AG_LayoutPlayArea();
    AG_ResetPaddle();
    AG_BuildBricks();
    g_score = 0;
    g_lives = 3;
    g_level = 1;
    AG_ResetBall(1);
}

void NextLevel(void)
{
    g_level++;
    AG_BuildBricks();
    AG_ResetPaddle();
    AG_ResetBall(1);
}

void AG_DrawWorld(void)
{
    AG_Color color;
    color.ARGB = 0xFF101014;
    AG_BufferClear(&g_AG_RenderBuffer, color);

    AG_ASSERT(((g_playArea.right - g_playArea.left) * (g_playArea.bottom - g_playArea.top)) > 100);

    color.ARGB = 0xFF46465A;
    AG_StrokeRectPx(&g_AG_RenderBuffer, g_playArea.left, g_playArea.top, g_playArea.right, g_playArea.bottom, color);

    for (int r = 0; r < g_rows; ++r)
    {
        for (int c = 0; c < g_cols; ++c)
        {
            AG_Brick* b = &g_bricks[r * g_cols + c];
            if (!b->alive) continue;
            AG_FillRectPx(&g_AG_RenderBuffer, (int)b->x, (int)b->y, (int)(b->x + b->w), (int)(b->y + b->h), b->color);
            color.ARGB = 0xFF000000;
            AG_StrokeRectPx(&g_AG_RenderBuffer, (int)b->x, (int)b->y, (int)(b->x + b->w), (int)(b->y + b->h), color);
        }
    }

    color.ARGB = 0xFFC8D2F0;
    AG_FillRectPx(&g_AG_RenderBuffer, (int)g_paddleX, (int)g_paddleY, (int)(g_paddleX + g_paddleW), (int)(g_paddleY + g_paddleH), color);
    color.ARGB = 0xFFFFB43C;
    AG_FillCirclePx(&g_AG_RenderBuffer, (int)roundf(g_ballX), (int)roundf(g_ballY), (int)g_ballR, color);
}

void AG_ClampPaddle(void)
{
    AG_ASSERT(((g_playArea.right - g_playArea.left) * (g_playArea.bottom - g_playArea.top)) > 100);

    float minX = (float)g_playArea.left + 2;
    float maxX = (float)g_playArea.right - g_paddleW - 2;
    if (g_paddleX < minX) g_paddleX = minX;
    if (g_paddleX > maxX) g_paddleX = maxX;
}

void AG_UpdateBall(unsigned long ticks)
{
    AG_ASSERT(((g_playArea.right - g_playArea.left) * (g_playArea.bottom - g_playArea.top)) > 100);

    if (!g_running)
    {
        g_ballX = g_paddleX + g_paddleW * 0.5f;
        g_ballY = g_paddleY - g_ballR - 2.0f;
        return;
    }

    if (g_prev_ticks == 0)
    {
        g_prev_ticks = ticks;
    }
    float nextX = g_ballX + g_ballVX / 20.0f * ((float)(ticks - g_prev_ticks));
    float nextY = g_ballY + g_ballVY / 20.0f * ((float)(ticks - g_prev_ticks));

    g_prev_ticks = ticks;

    if (nextX - g_ballR < g_playArea.left) { nextX = g_playArea.left + g_ballR; g_ballVX = -g_ballVX; }
    if (nextX + g_ballR > g_playArea.right) { nextX = g_playArea.right - g_ballR; g_ballVX = -g_ballVX; }
    if (nextY - g_ballR < g_playArea.top) { nextY = g_playArea.top + g_ballR; g_ballVY = -g_ballVY; }

    if (nextY - g_ballR > g_playArea.bottom)
    {
        if (--g_lives < 0)
        { // game over
            g_lives = 3; g_score = 0; g_level = 1; AG_BuildBricks();
        }
        AG_ResetBall(1);
        return;
    }

    float nx = 0, ny = 0;
    if (AG_CircleIntersectsRect(nextX, nextY, g_ballR, g_paddleX, g_paddleY, g_paddleW, g_paddleH, &nx, &ny))
    {
        float hit = (nextX - (g_paddleX + g_paddleW * 0.5f)) / (g_paddleW * 0.5f); // -1..1
        if (hit < -1) hit = -1; if (hit > 1) hit = 1;
        float speed = sqrtf(g_ballVX * g_ballVX + g_ballVY * g_ballVY);
        float angle = (float)(-M_PI * 0.25 - hit * (M_PI * 0.35));
        g_ballVX = cosf(angle) * speed;
        g_ballVY = sinf(angle) * speed;
        nextY = g_paddleY - g_ballR - 0.1f;
    }
    else
    {
        int aliveCount = 0;
        for (int r = 0; r < g_rows; ++r)
        {
            for (int c = 0; c < g_cols; ++c)
            {
                AG_Brick* b = &g_bricks[r * g_cols + c];
                if (!b->alive) continue;
                aliveCount++;
                if (AG_CircleIntersectsRect(nextX, nextY, g_ballR, b->x, b->y, b->w, b->h, &nx, &ny))
                {
                    AG_WARNING("HIT the block %d x %d!", c, r);
                    float dot = g_ballVX * nx + g_ballVY * ny;
                    g_ballVX -= 2 * dot * nx;
                    g_ballVY -= 2 * dot * ny;
                    g_score += 10;
                    if (--b->hp <= 0)
                    {
                        b->alive = 0;
                        aliveCount--;
                    }
                    nextX += nx * 1.2f;
                    nextY += ny * 1.2f;
                    goto bricks_done;
                }
            }
        }
    bricks_done:
        if (aliveCount == 0)
        {
            NextLevel();
        }
    }

    g_ballX = nextX; g_ballY = nextY;
}

void AG_UpdatePaddle(void)
{
    float move = 0.f;
    if (g_keyLeft || g_keyA)  move -= 1.f;
    if (g_keyRight || g_keyD) move += 1.f;
    float speed = 9.0f;
    g_paddleX += move * speed;
    AG_ClampPaddle();
}

void AG_GameIteration(unsigned long ticks)
{
    AG_UpdatePaddle();
    AG_UpdateBall(ticks);
}


void AG_UpdateGameArea(int resetBall)
{
    AG_LayoutPlayArea();
    AG_ResetPaddle();
    AG_BuildBricks();
    if (resetBall)
    {
        AG_ResetBall(1);
    }
}

void AG_KeyAction(enum AG_Key key, int down)
{
    switch (key) {
    case AG_Key_Left:   g_keyLeft = down; break;
    case AG_Key_Right:  g_keyRight = down; break;
    case AG_Key_A: g_keyA = down; break;
    case AG_Key_D: g_keyD = down; break;
    case AG_Key_Space:  
        if (down)
        {
            g_running = !g_running;
        }
        break;
    case AG_Key_Reset: AG_NewGame(); break;
    }
}

void AG_MouseAction(int mouseX, int mouseY, int btns, int action)
{

}

