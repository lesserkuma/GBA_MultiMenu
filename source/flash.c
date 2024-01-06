/*
GBA Multi Game Menu
Author: Lesserkuma (github.com/lesserkuma)
*/

#include <gba.h>
#include <stdio.h>
#include <string.h>

#include "main.h"
#include "flash.h"

u8 flash_type;
u8 *itemlist;
u16 itemlist_offset;
u32 flash_sector_size;
u32 flash_itemlist_sector_offset;
u32 flash_status_sector_offset;
u32 flash_save_sector_offset;
EWRAM_BSS u8 sram_register_backup[4];
EWRAM_BSS u8 data_buffer[SRAM_SIZE];

void FlashCalcOffsets(void)
{
	u32 own_size = (u32)(&__rom_end__) - 0x8000000;
	flash_itemlist_sector_offset = own_size;
	flash_itemlist_sector_offset = 0x40000 - (flash_itemlist_sector_offset % 0x40000) + flash_itemlist_sector_offset;
	flash_itemlist_sector_offset = _DIV_CEIL(flash_itemlist_sector_offset, flash_sector_size);
	flash_status_sector_offset = flash_itemlist_sector_offset + 1;
	flash_save_sector_offset = flash_status_sector_offset + 1;
	itemlist = (u8 *)(AGB_ROM + flash_itemlist_sector_offset * flash_sector_size);
}

IWRAM_CODE void FlashDetectType(void)
{
	u32 data;
	u16 ie = REG_IE;
	REG_IE = ie & 0xFFFE;

	// 2G cart with 6600M0U0BE (369-in-1)
	_FLASH_WRITE(0, 0xFF);
	_FLASH_WRITE(0, 0x90);
	data = *(vu32 *)AGB_ROM;
	_FLASH_WRITE(0, 0xFF);
	if (data == 0x88B0008A)
	{
		REG_IE = ie;
		flash_type = 1;
		flash_sector_size = 0x40000;
		FlashCalcOffsets();
		return;
	}

	// 512M cart with MSP55LV100S (Zelda Classic Collection 7-in-1)
	_FLASH_WRITE(0, 0xF0F0);
	_FLASH_WRITE(0xAAA, 0xAAA9);
	_FLASH_WRITE(0x555, 0x5556);
	_FLASH_WRITE(0xAAA, 0x9090);
	data = *(vu32 *)AGB_ROM;
	_FLASH_WRITE(0, 0xF0F0);
	if (data == 0x7E7D0102)
	{
		REG_IE = ie;
		flash_type = 2;
		flash_sector_size = 0x20000;
		FlashCalcOffsets();
		return;
	}

	// 1G cart with MSP54LV100S (Zelda Classic Collection 7-in-1)
	_FLASH_WRITE(0, 0xF0);
	_FLASH_WRITE(0xAAA, 0xA9);
	_FLASH_WRITE(0x555, 0x56);
	_FLASH_WRITE(0xAAA, 0x90);
	data = *(vu32 *)AGB_ROM;
	_FLASH_WRITE(0, 0xF0);
	if (data == 0x227D0002)
	{
		REG_IE = ie;
		flash_type = 3;
		flash_sector_size = 0x20000;
		FlashCalcOffsets();
		return;
	}

	// Unknown type
	REG_IE = ie;
	flash_type = 0;
	flash_sector_size = 0x20000;
	FlashCalcOffsets();
	return;
}

IWRAM_CODE void FlashEraseSector(u32 address)
{
	if (flash_type == 0)
	{
		FlashDetectType();
	}
	vu8 _flash_type = flash_type;
	vu16 ie = REG_IE;
	REG_IE = ie & 0xFFFE;

	if (_flash_type == 1)
	{
		_FLASH_WRITE(address, 0xFF);
		_FLASH_WRITE(address, 0x60);
		_FLASH_WRITE(address, 0xD0);
		_FLASH_WRITE(address, 0x20);
		_FLASH_WRITE(address, 0xD0);
		while (1)
		{
			__asm("nop");
			if ((*((vu16 *)(AGB_ROM + address)) & 0x80) == 0x80)
			{
				break;
			}
		}
		_FLASH_WRITE(address, 0xFF);
	}
	else if (_flash_type == 2)
	{
		_FLASH_WRITE(0xAAA, 0xAAA9);
		_FLASH_WRITE(0x555, 0x5556);
		_FLASH_WRITE(0xAAA, 0x8080);
		_FLASH_WRITE(0xAAA, 0xAAA9);
		_FLASH_WRITE(0x555, 0x5556);
		_FLASH_WRITE(address, 0x3030);
		while (1)
		{
			__asm("nop");
			if ((*((vu16 *)(AGB_ROM + address))) == 0xFFFF)
			{
				break;
			}
		}
		_FLASH_WRITE(address, 0xF0F0);
	}
	else if (_flash_type == 3)
	{
		_FLASH_WRITE(0xAAA, 0xA9);
		_FLASH_WRITE(0x555, 0x56);
		_FLASH_WRITE(0xAAA, 0x80);
		_FLASH_WRITE(0xAAA, 0xA9);
		_FLASH_WRITE(0x555, 0x56);
		_FLASH_WRITE(address, 0x30);
		while (1)
		{
			__asm("nop");
			if ((*((vu16 *)(AGB_ROM + address))) == 0xFFFF)
			{
				break;
			}
		}
		_FLASH_WRITE(address, 0xF0);
	}

	REG_IE = ie;
}

