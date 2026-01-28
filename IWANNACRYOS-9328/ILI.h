#ifndef ILI_H
#define ILI_H

#include <Arduino.h>
#include <avr/pgmspace.h>

// ---------- REGISTER MAP ----------
#define ILI_ID_CHECK       0x00
#define ILI_DRIV_OUT       0x01
#define ILI_ENTRY_MODE     0x03
#define ILI_DISP_CTRL1     0x07
#define ILI_GRAM_AD_X      0x20
#define ILI_GRAM_AD_Y      0x21
#define ILI_RW_GRAM        0x22
#define ILI_H_START        0x50
#define ILI_H_END          0x51
#define ILI_V_START        0x52
#define ILI_V_END          0x53
#define ILI_GATE_SCAN      0x60

// Common 16-bit RGB565 color values
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

// ---------- MACROS ----------
#define PIXEL_PULSE \
    D01_PORT = hi_B; D27_PORT = hi_D; WR_LOW(); WR_HIGH(); \
    D01_PORT = lo_B; D27_PORT = lo_D; WR_LOW(); WR_HIGH();

// ---------- CLASS / STRUCT FOR TEXT STATE ----------
typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t textColor;
    uint16_t bgColor;
    uint8_t  textSize;
    bool     wrap;
} TFT_State;

extern TFT_State cursor;

// ---------- CORE FUNCTIONS ----------
// ---------- CORE FUNCTIONS ----------
void tftInit(void);
void tftSetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1); // Match the .cpp arguments
void tftResetWindow(void);
void tftCmd(uint16_t reg);   // Remove void 
void tftData(uint16_t data); // Remove 

// ---------- FAST DRAWING ----------
void drawPixel(uint16_t x, uint16_t y, uint16_t color);
void tftFill(uint16_t color);
void fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void drawFastHLine(uint16_t x, uint16_t y, uint16_t w, uint16_t color);
void drawFastVLine(uint16_t x, uint16_t y, uint16_t h, uint16_t color);
void drawLine(int x0, int y0, int x1, int y1, uint16_t color);

// Add these to ILI.h
extern TFT_State cursor;

// fast functions
void setCursor(uint16_t x, uint16_t y);
void setTextColor(uint16_t c, uint16_t b);
void setTextColor(uint16_t c);
void setTextSize(uint8_t s);
void setTextWrap(bool w);
uint16_t getCursorX(void);
uint16_t getCursorY(void);

// Core text functions
void drawChar(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg, uint8_t size);
void tftPrint(const char* str);
void tftPrintln(const char* str);
void tftPrintChar(char c);
void tftPrintInt(int32_t n);
void tftPrintUInt(uint32_t n);
void tftPrintFixed(uint32_t value, uint8_t decimals);
void clearToEOL(void);
void clearLine(void);
#endif


/*
====================================================
ILI9328 FAST LIB — BENCHMARK RESULTS (Arduino UNO)
Screen: 320x240 RGB565
MCU: ATmega328P @ 16MHz
Test: 100 calls per function
----------------------------------------------------
drawPixel:
  7064 us total
  14.16 calls/ms
  884.77 calls per 1/16 second

drawFastHLine:
  59224 us total
  1.69 calls/ms
  105.53 calls per 1/16 second

drawFastVLine:
  46180 us total
  2.17 calls/ms
  135.34 calls per 1/16 second

fillRect (50x50):
  462616 us total
  0.22 calls/ms
  13.51 calls per 1/16 second

tftFill (full screen):
  5985792 us total
  0.02 calls/ms
  1.04 calls per 1/16 second

drawLine (diagonal):
  2167600 us total
  0.05 calls/ms
  2.88 calls per 1/16 second

drawChar (size 1):
  52564 us total
  1.90 calls/ms
  118.90 calls per 1/16 second
----------------------------------------------------
Notes:
- drawPixel is the only viable primitive for 3D rendering.
- Full screen clears are unusable per-frame.
- Motion blur via dark fillRect is mandatory.
- Dense Bresenham lines approximate solid edges.
====================================================
*/
