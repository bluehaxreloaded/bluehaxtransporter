#include "ui.hpp"
#include <stdarg.h>

int initUI() {
    if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)) {
        return -1;
    }
    if (!C2D_Init(C2D_DEFAULT_MAX_OBJECTS)) {
        return -2;
    }
    C2D_Prepare();
    return 0;
}

void exitUI() {
    C2D_Fini();
    C3D_Fini();
}

void initContext(DrawContext* ctx)
{
    ctx->top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    ctx->bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
}

void initColors(DrawContext* ctx)
{
    ctx->clrBlack = C2D_Color32(0x00, 0x00, 0x00, 0xFF);
    ctx->clrWhite = C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF);
    ctx->clrRed = C2D_Color32(0xFF, 0x00, 0x00, 0xFF);
    ctx->clrBlue = C2D_Color32(0x00, 0x00, 0xFF, 0xFF);
    ctx->clrGreen = C2D_Color32(0x00, 0xFF, 0x00, 0xFF);
    ctx->clrYellow = C2D_Color32(0xFF, 0xFF, 0x00, 0xFF);
    ctx->clrBgBright = C2D_Color32(0x25, 0x70, 0xC7, 0xFF);
    ctx->clrBgDark = C2D_Color32(0x10, 0x3C, 0x6F, 0xFF);
}

void drawText(float x, float y, float z, float scale, u32 color, u32 flags, const char* fmt, ...)
{
    va_list va_stuff;
    va_start(va_stuff, fmt);
    C2D_TextBuf buf=C2D_TextBufNew(4096);
    C2D_Text text;
    char sbuf[4096];
    vsnprintf(sbuf, 4095, fmt, va_stuff);
    C2D_TextParse(&text, buf, sbuf);
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, C2D_WithColor | flags, x, y, z, scale, scale, color);
    C2D_TextBufDelete(buf);
    va_end(va_stuff);
}

void drawTextCenter(float x, float z, float scale, u32 color, u32 flags, const char* fmt, ...)
{
    float textWidth, textHeight;
    va_list va_stuff;
    va_start(va_stuff, fmt);
    C2D_TextBuf buf=C2D_TextBufNew(4096);
    C2D_Text text;
    char sbuf[4096];
    vsnprintf(sbuf, 4095, fmt, va_stuff);
    C2D_TextParse(&text, buf, sbuf);
    C2D_TextOptimize(&text);
    C2D_TextGetDimensions(&text, scale, scale, &textWidth, &textHeight);
    C2D_DrawText(&text, C2D_WithColor | flags, x, (SCREEN_HEIGHT / 2.0f) - (textHeight / 2.0f), z, scale, scale, color);
    C2D_TextBufDelete(buf);
    va_end(va_stuff);
}