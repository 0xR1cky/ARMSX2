
#pragma once

enum class MemcardPS2Mode
{
	NOT_SET = 0xff,
	PROBE = 0x11,
	UNKNOWN_WRITE_DELETE_END = 0x12,
	SET_ERASE_SECTOR = 0x21,
	SET_WRITE_SECTOR = 0x22,
	SET_READ_SECTOR = 0x23,
	GET_SPECS = 0x26,
	SET_TERMINATOR = 0x27,
	GET_TERMINATOR = 0x28,
	WRITE_DATA = 0x42,
	READ_DATA = 0x43,
	READ_WRITE_END = 0x81,
	ERASE_BLOCK = 0x82,
	UNKNOWN_BOOT = 0xbf,
	AUTH_XOR = 0xf0,
	AUTH_F3 = 0xf3,
	AUTH_F7 = 0xf7,
};

enum class Terminator
{
	DEFAULT = 0x55
};

// Size of a sector, counted in bytes. Datatype is u16.
enum class SectorSize
{
	STANDARD = 0x0200
};

// Size of an erase block, counted in sectors. Datatype is u16.
enum class EraseBlockSize
{
	STANDARD = 0x10
};

// Size of a memcard, counted in sectors. Datatype is u32.
enum class SectorCount
{
	STANDARD = 0x00004000
};

constexpr size_t ECC_BYTES = 16;
