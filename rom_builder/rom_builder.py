# -*- coding: utf-8 -*-
# GBA Multi Game Menu – ROM Builder
# Author: Lesserkuma (github.com/lesserkuma)

import sys, os, glob, json, math, re, struct, hashlib, argparse, datetime

# Configuration
app_version = "0.3"
default_file = "LK_MULTIMENU_<CODE>.gba"

################################

def UpdateSectorMap(start, length, c):
	sector_map[start + 1:start + length] = c * (length - 1)
	sector_map[start] = c.upper()


def formatFileSize(size):
	if size == 1:
		return "{:d} Byte".format(size)
	elif size < 1024:
		return "{:d} Bytes".format(size)
	elif size < 1024 * 1024:
		val = size/1024
		return "{:.1f} KB".format(val)
	else:
		val = size/1024/1024
		return "{:.2f} MB".format(val)

def logp(*args, **kwargs):
	global log
	s = format(" ".join(map(str, args)))
	print("{:s}".format(s), **kwargs)
	if "end" in kwargs and kwargs["end"] == "":
		log += "{:s}".format(s)
	else:
		log += "{:s}\n".format(s)

################################

cartridge_types = [
	{
		"name":"MSP55LV100S",
		"flash_size":0x4000000,
		"sector_size":0x20000,
		"block_size":0x80000,
	},
	{
		"name":"6600M0U0BE",
		"flash_size":0x10000000,
		"sector_size":0x40000,
		"block_size":0x80000,
	},
]
now = datetime.datetime.now()
log = ""

logp("GBA Multi Game Menu ROM Builder v{:s}\nby Lesserkuma\n".format(app_version))
class ArgParseCustomFormatter(argparse.ArgumentDefaultsHelpFormatter, argparse.RawDescriptionHelpFormatter): pass
parser = argparse.ArgumentParser()
parser.add_argument("--split", help="splits output files into 32 MiB parts", action="store_true", default=False)
parser.add_argument("--no-wait", help="don’t wait for user input when finished", action="store_true", default=False)
parser.add_argument("--no-log", help="don’t write a log file", action="store_true", default=False)
parser.add_argument("--config", type=str, default="config.json", help="sets the config file to use")
parser.add_argument("--output", type=str, default=default_file, help="sets the file name of the compilation ROM")
args = parser.parse_args()
output_file = args.output
if output_file == "lk_multimenu.gba":
	logp("Error: The file must not be named “lk_multimenu.gba”")
	if not args.no_wait: input("\nPress ENTER to exit.\n")
	sys.exit(1)
if not os.path.exists("lk_multimenu.gba"):
	logp("Error: The Menu ROM is missing.\nPlease put it in the same directory that you are running this tool from.\nExpected file name: “lk_multimenu.gba”")
	if not args.no_wait: input("\nPress ENTER to exit.\n")
	sys.exit()

# Read game list
files = []
if not os.path.exists(args.config):
	files = glob.glob("roms/*.gba")
	files = sorted(files, key=str.casefold)
	save_slot = 1
	games = []
	cartridge_type = 1
	battery_present = False
	for file in files:
		games.append({
			"enabled": True,
			"file": os.path.split(file)[1],
			"title": os.path.splitext(os.path.split(file)[1])[0],
			"title_font": 1,
			"save_slot": save_slot,
		})
		save_slot += 1
	obj = {
		"cartridge": {
			"type": cartridge_type + 1,
			"battery_present": battery_present,
		},
		"games": games,
	}
	if len(games) == 0:
		logp("Error: No usable ROM files were found in the “roms” folder.")
	else:
		with open(args.config, "w", encoding="UTF-8-SIG") as f:
			f.write(json.dumps(obj=obj, indent=4, ensure_ascii=False))
		logp(f"A new configuration file ({args.config:s}) was created based on the files inside the “roms” folder.\nPlease edit the file to your liking in a text editor, then run this tool again.")
	if not args.no_wait: input("\nPress ENTER to exit.\n")
	sys.exit()
else:
	with open(args.config, "r", encoding="UTF-8-SIG") as f:
		try:
			j = json.load(f)
		except json.decoder.JSONDecodeError as e:
			logp(f"Error: The configuration file ({args.config:s}) is malformed and could not be loaded.\n" + str(e))
			if not args.no_wait: input("\nPress ENTER to exit.\n")
			sys.exit()
		games = j["games"]
		cartridge_type = j["cartridge"]["type"] - 1
		battery_present = j["cartridge"]["battery_present"]

