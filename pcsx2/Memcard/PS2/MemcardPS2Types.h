
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
// 
// The PS2 spec allows for sizes of 0x200 and 0x400, but
// no others. However, there are no (documented) cases of
// memcards either first or third party using the 0x400
// sector size. In order to make sector counts inferrable
// by a memcard file's size, we are going to enforce this
// as the only sector size option.
enum class SectorSize
{
	STANDARD = 0x0200
};

// Size of an erase block, counted in sectors. Datatype is u16.
// 
// Could be modified, presumably in powers of 2, to affect I/O
// rates. No (documented) cases of memcards using non-standard
// erase block sizes, so we will not provide options for now.
enum class EraseBlockSize
{
	STANDARD = 0x10
};

// Size of a memcard, counted in sectors. Datatype is u32.
// 
// Memory cards by Sony are always 8 MiB of raw capacity (excluding ECC).
// Third party memory cards have been spotted in the wild up to 256 MiB.
// The PS2 memory card file system has a theoretical upper limit of 2 GiB;
// this size uses all available positions in the indirect FAT cluster list,
// in order to describe an indirect FAT large enough to describe a FAT, large
// enough to describe the directory tree, large enough to contain the data of
// the memcard's writeable portion. That was a run-on but I think it gets the
// point across.
// 
// Memory cards in PCSX2 are not immune to (all) the stability issues that real
// PS2 memory cards had. Certain games will reject cards larger than 8 MiB, or
// do dangerous I/O which can brick the memcard if it is not a standard size.
// The only scenario we are safe from is a third party memcard which used low
// quality NAND flash that was error prone and would corrupt data just from
// normal operation. This is an important consideration, because 
// 
// The PS2 memcard file system has its 2 GiB upper limit, but it does seem to retain
// some basic functionality up to even 8 GiB and can successfully format itself,
// successfully reporting up to 8 GiB of capacity in the BIOS. However, because the
// capacity reported based on the sector count information does not actually match
// the capacity available in the FAT, the memcard will almost certainly fail I/O
// operations in some capacity and behave unpredictably. At best, one or a few games
// may be able to operate on it but any sustained use will inevitably kill save files
// at best, the entire memcard at worst.
enum class SectorCount
{
	STANDARD = 0x00004000,	// 8 MiB
	X2 = 0x00008000,		// 16 MiB
	X4 = 0x00010000,		// 32 MiB
	X8 = 0x00020000,		// 64 MiB
	X16 = 0x00040000,		// 128 MiB
	X32 = 0x00080000,		// 256 MiB
	X64 = 0x00100000,		// 512 MiB
	X128 = 0x00200000,		// 1 GiB
	X256 = 0x00400000		// 2 GiB
};

constexpr size_t SECTOR_READ_SIZE = 128;
constexpr size_t ECC_BYTES = 16;