IWRAM_CODE void FlashWriteData(u32 address, u32 length)
{
	if (flash_type == 0)
	{
		FlashDetectType();
	}
	u8 _flash_type = flash_type;
	vu16 *p_rom = (vu16 *)(AGB_ROM + address);
	vu16 ie = REG_IE;
	REG_IE = ie & 0xFFFE;

	if (_flash_type == 1)
	{
		for (int j = 0; j < (int)(length / 0x400); j++)
		{
			_FLASH_WRITE(address + (j * 0x400), 0xEA);
			while (1)
			{
				__asm("nop");
				if ((p_rom[(j * 0x200)] & 0x80) == 0x80)
				{
					break;
				}
			}
			_FLASH_WRITE(address + (j * 0x400), 0x1FF);
			for (int i = 0; i < 0x400; i += 2)
			{
				_FLASH_WRITE(address + (j * 0x400) + i, data_buffer[(j * 0x400) + i + 1] << 8 | data_buffer[(j * 0x400) + i]);
			}
			_FLASH_WRITE(address + (j * 0x400), 0xD0);
			while (1)
			{
				__asm("nop");
				if ((p_rom[(j * 0x200)] & 0x80) == 0x80)
				{
					break;
				}
			}
		}
		_FLASH_WRITE(address, 0xFF);
	}
	else if (_flash_type == 2)
	{
		for (int j = 0; j < (int)(length / 0x20); j++)
		{
			_FLASH_WRITE(0xAAA, 0xAAA9);
			_FLASH_WRITE(0x555, 0x5556);
			_FLASH_WRITE(address + (j * 0x20), 0x2526);
			_FLASH_WRITE(address + (j * 0x20), 0x0F0F);
			u16 data = 0;
			for (int i = 0; i < 0x20; i += 2)
			{
				__asm("nop");
				data = data_buffer[(j * 0x20) + i + 1] << 8 | data_buffer[(j * 0x20) + i];
				_FLASH_WRITE(address + (j * 0x20) + i, data);
			}
			_FLASH_WRITE(address + (j * 0x20), 0x292A);
			while (1)
			{
				__asm("nop");
				if (p_rom[(j * 0x10) + 0x0F] == data)
				{
					break;
				}
			}
		}
		_FLASH_WRITE(address, 0xF0F0);
	}
	else if (_flash_type == 3)
	{
		for (int j = 0; j < (int)(length / 0x40); j++)
		{
			_FLASH_WRITE(0xAAA, 0xA9);
			_FLASH_WRITE(0x555, 0x56);
			_FLASH_WRITE(address + (j * 0x40), 0x26);
			_FLASH_WRITE(address + (j * 0x40), 0x1F);
			u16 data = 0;
			for (int i = 0; i < 0x40; i += 2)
			{
				__asm("nop");
				data = data_buffer[(j * 0x40) + i + 1] << 8 | data_buffer[(j * 0x40) + i];
				_FLASH_WRITE(address + (j * 0x40) + i, data);
			}
			_FLASH_WRITE(address + (j * 0x40), 0x2A);
			while (1)
			{
				__asm("nop");
				if (p_rom[(j * 0x20) + 0x1F] == data)
				{
					break;
				}
			}
		}
		_FLASH_WRITE(address, 0xF0);
	}

	REG_IE = ie;
}

IWRAM_CODE void DrawBootStatusLine(u8 begin, u8 end)
{
	for (int i = begin; i < end; i++)
	{
		((u16 *)AGB_VRAM)[(120 * 159) + i] = 0;
	}
}

