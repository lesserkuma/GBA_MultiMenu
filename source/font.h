/*
GBA Multi Game Menu
Author: Lesserkuma (github.com/lesserkuma)
*/

#ifndef FONT_H_
#define FONT_H_

#include "main.h"

#include "font_nftr.h"

#if __has_include("NTR_IPL_font_s_nftr.h")
	#define FONT_NTR_IPL
	#include "NTR_IPL_font_s_nftr.h"
#endif
#if __has_include("TBF1_s_nftr.h")
	#define FONT_TBF1
	#include "TBF1_s_nftr.h"
#endif
#if __has_include("TBF1-cn_s_nftr.h")
	#define FONT_TBF1_CN
	#include "TBF1-cn_s_nftr.h"
#endif
#if __has_include("TBF1-kr_s_nftr.h")
	#define FONT_TBF1_KR
	#include "TBF1-kr_s_nftr.h"
#endif
#if __has_include("TWL-IRAJ-1_nftr.h")
	#define FONT_TWL_IRAJ_1
	#include "TWL-IRAJ-1_nftr.h"
#endif

#define MAGIC_NFTR 0x4E465452
#define MAGIC_FINF 0x46494E46
#define MAGIC_CGLP 0x43474C50
#define MAGIC_CMAP 0x434D4150
#define MAGIC_CWDH 0x43574448

#define ALIGN_LEFT   0
#define ALIGN_CENTER 1
#define ALIGN_RIGHT  2

typedef struct FontSpecs_
{
	u32 cmap_offset;
	u32 cwdh_offset;
	u32 cglp_offset;
	u32 num_of_chars;
	u8 max_width;
	u8 max_height;
	u8 bytes_per_char;
	u8 bpp;
	u8 nftr_version;
	u16 fallback_char;
} FontSpecs;

typedef struct NFTR_Header_
{
	u32 magic;
	u16 byteorder;
	u8 version;
	u8 unknown1;
	u32 file_size;
	u16 size;
	u16 chunks_num;
} NFTR_Header;

typedef struct FINF_Header_
{
	u32 magic;
	u32 size;
	u32 unknown1;
	u32 unknown2;
	u32 unknown3;
	u32 offset_CWDH;
	u32 offset_CMAP;
} FINF_Header;

typedef struct CGLP_Header_
{
	u32 magic;
	u32 size;
	u8 max_width;
	u8 max_height;
	u16 bytes_per_char;
	u16 unknown1;
	u8 bpp;
	u8 orientation;
} CGLP_Header;

typedef struct CWDH_Header_
{
	u32 magic;
	u32 size;
	u16 unknown1;
	u16 num_of_chars;
	u32 unknown2;
} CWDH_Header;

typedef struct CMAP_Header_
{
	u32 magic;
	u32 size;
	u16 start_code;
	u16 end_code;
	u16 type;
	u16 unknown1;
	u32 next_offset;
} CMAP_Header;

void LoadFont(u8 index);
void LoadNFTR(const u8 *nftr_data);
u16 GetFontIndex(u16 ch, const u8 *nftr_data);
void GetFontWidths(u16 index, const u8 *nftr_data, u8 *a, u8 *b, u8 *c);
void AsciiToUnicode(char *text, u16 *output);
void DrawText(u8 px, u8 py, u8 align, u16 *text, u8 length, const u8 *nftr_data, volatile void *canvas, BOOL highlighted);

#endif
