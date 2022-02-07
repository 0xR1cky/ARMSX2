
#pragma once

enum class MemcardType
{
	PS1 = 0x00,
	POCKETSTATION = 0x01,
	PS2 = 0x02,
	EJECTED = 0xff
};

enum class MemcardHostType
{
	NOT_SET = 0xff,
	FILE = 0x00,
	FOLDER = 0x01
};

static constexpr size_t FOLDER_MEMCARD_SUPERBLOCK_SIZE = 8192;

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
// as the only sector size option for PS2.
//
// PS1 enforces a strict sector size of 128 bytes, with no
// ability to change whatsoever.
enum class SectorSize
{
	PS1 = 0x80,
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
// normal operation.
//
// The PS2 memcard file system has its 2 GiB upper limit, but it does seem to retain
// some basic functionality up to even 8 GiB and can successfully format itself,
// successfully reporting up to 8 GiB of capacity in the BIOS. However, because the
// capacity reported based on the sector count information does not actually match
// the capacity available in the FAT, the memcard will almost certainly fail I/O
// operations in some capacity and behave unpredictably. At best, one or a few games
// may be able to operate on it but any sustained use will inevitably kill save files
// at best, the entire memcard at worst.
//
// PS1 enforces a strict 1024 sector count, with no ability to change whatsoever.
enum class SectorCount
{
	PS1 = 0x0400, // 128 KiB PS1 Memcards only
	STANDARD = 0x00004000, // 8 MiB
	X2 = 0x00008000, // 16 MiB
	X4 = 0x00010000, // 32 MiB
	X8 = 0x00020000, // 64 MiB
	X16 = 0x00040000, // 128 MiB
	X32 = 0x00080000, // 256 MiB
	X64 = 0x00100000, // 512 MiB
	X128 = 0x00200000, // 1 GiB
	X256 = 0x00400000 // 2 GiB
};

static constexpr size_t ECC_BYTES = 16;
static constexpr size_t BASE_PS1_SIZE = static_cast<u8>(SectorSize::PS1) * static_cast<u16>(SectorCount::PS1);
static constexpr size_t BASE_8MB_SIZE = (static_cast<u16>(SectorSize::STANDARD) + ECC_BYTES) * static_cast<u32>(SectorCount::STANDARD);
static constexpr size_t MAX_2GB_SIZE = (static_cast<u16>(SectorSize::STANDARD) + ECC_BYTES) * static_cast<u32>(SectorCount::X256);

enum class MemcardPS1Mode
{
	NOT_SET = 0x00,
	INIT = 0x81,
	READ = 0x52,
	STATE = 0x53,
	WRITE = 0x57,
	PS_STATE = 0x58,
	DONE = 0x7f,
	INVALID = 0xff
};

namespace Flag
{
	static constexpr u8 WriteError = 0x04;
	static constexpr u8 DirectoryRead = 0x08;
}

static constexpr size_t PS1_MEMCARD_SIZE = static_cast<u16>(SectorSize::PS1) * static_cast<u16>(SectorCount::PS1);

// 128 KB read size, since that's the minimum size of a memcard file (PS1)
// and a nice factor of all others (8 MB -> 2 GB)
static constexpr size_t STREAM_BATCH_SIZE = 1024 * 128;

static const std::string FOLDER_MEMCARD_SUPERBLOCK_NAME = "_pcsx2_superblock";