IWRAM_CODE u8 BootGame(ItemConfig config, FlashStatus status)
{
	u32 _flash_sector_size = flash_sector_size;
	u32 _flash_save_block_offset = flash_save_sector_offset;
	u32 _flash_status_block_offset = flash_status_sector_offset;

	// Check if supported flash chip is present
	FlashDetectType();
	u8 _flash_type = flash_type;
	if (_flash_type == 0)
		return 1;

	// Temporarily store SRAM values located at mapper registers
	sram_register_backup[0] = *(vu8 *)MAPPER_CONFIG1;
	sram_register_backup[1] = *(vu8 *)MAPPER_CONFIG2;
	sram_register_backup[2] = *(vu8 *)MAPPER_CONFIG3;
	sram_register_backup[3] = *(vu8 *)MAPPER_CONFIG4;

	// Enable SRAM access
	*(vu8 *)MAPPER_CONFIG4 = 1;

	// Write previous SRAM to flash
	if (status.battery_present)
	{
		if (status.last_boot_save_type != SRAM_NONE)
		{
			for (int i = 0; i < SRAM_SIZE; i++)
			{
				data_buffer[i] = ((vu8 *)AGB_SRAM)[i];
			}
			data_buffer[2] = sram_register_backup[0];
			data_buffer[3] = sram_register_backup[1];
			data_buffer[4] = sram_register_backup[2];
			data_buffer[5] = sram_register_backup[3];
			FlashEraseSector((_flash_save_block_offset + status.last_boot_save_index) * _flash_sector_size);
			FlashWriteData((_flash_save_block_offset + status.last_boot_save_index) * _flash_sector_size, SRAM_SIZE);
		}
	}

	// Save status to flash
	status.last_boot_save_index = config.save_index;
	status.last_boot_save_type = config.save_type;
	memset((void *)data_buffer, 0, 0x1000);
	memcpy(data_buffer, &status, sizeof(status));
	FlashEraseSector(_flash_status_block_offset * flash_sector_size);
	FlashWriteData(_flash_status_block_offset * flash_sector_size, 0x1000);

	// Disable SRAM access
	*(vu8 *)MAPPER_CONFIG4 = 0;

	// Read new SRAM from flash
	if (config.save_type != SRAM_NONE)
	{
		for (int i = 0; i < SRAM_SIZE; i += 2)
		{
			data_buffer[i] = *((vu16 *)(AGB_ROM + ((_flash_save_block_offset + config.save_index) * _flash_sector_size) + i)) & 0xFF;
			data_buffer[i + 1] = *((vu16 *)(AGB_ROM + ((_flash_save_block_offset + config.save_index) * _flash_sector_size) + i)) >> 8;
		}
	}

	// Fade out
	REG_BLDCNT = 0x00FF;
	for (u8 i = 0; i < 17; i++)
	{
		REG_BLDY = i;
		SystemCall(5);
	}

	// Disable interrupts
	REG_IE = 0;

	// Set mapper configuration
	*(vu8 *)MAPPER_CONFIG1 = ((config.rom_offset / 0x40) & 0xF) << 4; // flash bank (0~7)
	*(vu8 *)MAPPER_CONFIG2 = 0x40 + (config.rom_offset % 0x40);		  // ROM offset (in 512 KB blocks) within current flash bank
	*(vu8 *)MAPPER_CONFIG3 = 0x40 - config.rom_size;				  // accessible ROM size (in 512 KB blocks)

	// Wait until menu ROM is no longer visible
	u32 timeout = 0x2FFF;
	while (((vu16 *)AGB_ROM)[0x58] == 0x4B4C)
	{
		if (!timeout--)
		{
			REG_IE = 1;
			REG_BLDY = 0;
			SystemCall(5);
			return 2;
		}
	}

	// Lock mapper
	*(vu8 *)MAPPER_CONFIG2 |= 0x80;

	// Write buffer to SRAM
	if (config.save_type != SRAM_NONE)
	{
		for (int i = 0; i < SRAM_SIZE; i++)
		{
			((vu8 *)AGB_SRAM)[i] = data_buffer[i];
		}
	}
	else
	{
		*(vu8 *)MAPPER_CONFIG1 = sram_register_backup[0];
		*(vu8 *)MAPPER_CONFIG2 = sram_register_backup[1];
		*(vu8 *)MAPPER_CONFIG3 = sram_register_backup[2];
		*(vu8 *)MAPPER_CONFIG4 = sram_register_backup[3];
	}

	// Clear palette
	for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT >> 1; i++)
	{
		((vu16 *)AGB_VRAM)[i] = 0;
	}
	REG_BLDY = 0;

	// Boot ROM
	__asm("swi 0"); // Soft reset

	return 3;
}
