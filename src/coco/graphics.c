/*
  Graphics functionality
*/

#include "hires.h"
#include <peekpoke.h>
#include <string.h>
#include "../platform-specific/graphics.h"
#include "../platform-specific/sound.h"
#include "../misc.h"
#include <coco.h>

extern uint8_t charset[];
#define OFFSET_Y 2

#define ROP_CPY 0xff

// Mode 4
#define ROP_BLUE 0b10101010
#define ROP_YELLOW 0b01010101
#define BOX_SIDE 0b111100

#define COLOR_MODE_COCO3_RGB 1
#define COLOR_MODE_COCO3_COMPOSITE 2

#define LEGEND_X 24

// Defined in this file
void drawTextAltAt(uint8_t x, uint8_t y, const char *s);
void drawTextAt(uint8_t x, uint8_t y, const char *s);
void drawShipInternal(uint8_t *dest, uint8_t size, uint8_t delta);

extern char lastKey;
extern uint8_t background;
static uint8_t fieldX = 0;
int8_t highlightX = -1;
bool inBorderScreen = false;
uint8_t box_color = 0xff;
uint16_t quadrant_offset[] = {
    256U * 12 + 5 + 64,
    256U * 1 + 5 + 64,
    256U * 1 + 17 + 64,
    256U * 12 + 17 + 64};

uint16_t legendShipOffset[] = {2, 1, 0, 256U * 5, 256U * 6 + 1};

void updateColors()
{
    if (prefs.colorMode == COLOR_MODE_COCO3_RGB)
    {
        rgb();
        paletteRGB(1, 3, 3, 3); // White
        paletteRGB(2, 0, 0, 2); // Blue
        paletteRGB(3, 2, 0, 0); // Red
    }
    else if (prefs.colorMode == COLOR_MODE_COCO3_COMPOSITE)
    {
        cmp();
        palette(1, 63); // White
        palette(2, 11); // Blue
        palette(3, 22); // Red
    }
}

uint8_t cycleNextColor()
{
    if (!prefs.colorMode)
        return 0; // Coco3 is not enabled

    ++prefs.colorMode;
    if (prefs.colorMode > 2)
        prefs.colorMode = 1;

    updateColors();
    return prefs.colorMode;
}

void rgbOrComposite()
{
    if (!isCoCo3)
        return; // not a coco3, we can't change palettes anyway.

    while (!prefs.colorMode)
    {
        drawTextAltAt(8, 96, "R-GB or c-composite");
        switch (cgetc())
        {
        case 'R':
        case 'r':
            prefs.colorMode = COLOR_MODE_COCO3_RGB;
            break;
        case 'C':
        case 'c':
            prefs.colorMode = COLOR_MODE_COCO3_COMPOSITE;
            break;
        }
    }

    updateColors();
}

void initGraphics()
{
    initCoCoSupport();

    pmode(3, SCREEN);
    pcls(0);
    screen(1, 0);

    rgbOrComposite();
}

bool saveScreenBuffer()
{
    // No room on CoCo 32K for second page
    return false;
}

void restoreScreenBuffer()
{
    // No-op on CoCo
}

void drawPlayerName(uint8_t player, const char *name, bool active)
{
    uint8_t x, y;
    uint16_t pos = fieldX + quadrant_offset[player];

    x = (uint8_t)(pos % 32 + 1);
    y = (uint8_t)(pos / 32 - 9);

    if (player == 0 || player == 3)
    {
        y += 89;
    }

    background = ROP_YELLOW;
    if (active)
    {
        hires_putc(x - 1, y, ROP_CPY, 0x05);
        drawTextAt(x, y, name);
    }
    else
    {
        hires_putc(x - 1, y, ROP_CPY, 0x62);
        drawTextAltAt(x, y, name);
    }
    background = 0;
}
void drawText(uint8_t x, uint8_t y, const char *s)
{
    y = y * 8 + OFFSET_Y;
    if (y > 184)
        y = 184;
    drawTextAt(x, y, s);
}
void drawTextAt(uint8_t x, uint8_t y, const char *s)
{
    char c;

    while ((c = *s++))
    {
        if (c >= 97 && c <= 122)
            c -= 32;
        hires_putc(x++, y, ROP_CPY, c);
    }
}
void drawTextAlt(uint8_t x, uint8_t y, const char *s)
{
    y = y * 8 + OFFSET_Y;
    if (y > 184)
        y = 184;
    drawTextAltAt(x, y, s);
}

