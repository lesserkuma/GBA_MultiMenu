/*
GBA Multi Game Menu
Author: Lesserkuma (github.com/lesserkuma)
*/

#ifndef FLASH_H_
#define FLASH_H_

#include "main.h"

#define _FLASH_WRITE(pa, pd)                     \
    {                                            \
        *(((vu16 *)AGB_ROM) + ((pa) >> 1)) = pd; \
        __asm("nop");                            \
    }

#define MAGIC_FLASH_STATUS 0x414D554B

typedef struct __attribute__((packed)) FlashStatus_
{
    u32 magic;
    u8 version;
    u8 battery_present;
    u16 last_boot_menu_index;
    u8 last_boot_save_index;
    SAVE_TYPE last_boot_save_type;
} FlashStatus;

IWRAM_CODE void FlashDetectType(void);
IWRAM_CODE void FlashEraseSector(u32 address);
IWRAM_CODE void FlashWriteData(u32 address, u32 length);
IWRAM_CODE u8 BootGame(ItemConfig config, FlashStatus status);

#endif
