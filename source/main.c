/*
GBA Multi Game Menu
Author: Lesserkuma (github.com/lesserkuma)
*/

#include <gba.h>
#include <gba_console.h>
#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_input.h>
#include <gba_dma.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "version.h"
#include "main.h"
#include "font.h"
#include "flash.h"

extern FontSpecs sFontSpecs;
extern u16 FallbackCharacter;
extern u16 ArrowCharacter;
extern s8 FontMarginTop;
extern s8 FontMarginBottom;
extern const u8* font;
extern u8 *itemlist;
extern u8 flash_type;
extern u16 itemlist_offset;
extern u32 flash_sector_size;
extern u32 flash_itemlist_sector_offset;
extern u32 flash_status_sector_offset;
extern u32 flash_save_sector_offset;
extern u8 data_buffer[0x10000];
ItemConfig sItemConfig;
FlashStatus sFlashStatus;

void SetPixel(volatile u16* buffer, u8 row, u8 col, u8 color) {
	/* https://ianfinlayson.net/class/cpsc305/notes/09-graphics */
	u16 offset = (row * SCREEN_WIDTH + col) >> 1;
	u16 pixel = buffer[offset];
	if (col & 1) {
		buffer[offset] = (color << 8) | (pixel & 0x00FF);
	} else {
		buffer[offset] = (pixel & 0xFF00) | color;
	}
}

void ClearList(void* vram, u8 top, u8 height) {
	dmaCopy(bgBitmap + (top * (SCREEN_WIDTH >> 2)), (void*)AGB_VRAM+0xA000 + (top * SCREEN_WIDTH), SCREEN_WIDTH * height);
}