void drawTextAltAt(uint8_t x, uint8_t y, const char *s)
{
    char c;
    uint8_t rop;

    while ((c = *s++))
    {
        if (c < 65 || c > 90)
        {
            rop = ROP_BLUE;
        }
        else
        {
            rop = ROP_CPY;
        }

        if (c >= 97 && c <= 122)
            c -= 32;
        hires_putc(x++, y, rop, c);
    }
}

void resetScreen()
{
    pcls(0);
}

void drawLegendShip(uint8_t player, uint8_t index, uint8_t size, uint8_t status)
{
    uint16_t dest = fieldX + quadrant_offset[player] + legendShipOffset[index];

    if (player > 1 || (player > 0 && fieldX > 0))
    {
        dest += 256 + 11;
    }
    else
    {
        dest += 256 - 4;
    }

    if (status)
    {
        drawShipInternal((uint8_t *)SCREEN + dest, size, 1);
    }
    else
    {
        hires_Draw((uint8_t)(dest % 32), (uint8_t)(dest / 32), 1, size * 8, ROP_CPY, &charset[(uint16_t)0x1c << 3]);
    }
}

void drawGamefieldCursor(uint8_t quadrant, uint8_t x, uint8_t y, uint8_t *gamefield, uint8_t blink)
{
    uint8_t *src, *dest = (uint8_t *)SCREEN + quadrant_offset[quadrant] + fieldX + (uint16_t)y * 256 + x;
    uint8_t j, c = gamefield[y * 10 + x];

    if (blink)
    {
        c = c * 2 + 5 + blink;
    }
    else
    {
        c += 0x18;
    }
    src = &charset[(uint16_t)c << 3];

    for (j = 0; j < 8; ++j)
    {
        *dest = *src++;
        dest += 32;
    }
}

uint8_t *srcBlank = &charset[(uint16_t)0x18 << 3];
uint8_t *srcHit = &charset[(uint16_t)0x19 << 3];
uint8_t *srcMiss = &charset[(uint16_t)0x1A << 3];
uint8_t *srcHit2 = &charset[(uint16_t)0x1B << 3];
uint8_t *srcHitLegend = &charset[(uint16_t)0x1C << 3];

// Updates the gamefield display at attackPos
void drawGamefieldUpdate(uint8_t quadrant, uint8_t *gamefield, uint8_t attackPos, uint8_t blink)
{
    uint8_t *src, *dest = (uint8_t *)SCREEN + quadrant_offset[quadrant] + fieldX + (uint16_t)(attackPos / 10) * 256 + (attackPos % 10);
    uint8_t j, c = gamefield[attackPos];

    if (c == FIELD_ATTACK)
    {
        src = blink ? srcHit2 : srcHit;
    }
    else if (c == FIELD_MISS)
    {
        src = srcMiss;
    }
    else
    {
        return;
    }

    for (j = 0; j < 8; ++j)
    {
        *dest = *src++;
        dest += 32;
    }
}

void drawGamefield(uint8_t quadrant, uint8_t *field)
{
    uint8_t *dest = (uint8_t *)SCREEN + quadrant_offset[quadrant] + fieldX;
    uint8_t y, x, j;
    uint8_t *src;

    for (y = 0; y < 10; ++y)
    {
        for (x = 0; x < 10; ++x)
        {
            if (*field)
            {
                src = *field == 1 ? srcHit : srcMiss;
                for (j = 0; j < 8; ++j)
                {
                    *dest = *src++;
                    dest += 32;
                }
                dest -= 256;
            }
            field++;
            dest++;
        }

        dest += 246;
    }
}

