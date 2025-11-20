#include <windows.h>

//
// Простые настройки игры
//
const int SCREEN_W = 800;
const int SCREEN_H = 600;

const int TILE_SIZE = 32;
const int LEVEL_W = 25;
const int LEVEL_H = 18;

const float GRAVITY = 900.0f;  // пикселей/сек^2
const float MOVE_SPEED = 200.0f; // пикселей/сек
const float JUMP_SPEED = -450.0f;

HWND g_hWnd = NULL;
bool g_running = true;

// буфер кадра (ARGB, но используем как 32-битный int)
unsigned int* g_pixels = 0;
BITMAPINFO g_bmi;

struct Player
{
    float x;
    float y;
    float vx;
    float vy;
    bool onGround;
};

Player g_player;

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

// Очистка экрана
void ClearScreen(unsigned int color)
{
    int count = SCREEN_W * SCREEN_H;
    for (int i = 0; i < count; i++)
        g_pixels[i] = color;
}

// Простой уровень: 0 - пусто, 1 - платформа
int g_level[LEVEL_H][LEVEL_W] =
{
    // 0..24 (25 столбцов)
    // Верхние строки пустые
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},

    // Пара висящих платформ
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},

    // ещё пару строк пустых
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},

    // Низ — сплошная платформа
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
};

// Проверка, является ли тайл твёрдым
bool IsSolid(int tx, int ty)
{
    if (tx < 0 || tx >= LEVEL_W || ty < 0 || ty >= LEVEL_H) return false;
    return g_level[ty][tx] != 0;
}

// Рисуем прямоугольник в тайловых координатах
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

// Отрисовка уровня
void DrawLevel()
{
    unsigned int tileColor = MakeColor(100, 100, 255); // синий
    for (int ty = 0; ty < LEVEL_H; ++ty)
    {
        for (int tx = 0; tx < LEVEL_W; ++tx)
        {
            if (g_level[ty][tx] != 0)
            {
                int x = tx * TILE_SIZE;
                int y = ty * TILE_SIZE;
                DrawTileRect(x, y, TILE_SIZE, TILE_SIZE, tileColor);
            }
        }
    }
}

// Отрисовка игрока (прямоугольник)
void DrawPlayer()
{
    int pw = 24;
    int ph = 32;
    unsigned int col = MakeColor(255, 200, 50); // жёлто-оранжевый

    int x0 = (int)g_player.x;
    int y0 = (int)g_player.y;

    DrawTileRect(x0, y0, pw, ph, col);
}

// Простая обработка столкновений по осям
void MovePlayer(float dt)
{
    int pw = 24;
    int ph = 32;

    // Обработка ввода
    g_player.vx = 0.0f;

    if (GetAsyncKeyState(VK_LEFT) & 0x8000)
    {
        g_player.vx = -MOVE_SPEED;
    }
    if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
    {
        g_player.vx = MOVE_SPEED;
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

    // Горизонтальные столкновения
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
            newX = txRight * TILE_SIZE - pw; // упираемся в блок
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
            newX = (txLeft + 1) * TILE_SIZE; // упираемся
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

    // Простейшие границы экрана
    if (g_player.x < 0) g_player.x = 0;
    if (g_player.x + pw >= SCREEN_W) g_player.x = (float)(SCREEN_W - pw);
    if (g_player.y + ph >= SCREEN_H)
    {
        g_player.y = (float)(SCREEN_H - ph);
        g_player.vy = 0;
        g_player.onGround = true;
    }
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
    g_bmi.bmiHeader.biHeight = -SCREEN_H; // отрицательная высота -> верхний левый (top-down)
    g_bmi.bmiHeader.biPlanes = 1;
    g_bmi.bmiHeader.biBitCount = 32;
    g_bmi.bmiHeader.biCompression = BI_RGB;

    return true;
}

void InitGame()
{
    g_player.x = 50.0f;
    g_player.y = 100.0f;
    g_player.vx = 0.0f;
    g_player.vy = 0.0f;
    g_player.onGround = false;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    if (!InitWindow(hInstance, nCmdShow))
        return 0;

    if (!InitGraphics())
        return 0;

    InitGame();

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

        // обновление логики
        MovePlayer(dt);

        // рендер в буфер
        ClearScreen(MakeColor(50, 50, 80)); // фон
        DrawLevel();
        DrawPlayer();

        // вывод на экран
        HDC hdc = GetDC(g_hWnd);
        PresentFrame(hdc);
        ReleaseDC(g_hWnd, hdc);

        // можно немного "подтормозить", чтобы снизить нагрузку
        Sleep(1);
    }

    delete[] g_pixels;
    return 0;
}