int main(void) {
	char temp_ascii[64];
	u16 temp_unicode[64];
	s8 page_active = 0;
	u8 page_total = 64;
	u16 roms_total = 0;
	s8 cursor_pos = 0;
	u8 redraw_items = 0xFF;
	u8 roms_page = 7;
	u16 kHeld = 0;
	u16 kHeld_boot = 0;
	BOOL show_debug = FALSE;
	BOOL show_credits = FALSE;
	BOOL boot_failed = FALSE;

	irqInit();
	irqEnable(IRQ_VBLANK);

	FlashDetectType();

	// Load palette
	memset((void*)AGB_VRAM, 255, SCREEN_WIDTH * SCREEN_HEIGHT * 2);
	dmaCopy(bgPal, BG_PALETTE, 256 * 2);
	((u16*)AGB_PRAM)[250] = 0xFFFF;
	((u16*)AGB_PRAM)[251] = 0xB18C;
	((u16*)AGB_PRAM)[252] = 0xDEF7;
	((u16*)AGB_PRAM)[253] = 0x9084;
	((u16*)AGB_PRAM)[254] = 0x8000;
	((u16*)AGB_PRAM)[255] = 0xFFFF;
	((u16*)AGB_PRAM)[240] = 0xFFFF;
	((u16*)AGB_PRAM)[241] = 0xDD8C;
	((u16*)AGB_PRAM)[242] = 0xFAF7;
	((u16*)AGB_PRAM)[243] = 0xC084;
	((u16*)AGB_PRAM)[244] = 0xC084;
	((u16*)AGB_PRAM)[245] = 0xFFFF;
	VBlankIntrWait();

	// Load background
	SetMode(MODE_4 | BG2_ENABLE);
	dmaCopy(bgBitmap, (void*)AGB_VRAM+0xA000, SCREEN_WIDTH * SCREEN_HEIGHT);
	
	// Check on-boot keys
	scanKeys();
	kHeld = keysHeld();
	kHeld_boot = kHeld;
	if ((kHeld & KEY_SELECT) && (kHeld & KEY_R)) {
		show_credits = TRUE;
	} else if (kHeld & KEY_SELECT) {
		show_debug = TRUE;
	}
	if (kHeld) {
		BOOL found_keys = FALSE;
		for (itemlist_offset = 0; itemlist_offset < 0xE000; itemlist_offset += 0x70) {
			memcpy(&sItemConfig, ((u8*)itemlist)+itemlist_offset, sizeof(sItemConfig));
			if (sItemConfig.title_length == 0) break;
			if (sItemConfig.title_length == 0xFF) break;
			if (sItemConfig.keys == kHeld) {
				found_keys = TRUE;
				break;
			}
		}
		if (!found_keys) {
			kHeld = 0;
			itemlist_offset = 0;
		}
	}

	// Count number of ROMs
	for (roms_total = 0; roms_total < 512; roms_total++) {
		memcpy(&sItemConfig, ((u8*)itemlist+itemlist_offset)+(0x70*roms_total), sizeof(sItemConfig));
		if (sItemConfig.keys != kHeld) break;
		if (sItemConfig.title_length == 0) break;
		if (sItemConfig.title_length == 0xFF) break;
	}
	if (roms_total == 0) {
		LoadFont(2);
		DrawText(0, 64, ALIGN_CENTER, u"Please use the ROM Builder to", 48, font, (void*)AGB_VRAM+0xA000, FALSE);
		DrawText(0, 64 + sFontSpecs.max_height, ALIGN_CENTER, u"create your own compilation.", 48, font, (void*)AGB_VRAM+0xA000, FALSE);
		LoadFont(0);
		DrawText(0, 127, ALIGN_CENTER, u"https://github.com/lesserkuma/GBA_MultiMenu", 48, font, (void*)AGB_VRAM+0xA000, FALSE);
		DrawText(14, SCREEN_HEIGHT - sFontSpecs.max_height - 3 - FontMarginBottom, ALIGN_RIGHT, u"No ROMs", 10, font, (void*)AGB_VRAM+0xA000, FALSE);
		REG_DISPCNT ^= 0x0010;
		while (1) { VBlankIntrWait(); }
	} else if (roms_total == 1) {
		memcpy(&sItemConfig, ((u8*)itemlist+itemlist_offset), sizeof(sItemConfig));
		u8 error_code = BootGame(sItemConfig, sFlashStatus);
		boot_failed = error_code;
	}
	page_total = (roms_total + 8.0 - 1) / 8.0;

	memcpy(&sFlashStatus, (void *)(AGB_ROM + flash_status_sector_offset * flash_sector_size), sizeof(sFlashStatus));
	if ((sFlashStatus.magic != MAGIC_FLASH_STATUS) || (sFlashStatus.last_boot_menu_index >= roms_total)) {
		sFlashStatus.magic = MAGIC_FLASH_STATUS;
		sFlashStatus.version = 0;
		sFlashStatus.battery_present = 1;
		sFlashStatus.last_boot_menu_index = 0xFFFF;
		sFlashStatus.last_boot_save_index = 0xFF;
		sFlashStatus.last_boot_save_type = SRAM_NONE;
	} else {
		cursor_pos = sFlashStatus.last_boot_menu_index % 8;
		page_active = sFlashStatus.last_boot_menu_index / 8;
	}

	s32 wait = 0;
	u8 f = 0;
	while (1) {
		if (redraw_items != 0) {
			if (redraw_items == 0xFF) {
				// Full redraw (new page etc.)
				roms_page = 7;
				for (u8 i = 0; i < 8; i++) {
					if ((page_active * 8 + i) >= roms_total) {
						roms_page = i - 1;
						break;
					}
				}
				if (roms_page < 7) ClearList((void*)AGB_VRAM+0xA000, 26+(roms_page+1)*14, 14*(8-roms_page));
				if (cursor_pos > roms_page) cursor_pos = roms_page;
				for (u8 i = 0; i <= roms_page; i++) {
					memcpy(&sItemConfig, ((u8*)itemlist+itemlist_offset)+0x70*(page_active*8+i), sizeof(sItemConfig));
					ClearList((void*)AGB_VRAM+0xA000, 27+i*14, 14);
					LoadFont(sItemConfig.font);
					DrawText(28, 26+i*14, ALIGN_LEFT, sItemConfig.title, sItemConfig.title_length, font, (void*)AGB_VRAM+0xA000, i == cursor_pos);
				}
			} else {
				// Re-draw only changed list items (cursor moved up or down)
				for (u8 i = 0; i < 8; i++) {
					if ((redraw_items >> i) & 1) {
						memcpy(&sItemConfig, ((u8*)itemlist+itemlist_offset)+0x70*(page_active*8+i), sizeof(sItemConfig));
						ClearList((void*)AGB_VRAM+0xA000, 27+i*14, 14);
						LoadFont(sItemConfig.font);
						DrawText(28, 26+i*14, ALIGN_LEFT, sItemConfig.title, sItemConfig.title_length, font, (void*)AGB_VRAM+0xA000, i == cursor_pos);
					}
				}
			}
			
			memcpy(&sItemConfig, ((u8*)itemlist+itemlist_offset)+0x70*(page_active*8+cursor_pos), sizeof(sItemConfig));

			// Draw cursor
			LoadFont(1);
			ClearList((void*)AGB_VRAM+0xA000, SCREEN_HEIGHT - sFontSpecs.max_height - 1, sFontSpecs.max_height);
			u16 arrow[1] = { ArrowCharacter };
			DrawText(14, 26+cursor_pos*14, ALIGN_LEFT, (u16*)&arrow, 1, font, (void*)AGB_VRAM+0xA000, FALSE);
			
			// Draw status bar
			LoadFont(2);
			memset(temp_unicode, 0, sizeof(temp_unicode));
			snprintf(temp_ascii, 10+1, "%d/%d", page_active*8+cursor_pos+1, roms_total);
			AsciiToUnicode(temp_ascii, temp_unicode);
			DrawText(11, SCREEN_HEIGHT - sFontSpecs.max_height - 3 - FontMarginBottom, ALIGN_RIGHT, temp_unicode, 10, font, (void*)AGB_VRAM+0xA000, FALSE);
			if (boot_failed) {
				LoadFont(0);
				if (boot_failed == 1) {
					DrawText(5, SCREEN_HEIGHT - sFontSpecs.max_height - 3 - FontMarginBottom, ALIGN_LEFT, u"Error: Unsupported cartridge!", 48, font, (void*)AGB_VRAM+0xA000, FALSE);
				} else if (boot_failed == 2) {
					DrawText(5, SCREEN_HEIGHT - sFontSpecs.max_height - 3 - FontMarginBottom, ALIGN_LEFT, u"Error: Mapper is not responding!", 48, font, (void*)AGB_VRAM+0xA000, FALSE);
				} else {
					DrawText(5, SCREEN_HEIGHT - sFontSpecs.max_height - 3 - FontMarginBottom, ALIGN_LEFT, u"Error: Game couldn't be launched!", 48, font, (void*)AGB_VRAM+0xA000, FALSE);
				}
			} else if (show_credits) {
				LoadFont(0);
				memset(temp_unicode, 0, sizeof(temp_unicode));
				snprintf(temp_ascii, 48, "Menu by LK - %s", BUILDTIME);
				AsciiToUnicode(temp_ascii, temp_unicode);
				DrawText(6, SCREEN_HEIGHT - sFontSpecs.max_height - 3 - FontMarginBottom, ALIGN_LEFT, temp_unicode, 48, font, (void*)AGB_VRAM+0xA000, FALSE);
			} else if (show_debug) {
				LoadFont(0);
				u8 a = ((sItemConfig.rom_offset / 0x40) & 0xF) << 4;
				u8 b = 0x40 + (sItemConfig.rom_offset % 0x40);
				u8 c = 0x40 - sItemConfig.rom_size;
				snprintf(temp_ascii, 64, "%02X:%02X:%02X|0x%X~%dMiB|%X", a, b, c, (int)(sItemConfig.rom_offset * 512 * 1024), (int)(sItemConfig.rom_size * 512 >> 10), (int)(flash_save_sector_offset + sItemConfig.save_index));
				memset(temp_unicode, 0, sizeof(temp_unicode));
				AsciiToUnicode(temp_ascii, temp_unicode);
				DrawText(6, SCREEN_HEIGHT - sFontSpecs.max_height - 3 - FontMarginBottom, ALIGN_LEFT, temp_unicode, 64, font, (void*)AGB_VRAM+0xA000, FALSE);
			}
			
			// VRAM bank swapping
			REG_DISPCNT ^= 0x0010;
			dmaCopy((void*)AGB_VRAM+0xA000, (void*)AGB_VRAM, SCREEN_WIDTH * SCREEN_HEIGHT);
			REG_DISPCNT ^= 0x0010;
			redraw_items = 0;
		}
		
		// Check for menu keys
		scanKeys();
		kHeld = keysHeld();
		if ((kHeld_boot == 0) || (kHeld != kHeld_boot)) {
			kHeld_boot = 0;
		} else {
			kHeld = 0;
		}
		if (kHeld != 0) {
			wait++;
		} else {
			f = 0;
		}
		if (((kHeld & 0x3FF) && !f) || (wait > 1000)) {
			if (!f) {
				wait = -8000;
			} else {
				wait = 0;
			}
			f = 1;
			
			if (boot_failed) {
				SystemCall(0); // Soft reset
			}

			if ((kHeld & KEY_A) || (kHeld & KEY_START)) {
				sFlashStatus.last_boot_menu_index = page_active * 8 + cursor_pos;
				if (!show_credits && !show_debug) {
					LoadFont(0);
					DrawText(5, SCREEN_HEIGHT - sFontSpecs.max_height - 3 - FontMarginBottom, ALIGN_LEFT, u"Loading… Don't turn off the power!", 48, font, (void*)AGB_VRAM+0xA000, FALSE);
					REG_DISPCNT ^= 0x0010;
					dmaCopy((void*)AGB_VRAM+0xA000, (void*)AGB_VRAM, SCREEN_WIDTH * SCREEN_HEIGHT);
					REG_DISPCNT ^= 0x0010;
				}
				if (kHeld & KEY_SELECT) {
					// Skips reading latest save data from SRAM
					sFlashStatus.last_boot_save_type = SRAM_NONE;
				}
				u8 error_code = BootGame(sItemConfig, sFlashStatus);
				boot_failed = error_code;
				redraw_items = 0xFF;
				REG_IE = 1;

			} else if (kHeld & KEY_B) {
				SystemCall(0); // Soft reset

			} else if ((kHeld & KEY_LEFT) || (kHeld & KEY_RIGHT)) {
				if (kHeld & KEY_LEFT) {
					page_active--;
				} else if (kHeld & KEY_RIGHT) {
					page_active++;
				}
				if (page_active > page_total - 1) page_active = 0;
				if (page_active < 0) page_active = page_total - 1;
				redraw_items = 0xFF;

			} else if ((kHeld & KEY_UP) || (kHeld & KEY_DOWN)) {
				redraw_items |= 1 << cursor_pos;
				if (kHeld & KEY_UP)
					cursor_pos--;
				else if (kHeld & KEY_DOWN)
					cursor_pos++;
				
				if (cursor_pos < 0) {
					cursor_pos = 7;
					page_active--;
					redraw_items = 0xFF;
				} else if (cursor_pos > roms_page) {
					cursor_pos = 0;
					page_active++;
					redraw_items = 0xFF;
				}
				if (page_active > page_total - 1) page_active = 0;
				if (page_active < 0) page_active = page_total - 1;
				redraw_items |= 1 << cursor_pos;
			}
		}
	}
	
	while (1) { VBlankIntrWait(); }
}