void drawShipInternal(uint8_t *dest, uint8_t size, uint8_t delta)
{
    uint8_t i, j, c = 0x12;
    uint8_t *src;
    if (delta)
        c = 0x17;
    for (i = 0; i < size; i++)
    {
        // hires_putc(x, y, ROP_CPY, c);
        // Faster version of above, but uses ~100 bytes
        src = &charset[(uint16_t)c << 3];
        for (j = 0; j < 8; ++j)
        {
            *dest = *src++;
            dest += 32;
        }
        if (delta)
        {
            c = 0x16;
            if (i == size - 2)
                c = 0x15;
        }
        else
        {
            dest -= 255;
            c = 0x13;
            if (i == size - 2)
                c = 0x14;
        }
    }
}
void drawShip(uint8_t size, uint8_t pos, bool hide)
{
    uint8_t x, y, i, j, delta = 0;
    uint8_t *src;

    if (pos > 99)
    {
        delta = 1; // 1=vertical, 0=horizontal
        pos -= 100;
    }

    x = (pos % 10) + fieldX + 5;
    y = ((pos / 10) + 12) * 8 + OFFSET_Y;

    if (hide)
    {
        if (!delta)
            hires_Mask(x, y, size, 8, ROP_BLUE);
        else
            hires_Mask(x, y, 1, size * 8, ROP_BLUE);
        return;
    }

    uint8_t *dest = (uint8_t *)SCREEN + (uint16_t)y * 32 + x;
    drawShipInternal(dest, size, delta);
}

void drawIcon(uint8_t x, uint8_t y, uint8_t icon)
{
    hires_putc(x, y * 8 + OFFSET_Y, ROP_CPY, icon);
}

void drawClock()
{
    hires_putc(WIDTH - 1, HEIGHT * 8 - 8, ROP_CPY, 0x1D);
}

void drawConnectionIcon(bool show)
{
    hires_putcc(0, HEIGHT * 8 - 8, ROP_CPY, show ? 0x1e1f : 0x2020);
}

void drawSpace(uint8_t x, uint8_t y, uint8_t w)
{
    y = y * 8 + OFFSET_Y;
    if (y > 184)
        y = 184;
    hires_Mask(x, y, w, 8, 0);
}

