/*
GBA Multi Game Menu
Author: Lesserkuma (github.com/lesserkuma)
*/

#ifndef MAIN_H_
#define MAIN_H_

#include "bg.h"

typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;
typedef uint8_t BOOL;

#define FALSE 0
#define TRUE 1

#define AGB_ROM (volatile void *)0x8000000
#define AGB_PRAM (volatile void *)0x5000000
#define AGB_VRAM (volatile void *)0x6000000
#define AGB_SRAM (volatile void *)0xE000000

#define MAPPER_CONFIG1 (volatile void *)0xE000002
#define MAPPER_CONFIG2 (volatile void *)0xE000003
#define MAPPER_CONFIG3 (volatile void *)0xE000004
#define MAPPER_CONFIG4 (volatile void *)0xE000005

#define SRAM_SIZE 64 * 1024

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 160
#define SCREEN_MARGIN_RIGHT 7

#define V5bit(x) ((x) >> 3)
#define RGB555(r, g, b) ((V5bit(r) << 0) | (V5bit(g) << 5) | (V5bit(b) << 10) | (((1) & 1) << 15))
#define RGB555_CLEAR 0
#define RGB555_BLACK RGB555(0x00, 0x00, 0x00)
#define RGB555_WHITE RGB555(0xFF, 0xFF, 0xFF)
#define RGB555_RED RGB555(0xFF, 0x00, 0x00)
#define RGB555_GREEN RGB555(0x00, 0xFF, 0x00)
#define RGB555_BLUE RGB555(0x00, 0x00, 0xFF)
#define RGB555_PURPLE RGB555(0xFF, 0x00, 0xFF)
#define RGB555_YELLOW RGB555(0xFF, 0xFF, 0x00)
#define RGB555_GREY RGB555(0x7F, 0x7F, 0x7F)
#define RGB555_MILK RGB555(0x94, 0x94, 0x94)

#define _DIV_CEIL(a, b) ((a / b) + ((a % b) != 0))

typedef enum u8
{
	SRAM_NONE,
	SRAM_256K,
	SRAM_512K,
	SRAM_1M
} SAVE_TYPE;

typedef struct ItemConfig_
{
	u8 font;
	u8 title_length;
	u16 rom_offset;
	u16 rom_size;
	SAVE_TYPE save_type;
	u8 save_index;
	u16 keys;
	u8 reserved[6];
	u16 title[0x30];
} ItemConfig;

extern char __rom_end__;

void SetPixel(volatile u16 *buffer, u8 row, u8 col, u8 color);

#endif
