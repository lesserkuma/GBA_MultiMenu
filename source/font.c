/*
GBA Multi Game Menu
Author: Lesserkuma (github.com/lesserkuma)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "font.h"

FontSpecs sFontSpecs;
NFTR_Header sNFTR_Header;
FINF_Header sFINF_Header;
CGLP_Header sCGLP_Header;
CWDH_Header sCWDH_Header;
CMAP_Header sCMAP_Header;

u16 FallbackCharacter;
u16 ArrowCharacter;
s8 FontMarginTop;
s8 FontMarginBottom;

u8 last_font = -1;
const u8* font;

void LoadFont(u8 index) {
	if (index != last_font) {
		font = font_nftr;
		FallbackCharacter = 0x2753;
		ArrowCharacter = 0x21E8;
		FontMarginTop = 0;
		FontMarginBottom = 0;
#ifdef FONT_NTR_IPL
		if (index == 1) {
			font = NTR_IPL_font_s_nftr;
			FallbackCharacter = 0xE011;
			ArrowCharacter = 0xE019;
			FontMarginTop = 2;
			FontMarginBottom = 3;
		}
#endif
#ifdef FONT_TBF1
		if (index == 2) {
			font = TBF1_s_nftr;
			FallbackCharacter = 0xE011;
			ArrowCharacter = 0xE019;
			FontMarginTop = 1;
			FontMarginBottom = 1;
		}
#endif
#ifdef FONT_TBF1_CN
		if (index == 3) {
			font = TBF1_cn_s_nftr;
			FallbackCharacter = 0xE011;
			ArrowCharacter = 0xE019;
			FontMarginTop = 2;
			FontMarginBottom = 2;
		}
#endif
#ifdef FONT_TBF1_KR
		if (index == 4) {
			font = TBF1_kr_s_nftr;
			FallbackCharacter = 0xE011;
			ArrowCharacter = 0xE019;
			FontMarginTop = 2;
			FontMarginBottom = 2;
		}
#endif
#ifdef FONT_TWL_IRAJ_1
		if (index == 5) {
			font = TWL_IRAJ_1_nftr;
			FallbackCharacter = 0xFF1F;
			ArrowCharacter = 0x2192;
			FontMarginTop = 4;
			FontMarginBottom = 6;
		}
#endif
		LoadNFTR(font);
		last_font = index;
	}
}

void LoadNFTR(const u8* nftr_data) {
	u32 pos = 0;
	memcpy(&sNFTR_Header, nftr_data+pos, sizeof(sNFTR_Header));
	pos += sNFTR_Header.size;
	memcpy(&sFINF_Header, nftr_data+pos, sizeof(sFINF_Header));
	pos += sFINF_Header.size;
	memcpy(&sCGLP_Header, nftr_data+pos, sizeof(sCGLP_Header));
	sFontSpecs.cglp_offset = pos;
	pos = sFINF_Header.offset_CWDH - 8;
	memcpy(&sCWDH_Header, nftr_data+pos, sizeof(sCWDH_Header));
	sFontSpecs.cwdh_offset = pos + 16;
	pos = sFINF_Header.offset_CMAP - 8;
	memcpy(&sCMAP_Header, nftr_data+pos, sizeof(sCMAP_Header));
	sFontSpecs.cmap_offset = pos;
	sFontSpecs.nftr_version = sNFTR_Header.version;
	sFontSpecs.max_width = sCGLP_Header.max_width;
	sFontSpecs.max_height = sCGLP_Header.max_height;
	sFontSpecs.bytes_per_char = sCGLP_Header.bytes_per_char;
	sFontSpecs.num_of_chars = sCWDH_Header.num_of_chars;
	sFontSpecs.bpp = sCGLP_Header.bpp;
}

u16 GetFontIndex(u16 ch, const u8* nftr_data) {
	u32 pos = sFontSpecs.cmap_offset;
	while (TRUE) {
		if (pos <= 0) return 0xFFFF;
		CMAP_Header tCMAP_Header;
		memcpy(&tCMAP_Header, nftr_data+pos, sizeof(tCMAP_Header));
		pos += 20;
		
		if (ch < tCMAP_Header.start_code || ch > tCMAP_Header.end_code) {
			// CMAP doesn't have this character
			pos = tCMAP_Header.next_offset - 8;
			continue;
		}
		
		if (tCMAP_Header.type == 0) {
			u16 index_offset = nftr_data[pos+1] << 8 | nftr_data[pos];
			return ch - tCMAP_Header.start_code + index_offset;
		} else if (tCMAP_Header.type == 1) {
			u16 utf16le_index = tCMAP_Header.start_code;
			for (u16 i = tCMAP_Header.start_code; i < tCMAP_Header.end_code; i++) {
				u16 font_index = nftr_data[pos+1] << 8 | nftr_data[pos];
				pos += 2;
				if (utf16le_index == ch) return font_index;
				utf16le_index += 1;
			}
		} else if (tCMAP_Header.type == 2) {
			u16 index_offset = nftr_data[pos+1] << 8 | nftr_data[pos];
			pos += 2;
			for (u16 i = 0; i < index_offset; i++) {
				u16 utf16le_index = nftr_data[pos+1] << 8 | nftr_data[pos];
				pos += 2;
				u16 font_index = nftr_data[pos+1] << 8 | nftr_data[pos];
				pos += 2;
				if (utf16le_index == ch) return font_index;
				utf16le_index += 1;
			}
		}
		return 0xFFFF;
	}
}

void GetFontWidths(u16 index, const u8* nftr_data, u8* a, u8* b, u8* c) {
	u32 pos = sFontSpecs.cwdh_offset;
	pos = pos + (index * 3);
	*a = nftr_data[pos++];
	*b = nftr_data[pos++];
	*c = nftr_data[pos++];
}

void AsciiToUnicode(char* text, u16* output) {
	for (u8 i = 0; i < 64; i++) {
		if (text[i] == 0) break;
		output[i] = text[i];
	}
}

void DrawText(u8 px, u8 py, u8 align, u16* text, u8 length, const u8* nftr_data, volatile void* vram, BOOL highlighted) {
	u8 pos_left = 0;
	u8 glyph_width = 0;
	u8 glyph_left = 0;
	u8 pixels[sFontSpecs.max_width * sFontSpecs.max_height];
	u8 canvas[SCREEN_WIDTH * sFontSpecs.max_height];
	u8 color_modifier = 0;
	u32 offset = 0;
	
	py += FontMarginTop;

	if (highlighted) {
		color_modifier = 10;
	}
	
	memset(canvas, 255, SCREEN_WIDTH * sFontSpecs.max_height);
	for (u8 i = 0; i < length; i++) {
		u8 a, b, c = 0;
		
		u16 ch = text[i];
		if (ch == 0x0000 || ch == 0xFFFF) {
			break;
		}
		
		BOOL last_ch = FALSE;
		while (1) {
			u16 index = GetFontIndex(ch, nftr_data);
			if (index == 0xFFFF) { // character not found
				ch = FallbackCharacter;
				continue;
			}
			if (pos_left + sFontSpecs.max_width + (sFontSpecs.max_width >> 1) >= SCREEN_WIDTH - SCREEN_MARGIN_RIGHT - px) {
				if (ch != 0x2026) {
					ch = 0x2026; // ...
					last_ch = TRUE;
					continue;
				}
			}
			GetFontWidths(index, nftr_data, &a, &b, &c);
			
			offset = index * sFontSpecs.bytes_per_char;
			glyph_left = a;
			if (glyph_left >= sFontSpecs.max_width) glyph_left = 0;
			if (ch == 32) { // space
				glyph_left = 0;
			}
			
			glyph_width = c - glyph_left;
			break;
		}
		if (pos_left + glyph_width >= SCREEN_WIDTH - px) {
			break;
		}

		if (b == 0) {
			glyph_width = c;
		}
		if (sFontSpecs.nftr_version == 1) {
			if (glyph_width == 0) glyph_width = sFontSpecs.max_width;
			glyph_width += 1;
		}
		
		const u8* gl = &nftr_data[sFontSpecs.cglp_offset + 0x10 + offset];
		
		u16 pixel_pos = 0;
		if (align == ALIGN_LEFT) { // Draw to VRAM directly
			if (sFontSpecs.bpp == 1) {
				for (u8 a = 0; a < sFontSpecs.bytes_per_char; a++) {
					for (u8 b = 0; b < 8; b++) {
						pixels[pixel_pos++] = ((gl[a] >> (7 - b)) & 1);
					}
				}
				for (u8 x = 0; x < sFontSpecs.max_height; x++) {
					for (u8 y = 0; y < sFontSpecs.max_width; y++) {
						u8 p = pixels[(x * sFontSpecs.max_width) + y];
						if (p == 1) {
							SetPixel(vram, py + x, px + y + pos_left, 254 - color_modifier);
						}
					}
				}
			} else if (sFontSpecs.bpp == 2) {
				for (u8 a = 0; a < sFontSpecs.bytes_per_char; a++) {
					for (u8 b = 0; b < 8; b += sFontSpecs.bpp) {
						u8 p = 0;
						for (u8 bit = 0; bit < sFontSpecs.bpp; bit++) {
							p |= ((gl[a] >> (7 - (b + bit))) & 1) << bit;
						}
						pixels[pixel_pos++] = p;
					}
				}
				for (u8 x = 0; x < sFontSpecs.max_height; x++) {
					for (u8 y = 0; y < sFontSpecs.max_width; y++) {
						u8 p = pixels[(x * sFontSpecs.max_width) + y];
						if (p != 0) {
							SetPixel(vram, py + x, px + y + pos_left, p + 250 - color_modifier);
						}
					}
				}
			}
		} else {
			if (sFontSpecs.bpp == 1) {
				for (u8 a = 0; a < sFontSpecs.bytes_per_char; a++) {
					for (u8 b = 0; b < 8; b++) {
						pixels[pixel_pos++] = ((gl[a] >> (7 - b)) & 1);
					}
				}
				for (u8 x = 0; x < sFontSpecs.max_height; x++) {
					for (u8 y = 0; y < sFontSpecs.max_width; y++) {
						u8 p = pixels[(x * sFontSpecs.max_width) + y];
						if (p == 1) {
							canvas[(x * SCREEN_WIDTH) + y + pos_left] = 254 - color_modifier;
						}
					}
				}
			} else if (sFontSpecs.bpp == 2) {
				for (u8 a = 0; a < sFontSpecs.bytes_per_char; a++) {
					for (u8 b = 0; b < 8; b += sFontSpecs.bpp) {
						u8 p = 0;
						for (u8 bit = 0; bit < sFontSpecs.bpp; bit++) {
							p |= ((gl[a] >> (7 - (b + bit))) & 1) << bit;
						}
						pixels[pixel_pos++] = p;
					}
				}
				for (u8 x = 0; x < sFontSpecs.max_height; x++) {
					for (u8 y = 0; y < sFontSpecs.max_width; y++) {
						u8 p = pixels[(x * sFontSpecs.max_width) + y];
						if (p != 0) {
							canvas[(x * SCREEN_WIDTH) + y + pos_left] = p + 250 - color_modifier;
						}
					}
				}
			}
		}
		
		pos_left += glyph_width;
		if (last_ch) break;
	}
	
	if (align == ALIGN_LEFT) return;
	
	if (align == ALIGN_CENTER) {
		px = (SCREEN_WIDTH - pos_left) >> 1;
	} else if (align == ALIGN_RIGHT) {
		px = SCREEN_WIDTH - pos_left - px + 1;
	}
	u16 c = 0;
	for (u8 x = 0; x < sFontSpecs.max_height; x++) {
		for (u8 y = 0; y < SCREEN_WIDTH; y++) {
			if (y > pos_left) {
				c++;
				continue;
			}
			if (canvas[c] != 255) {
				SetPixel(vram, py + x, px + y, canvas[c]);
			}
			c++;
		}
	}
}
