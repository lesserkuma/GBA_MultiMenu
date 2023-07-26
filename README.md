# GBA Multi Game Menu (by Lesserkuma)

This is a menu program to be run on Game Boy Advance bootleg cartridges which are equipped with a special multi-game mapper.

The binaries are available in the [Releases](https://github.com/lesserkuma/GBA_MultiMenu/releases) section.

## Usage

Place your ROM files and save data files into the `roms` folder, then run the ROM Builder tool. Upon first launch, it will create a config.json automatically which you can then modify further to your liking. To reset the configuration and re-generate a new one, just delete the config.json file.

### Configuration
Open the config.json file in a text editor like Notepad.

The following section must be edited in order to specify the cartridge type to use and whether or not your cartridge has a battery installed:
```json
    "cartridge": {
        "type": 2,
        "battery_present": false
    },
```
Set `type` to `1` or `2`:
- `1` = MSP55LV100S (e.g. The Legend of Zelda Collection - Classic Edition 7-in-1)
- `2` = 6600M0U0BE (e.g. 369IN1 2048M)

Set `battery_present` to `true` or `false`. This will enable enhanced save data handling which will only be functional with a working battery.

In the `games` section, you can edit the game-related stuff:
- `enabled` can be set to `true` or `false`. If this option is set, the game entry will be skipped by the ROM Builder.
- `file` is the ROM's file name within the **roms** folder, including file extension.
- `title` is the unicode title that will be displayed in the menu.
- `title_font` is set to `1` by default. If you have certain non-free fonts installed, the following options can be made available:
  - `1` = Default font (based on [Fusion Pixel](https://github.com/TakWolf/fusion-pixel-font))
  - `2` = Nintendo DS IPL font
  - `3` = Nintendo DSi IPL font (JPN/USA/EUR)
  - `4` = Nintendo DSi IPL font (CHN)
  - `5` = Nintendo DSi IPL font (KOR)
  - `6` = Pokémon Black & White condensed battle font
- `save_slot` defines which save slot your game uses. Set it to `null` for no saving or a number starting from `1`. Multiple games can share a save slot.

### ROM Builder Command Line Arguments

No command line arguments are required for creating a compilation, however there are some optional ones that can tweak some things:

```
--split                 splits output files into 32 MiB parts
--no-wait               don't wait for user input when finished
--no-log                don't write a log file
--config config.json    sets the config file to use
--output output.gba     sets the file name of the compilation ROM
```

## Limitations
- up to 512 ROMs total (depending on cartridge memory)
- smallest ROM size is 512 KiB
- up to 256 MiB combined file size (depending on cartridge memory; also since ROMs need to be aligned in a very specific way, there may be less usable space)
- up to 64 KiB of save data per ROM

### Save Data
If the cartridge has a battery installed, the ROMs must be SRAM-patched with [GBATA](https://www.romhacking.net/utilities/601/) for saving to work.

If the cartridge has no battery installed, the ROMs must be patched for batteryless SRAM saving with maniac's [Automatic batteryless saving patcher](https://github.com/metroid-maniac/gba-auto-batteryless-patcher/).

## Compatibility
Tested repro cartridges:
- 100BS6600_48BALL_V4 with 6600M0U0BE
- 100SOP with MSP55LV100S

ROM and save data can be written and read using a [GBxCart RW v1.4+](https://www.gbxcart.com/) device by insideGadgets and the [FlashGBX](https://github.com/lesserkuma/FlashGBX) software.