void drawBoard(uint8_t playerCount)
{
    uint8_t i, x, y, ix, ox, left = 1, fy, eh, drawEdge, drawX, drawCorner, edgeSkip;

    uint16_t pos;
    // Center layout
    fieldX = playerCount > 2 ? 0 : 6;

    for (i = 0; i < playerCount; i++)
    {
        pos = fieldX + quadrant_offset[i];
        x = (uint8_t)(pos % 32);
        y = (uint8_t)(pos / 32);

        // right and left drawers
        if (i > 1 || playerCount == 2 && i > 0)
        {
            ox = x - 1;
            ix = x + 10;
            left = 0;
            drawX = ix + 1;
            drawEdge = drawX + 3;
            drawCorner = 0xd;
        }
        else
        {
            ix = x - 1;
            ox = x + 10;
            drawX = ix - 3;
            drawEdge = drawX - 1;
            drawCorner = 0xc;
        }
        if (i == 1 || i == 2)
        {
            // Name badge corners
            hires_putc(x - 1, y - 8, ROP_CPY, 0x5C);
            hires_putc(x + 10, y - 8, ROP_CPY, 0x5D);

            // Name badge

            // Fill
            hires_Mask(x, y - 9, 10, 9, ROP_YELLOW);

            // Border
            hires_Mask(x, y - 10, 10, 1, ROP_BLUE);
            hires_Mask(x - 1, y - 10, 1, 1, 0b00000010);
            hires_Mask(x + 10, y - 10, 1, 1, 0b10000000);
            hires_Mask(x - 1, y - 9, 1, 1, 0b001001);
            hires_Mask(x + 10, y - 9, 1, 1, 0b01100000);

            fy = y + 80;
        }
        else
        {
            // Name badge corners

            hires_putc(x - 1, y + 80, ROP_CPY, 0x5E);
            hires_putc(x + 10, y + 80, ROP_CPY, 0x5F);

            // Name fill
            hires_Mask(x, y + 80, 10, 9, ROP_YELLOW);

            // Border
            hires_Mask(x - 1, y + 88, 1, 1, 0b001001);
            hires_Mask(x + 10, y + 88, 1, 1, 0b01100000);

            hires_Mask(x, y + 89, 10, 1, ROP_BLUE);
            hires_Mask(x - 1, y + 89, 1, 1, 0b00000010);
            hires_Mask(x + 10, y + 89, 1, 1, 0b10000000);

            fy = y - 8;
        }

        // Outside edge
        hires_Draw(ox, y, 1, 80, ROP_CPY, &charset[(uint16_t)(left ? 0x23 : 0x22) << 3]);

        // Inner edge (adjacent to ships drawer)
        hires_Draw(ix, y + 8, 1, 64, ROP_CPY, &charset[(uint16_t)(left ? 0x01 : 0x04) << 3]);

        // Inner edge + ship drawer
        hires_putc(ix, y, ROP_CPY, left ? 0x24 : 0x25);
        hires_putc(ix, y + 72, ROP_CPY, left ? 0x24 : 0x25);

        // Blue gamefield
        hires_Mask(x, y, 10, 80, ROP_BLUE);
        edgeSkip = 0;
        if (playerCount == 1)
        {
            fy += 5;
            edgeSkip = 4;
        }
        // Far edge
        if (i || edgeSkip)
        {
            if (i != 2 && !edgeSkip)
                eh = 8;
            else
                eh = 3;

            hires_Draw(x - 1, fy, 1, eh, ROP_CPY, &charset[(uint16_t)0x02 << 3] + edgeSkip);
            hires_Draw(x + 10, fy, 1, eh, ROP_CPY, &charset[(uint16_t)0x03 << 3] + edgeSkip);
            hires_Draw(x, fy, 10, eh, ROP_CPY, &charset[(uint16_t)0x29 << 3] + edgeSkip);
        }

        // Ship drawer edges
        hires_Draw(drawX, y, 3, 8, ROP_CPY, &charset[(uint16_t)0x11 << 3]);
        hires_Draw(drawX, y + 72, 3, 8, ROP_CPY, &charset[(uint16_t)0x11 << 3]);
        hires_Draw(drawEdge, y + 8, 1, 64, ROP_CPY, &charset[(uint16_t)0x10 << 3]);
        hires_putc(drawEdge, y, ROP_CPY, drawCorner);
        hires_putc(drawEdge, y + 72, ROP_CPY, drawCorner + 2);

        // Fill in the drawer
        hires_Mask(drawX, y + 8, 3, 64, ROP_BLUE);
    }
}

void drawLine(uint8_t x, uint8_t y, uint8_t w)
{
    y = y * 8 + OFFSET_Y + 1;
    hires_Mask(x, y, w, 2, ROP_BLUE);
}

void drawBox(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    y = y * 8 + 1 + OFFSET_Y;

    // Top Corners
    hires_putc(x, y, box_color, 0x3b);
    hires_putc(x + w + 1, y, box_color, 0x3c);

    // Top/bottom lines
    // hires_Mask(x+1,y+3,w,2, box_color);
    // hires_Mask(x+1,y+(h+1)*8+2,w,2, box_color);

    // Sides
    //   for(i=0;i<h;++i) {
    //     y+=8;
    //     hires_putc(x,y,box_color, 0x3f);
    //     hires_putc(x+w+1,y,box_color,0x40);
    //   }

    y += 8 * (h - 1);
    // Bottom Corners
    hires_putc(x, y + 14, box_color, 0x3d);
    hires_putc(x + w + 1, y + 14, box_color, 0x3e);
}

void resetGraphics()
{
}

void waitvsync()
{
    uint16_t i = getTimer();
    while (i == getTimer())
        ;
}

void drawBlank(uint8_t x, uint8_t y)
{
    hires_putc(x, y * 8 + OFFSET_Y, ROP_CPY, 0x20);
}