# Prepare compilation
flash_size = cartridge_types[cartridge_type]["flash_size"]
sector_size = cartridge_types[cartridge_type]["sector_size"]
sector_count = flash_size // sector_size
block_size = cartridge_types[cartridge_type]["block_size"]
block_count = flash_size // block_size
sectors_per_block = 0x80000 // sector_size
compilation = bytearray()
for i in range(flash_size // 0x2000000):
	chunk = bytearray([0xFF] * 0x2000000)
	compilation += chunk
sector_map = list("." * sector_count)

# Read menu ROM
with open("lk_multimenu.gba", "rb") as f:
	menu_rom = f.read()
menu_rom_size = menu_rom.find(b"dkARM\0\0\0") + 8
compilation[0:len(menu_rom)] = menu_rom
UpdateSectorMap(start=0, length=math.ceil(len(menu_rom) / sector_size), c="m")
item_list_offset = len(menu_rom)
item_list_offset = 0x40000 - (item_list_offset % 0x40000) + item_list_offset
item_list_offset = math.ceil(item_list_offset / sector_size)
UpdateSectorMap(start=item_list_offset, length=1, c="l")
status_offset = item_list_offset + 1
UpdateSectorMap(start=status_offset, length=1, c="c")
if battery_present:
	status = bytearray([0x4B, 0x55, 0x4D, 0x41, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00])
else:
	status = bytearray([0x4B, 0x55, 0x4D, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00])
compilation[status_offset * sector_size:status_offset * sector_size + len(status)] = status
save_data_sector_offset = status_offset + 1
boot_logo_found = hashlib.sha1(compilation[0x04:0xA0]).digest() == bytearray([ 0x17, 0xDA, 0xA0, 0xFE, 0xC0, 0x2F, 0xC3, 0x3C, 0x0F, 0x6A, 0xBB, 0x54, 0x9A, 0x8B, 0x80, 0xB6, 0x61, 0x3B, 0x48, 0xEE ])

# Read game ROMs and import save data
saves_read = []
games = [game for game in games if "enabled" in game and game["enabled"]]
index = 0
for game in games:
	if not game["enabled"]: continue
	if not os.path.exists(f"roms/{game['file']}"):
		game["missing"] = True
		continue
	size = os.path.getsize(f"roms/{game['file']}")
	if ((size & (size - 1)) != 0):
		x = 0x80000
		while (x < size): x *= 2
		size = x
	game["index"] = index
	game["size"] = size
	if "title_font" in game:
		game["title_font"] -= 1
	else:
		game["title_font"] = 0
	game["sector_count"] = int(size / sector_size)

	if battery_present and game["save_slot"] is not None:
		game["save_type"] = 2
		game["save_slot"] -= 1
		save_slot = game["save_slot"]
		offset = save_data_sector_offset + save_slot
		UpdateSectorMap(offset, 1, "s")
		
		if save_slot not in saves_read:
			save_data_file = os.path.splitext(f"roms/{game['file']}")[0] + ".sav"
			save_data = bytearray([0] * sector_size)
			if os.path.exists(save_data_file):
				with open(save_data_file, "rb") as f:
					save_data = f.read()
				if len(save_data) < sector_size:
					save_data += bytearray([0] * (sector_size - len(save_data)))
				if len(save_data) > sector_size:
					save_data = save_data[:sector_size]
				saves_read.append(save_slot)
			compilation[offset * sector_size:offset * sector_size + sector_size] = save_data
	else:
		game["save_type"] = 0
		game["save_slot"] = 0
	index += 1
if len(saves_read) > 0:
	save_end_offset = (''.join(sector_map).rindex("S") + 1)
else:
	save_end_offset = save_data_sector_offset

games = [game for game in games if not ("missing" in game and game["missing"])]
if len(games) == 0:
	logp(f"No ROMs found. Delete the “{args.config:s}” file to reset your configuration.")
	sys.exit()

# Add index
index = 0
for game in games:
	game["index"] = index
	index += 1

# Read ROM data
games.sort(key=lambda game: game["size"], reverse=True)
c = 0
for game in games:
	found = False
	for i in range(save_end_offset, len(sector_map)):
		sector_count_map = game["sector_count"]
		
		if "map_256m" in game and game["map_256m"] == True:
			# Map as 256M ROM, but don't waste space; some games may need this for unknown reasons
			sector_count_map = (32 * 1024 * 1024) // sector_size
		
		if i % sector_count_map == 0:
			if sector_map[i:i + game["sector_count"]] == ["."] * game["sector_count"]:
				UpdateSectorMap(i, game["sector_count"], "r")
				with open(f"roms/{game['file']}", "rb") as f: rom = f.read()
				compilation[i * sector_size:i * sector_size + len(rom)] = rom
				game["sector_offset"] = i
				game["block_offset"] = game["sector_offset"] * sector_size // block_size
				game["block_count"] = sector_count_map * sector_size // block_size
				found = True
				
				if not boot_logo_found and hashlib.sha1(rom[0x04:0xA0]).digest() == bytearray([ 0x17, 0xDA, 0xA0, 0xFE, 0xC0, 0x2F, 0xC3, 0x3C, 0x0F, 0x6A, 0xBB, 0x54, 0x9A, 0x8B, 0x80, 0xB6, 0x61, 0x3B, 0x48, 0xEE ]):
					compilation[0x04:0xA0] = rom[0x04:0xA0] # boot logo
					boot_logo_found = True
				break
	if not found:
		logp("“{:s}” couldn’t be added because it exceeds the available cartridge space.".format(game["title"]))

if not boot_logo_found:
	logp("Warning: Valid boot logo is missing!")

# Generate item list
games = [game for game in games if "sector_offset" in game]
games.sort(key=lambda game: game["index"])

# Print information
logp("Sector map (1 block = {:d} KiB):".format(sector_size // 1024))
for i in range(0, len(sector_map)):
	logp(sector_map[i], end="")
	if i % 64 == 63: logp("")
sectors_used = len(re.findall(r'[MmSsRrIiCc]', "".join(sector_map)))
logp("{:.2f}% ({:d} of {:d} sectors) used\n".format(sectors_used / sector_count * 100, sectors_used, sector_count))
logp(f"Added {len(games)} ROM(s) to the compilation\n")

if battery_present:
	logp     ("    | Offset    | Map Size  | Save Slot      | Title")
	toc_sep = "----+-----------+-----------+----------------+---------------------------------"
else:
	logp     ("    | Offset    | Map Size  | Title")
	toc_sep = "----+-----------+-----------+--------------------------------------------------"

item_list = bytearray()
for game in games:
	title = game["title"]
	if len(title) > 0x30: title = title[:0x2F] + "…"

	table_line = \
				f"{game['index'] + 1:3d} | " \
				f"0x{game['block_offset'] * block_size:07X} | "\
				f"0x{game['block_count'] * block_size:07X} | "
	if battery_present:
		if game['save_type'] > 0:
			table_line += f"{game['save_slot']+1:2d} (0x{(save_data_sector_offset + game['save_slot']) * sector_size:07X}) | "
		else:
			table_line += "               | "
	table_line += f"{title}"
	if c % 8 == 0: logp(toc_sep)
	logp(table_line)
	c += 1
	
	title = title.ljust(0x30, "\0")
	item_list += bytearray(struct.pack("B", game["title_font"]))
	item_list += bytearray(struct.pack("B", len(game["title"])))
	item_list += bytearray(struct.pack("<H", game["block_offset"]))
	item_list += bytearray(struct.pack("<H", game["block_count"]))
	item_list += bytearray(struct.pack("B", game["save_type"]))
	item_list += bytearray(struct.pack("B", game["save_slot"]))
	item_list += bytearray([0] * 8)
	item_list += bytearray(title.encode("UTF-16LE"))

compilation[item_list_offset * sector_size:item_list_offset * sector_size + len(item_list)] = item_list
rom_code = "L{:s}".format(hashlib.sha1(status + item_list).hexdigest()[:3]).upper()

# Write compilation
rom_size = len("".join(sector_map).rstrip(".")) * sector_size
compilation[0xAC:0xB0] = rom_code.encode("ASCII")
checksum = 0
for i in range(0xA0, 0xBD):
	checksum = checksum - compilation[i]
checksum = (checksum - 0x19) & 0xFF
compilation[0xBD] = checksum
logp("")
logp("Menu ROM:        0x{:07X}–0x{:07X}".format(0, len(menu_rom)))
logp("Game List:       0x{:07X}–0x{:07X}".format(item_list_offset * sector_size, item_list_offset * sector_size + len(item_list)))
logp("Status Area:     0x{:07X}–0x{:07X}".format(status_offset * sector_size, status_offset * sector_size + 0x1000))
logp("")
logp("Cartridge Type:  {:d} ({:s} {:s})".format(cartridge_type, cartridge_types[cartridge_type]["name"], "with battery" if battery_present else "without battery"))
logp("Output ROM Size: {:.2f} MiB".format(rom_size / 1024 / 1024))
logp("Output ROM Code: {:s}".format(rom_code))
output_file = output_file.replace("<CODE>", rom_code)
if args.split:
	for i in range(0, math.ceil(flash_size / 0x2000000)):
		pos = i * 0x2000000
		size = 0x2000000
		if pos > len(compilation[:rom_size]): break
		if pos + size > rom_size: size = rom_size - pos
		output_file_part = "{:s}_part{:d}{:s}".format(os.path.splitext(output_file)[0], i + 1, os.path.splitext(output_file)[1])
		with open(output_file_part, "wb") as f: f.write(compilation[pos:pos+size])
else:
	with open(output_file, "wb") as f: f.write(compilation[:rom_size])

# Write log
if not args.no_log:
	log += "\nArgument List: {:s}\n".format(str(sys.argv[1:]))
	log += "\n################################\n\n"
	with open("log.txt", "ab") as f: f.write(log.encode("UTF-8-SIG"))
if not args.no_wait: input("\nPress ENTER to exit.\n")
