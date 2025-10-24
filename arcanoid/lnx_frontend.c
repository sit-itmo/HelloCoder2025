#if !defined (WIN32)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "arcanoid.h"
#include "lnx_frontend.h"

typedef struct LNX_BackBuffer
{
    AG_BackBuffer* pBB;
    Display* dpy;
    XImage* img;
    Window win;
    GC gc;
} LNX_BackBuffer;

static LNX_BackBuffer g_LNX_RenderBuffer = { 0 };

unsigned long LNX_GetTickCount(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts); // time since system boot
    return (unsigned long)(ts.tv_sec) * 1000 + (ts.tv_nsec) / 1000000;
}

void LNX_WindowCreate(int w, int h, const char *p_caption)
{
    // X11 init
    g_LNX_RenderBuffer.dpy = XOpenDisplay(NULL);
    if (!g_LNX_RenderBuffer.dpy) {
        fprintf(stderr, "Error: cannot open X display\n");
        return 1;
    }
    int screen = DefaultScreen(g_LNX_RenderBuffer.dpy);
    Window root = RootWindow(g_LNX_RenderBuffer.dpy, screen);

    // Создаём окно
    XSetWindowAttributes attrs;
    attrs.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | StructureNotifyMask;
    g_LNX_RenderBuffer.win = XCreateWindow(
        g_LNX_RenderBuffer.dpy, root,
        0, 0, w, h,
        0,
        DefaultDepth(g_LNX_RenderBuffer.dpy, screen),
        InputOutput,
        DefaultVisual(g_LNX_RenderBuffer.dpy, screen),
        CWEventMask, &attrs
    );
    XStoreName(g_LNX_RenderBuffer.dpy, g_LNX_RenderBuffer.win, p_caption);
    XMapWindow(g_LNX_RenderBuffer.dpy, g_LNX_RenderBuffer.win);

    
    int bytes_per_pixel = 4;
    size_t img_size = (size_t)w * (size_t)h * (size_t)bytes_per_pixel;
    char* pixels = (char*)calloc(1, img_size);
    if (!pixels) {
        fprintf(stderr, "Error: cannot allocate framebuffer\n");
        return 1;
    }

    g_LNX_RenderBuffer.pBB = AG_GetBackBuffer();
    AG_ASSERT(g_LNX_RenderBuffer.pBB != NULL);
    g_LNX_RenderBuffer.pBB->pPixels = (AG_Color*)pixels;
    g_LNX_RenderBuffer.pBB->Width = w;
    g_LNX_RenderBuffer.pBB->Height = h;


    g_LNX_RenderBuffer.img = XCreateImage(
        g_LNX_RenderBuffer.dpy, DefaultVisual(g_LNX_RenderBuffer.dpy, screen),
        24,               
        ZPixmap,
        0,                
        pixels,           
        w,
        h,
        32,               
        w * bytes_per_pixel 
    );
    if (!g_LNX_RenderBuffer.img) {
        fprintf(stderr, "Error: XCreateImage failed\n");
        free(pixels);
        return 1;
    }

    g_LNX_RenderBuffer.gc = XCreateGC(g_LNX_RenderBuffer.dpy, g_LNX_RenderBuffer.win, 0, NULL);

}

void LNX_MainLoop(void)
{
    AG_UpdateGameArea(1);
    int running = 1;
    while (running) {
        while (XPending(g_LNX_RenderBuffer.dpy)) {
            XEvent ev;
            XNextEvent(g_LNX_RenderBuffer.dpy, &ev);
            switch (ev.type) {
            case Expose:
                break;
            case ConfigureNotify:
                
                break;
            case KeyPress: 
            {
                KeySym ks = XLookupKeysym(&ev.xkey, 0);
                
                switch (ks) {
                case XK_Left:     AG_KeyAction(AG_Key_Left, 1); break;
                case XK_Right:    AG_KeyAction(AG_Key_Right, 1); break;
                case XK_space:    AG_KeyAction(AG_Key_Space, 1); break;
                case XK_A:
                case XK_a:        AG_KeyAction(AG_Key_A, 1); break;
                case XK_D:
                case XK_d:        AG_KeyAction(AG_Key_D, 1); break;
                case XK_R:
                case XK_r:        AG_KeyAction(AG_Key_Reset, 1); break;
                default: break;
                }
                
                // По Esc также можно выйти на уровне оболочки:
                if (ks == XK_q) { running = 0; }
            } break;
            case KeyRelease: {
                // В простом варианте игнорируем auto-repeat;
                // при необходимости можно различать по XEvents
                KeySym ks = XLookupKeysym(&ev.xkey, 0);
                switch (ks) {
                case XK_Left:     AG_KeyAction(AG_Key_Left, 0); break;
                case XK_Right:    AG_KeyAction(AG_Key_Right, 0); break;
                case XK_space:    AG_KeyAction(AG_Key_Space, 0); break;
                case XK_A:
                case XK_a:        AG_KeyAction(AG_Key_A, 0); break;
                case XK_D:
                case XK_d:        AG_KeyAction(AG_Key_D, 0); break;
                case XK_R:
                case XK_r:        AG_KeyAction(AG_Key_Reset, 0); break;
                default: break;
                }
            } break;
            case ClientMessage:
            default:
                break;
            }
        }

        AG_GameIteration(LNX_GetTickCount());
        AG_DrawWorld();
        XPutImage(g_LNX_RenderBuffer.dpy, g_LNX_RenderBuffer.win, g_LNX_RenderBuffer.gc, g_LNX_RenderBuffer.img, 0, 0, 0, 0, g_LNX_RenderBuffer.pBB->Width, g_LNX_RenderBuffer.pBB->Height);
        XFlush(g_LNX_RenderBuffer.dpy);
        //usleep(10000);
    }

    XDestroyImage(g_LNX_RenderBuffer.img);
    XFreeGC(g_LNX_RenderBuffer.dpy, g_LNX_RenderBuffer.gc);
    XDestroyWindow(g_LNX_RenderBuffer.dpy, g_LNX_RenderBuffer.win);
    XCloseDisplay(g_LNX_RenderBuffer.dpy);
    return 0;
}
#endif
