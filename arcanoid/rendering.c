#include "arcanoid.h"
#include <memory.h>
#include <math.h> 

void AG_PutPixel(AG_BackBuffer* p_renderBuffer, int x, int y, AG_Color color)
{
    if (p_renderBuffer == NULL
        || x < 0 || y < 0
        || x >= p_renderBuffer->Width
        || y >= p_renderBuffer->Height
        || p_renderBuffer->pPixels
        || color.A == 0)
    {
        return;
    }
    AG_Color* p_px = p_renderBuffer->pPixels + y * p_renderBuffer->Width + x;

    if (color.A == 255)
    {
        p_px->ARGB = color.ARGB;
    }
    else
    {
        int invA = 255 - color.A;
        color.A = 0xFF;
        color.B = (unsigned char)((color.B * color.A + p_px->B * invA) / 255);
        color.G = (unsigned char)((color.G * color.A + p_px->G * invA) / 255);
        color.R = (unsigned char)((color.R * color.A + p_px->R * invA) / 255);
    }
}

void AG_FillRectPx(AG_BackBuffer* p_renderBuffer, int x0, int y0, int x1, int y1, AG_Color color)
{
    if (p_renderBuffer == NULL) return;
    AG_ASSERT(p_renderBuffer->pPixels != NULL);

    if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
    if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }
    if (x0 >= p_renderBuffer->Width || y0 >= p_renderBuffer->Height || x1 < 0 || y1 < 0) return;
    if (x0 < 0) x0 = 0; if (y0 < 0) y0 = 0;
    if (x1 >= p_renderBuffer->Width)  x1 = p_renderBuffer->Width - 1;
    if (y1 >= p_renderBuffer->Height) y1 = p_renderBuffer->Height - 1;

    for (int y = y0; y <= y1; ++y)
    {
        AG_Color* row = (p_renderBuffer->pPixels + y * p_renderBuffer->Width);
        for (int x = x0; x <= x1; ++x) row[x] = color;
    }
}

void AG_FillCirclePx(AG_BackBuffer* p_renderBuffer, int cx, int cy, int radius, AG_Color color)
{
    if (p_renderBuffer == NULL)
    {
        return;
    }
    AG_ASSERT(p_renderBuffer->pPixels != NULL);

    if (radius <= 0) { AG_PutPixel(p_renderBuffer, cx, cy, color); return; }
    int r2 = radius * radius;
    for (int y = -radius; y <= radius; ++y)
    {
        int yy = cy + y;
        if (yy < 0 || yy >= p_renderBuffer->Height) continue;
        int dx = (int)(sqrt((double)(r2 - y * y)) + 0.5);
        int x0 = cx - dx, x1 = cx + dx;
        if (x1 < 0 || x0 >= p_renderBuffer->Width) continue;
        if (x0 < 0) x0 = 0; if (x1 >= p_renderBuffer->Width) x1 = p_renderBuffer->Width - 1;

        AG_Color* row = (p_renderBuffer->pPixels + yy * p_renderBuffer->Width);
        for (int x = x0; x <= x1; ++x) row[x] = color;
    }
}

void AG_StrokeRectPx(AG_BackBuffer* p_renderBuffer, int x0, int y0, int x1, int y1, AG_Color color)
{
    AG_FillRectPx(p_renderBuffer, x0, y0, x1, y0, color);
    AG_FillRectPx(p_renderBuffer, x0, y1, x1, y1, color);
    AG_FillRectPx(p_renderBuffer, x0, y0, x0, y1, color);
    AG_FillRectPx(p_renderBuffer, x1, y0, x1, y1, color);
}


void AG_BufferClear(AG_BackBuffer* p_renderBuffer, AG_Color color)
{
    AG_ASSERT(p_renderBuffer->pPixels != NULL);
    memset(p_renderBuffer->pPixels, 0, p_renderBuffer->Width * p_renderBuffer->Height * sizeof(p_renderBuffer->pPixels[0]));
    //if (p_renderBuffer == NULL)
    //{
    //    return;
    //}
    //color.A = 0xFF;
    //int count = p_renderBuffer->Width * p_renderBuffer->Height;
    //for (int i = 0; i < count; ++i)
    //{
    //    p_renderBuffer->pPixels[i].ARGB = color.ARGB;
    //}
}

int AG_CircleIntersectsRect(float cx, float cy, float r, float rx, float ry, float rw, float rh, float* nx, float* ny)
{
    float closestX = (cx < rx) ? rx : (cx > rx + rw ? rx + rw : cx);
    float closestY = (cy < ry) ? ry : (cy > ry + rh ? ry + rh : cy);
    float dx = cx - closestX;
    float dy = cy - closestY;
    float dist2 = dx * dx + dy * dy;
    if (dist2 <= r * r)
    {
        float len = sqrtf(dist2);
        if (len > 1e-4f) { *nx = dx / len; *ny = dy / len; }
        else
        {
            if (fabsf(dx) > fabsf(dy)) { *nx = (dx > 0) ? 1.f : -1.f; *ny = 0.f; }
            else { *nx = 0.f; *ny = (dy > 0) ? 1.f : -1.f; }
        }
        return 1;
    }
    return 0;
}

