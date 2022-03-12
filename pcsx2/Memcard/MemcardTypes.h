
#pragma once

#include <array>

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

// Transparent to pretty much anything except managing the FAT,
// this describes how many pages make one cluster. Pretty much
// every aspect of the memcard is addressed by page, yet for
// whatever reason the FAT counts things by cluster. Known options
// are 1 or 2, but cards have only been observed in the wild with 2.
// I would guess changing to 1 has some implications for read/write
// efficiency but I don't know if it would be a good or a bad thing.
// 
// For consistency sake, we'll enforce the standard 2 pages per cluster.
enum class ClusterSize
{
	STANDARD = 0x02
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
static constexpr size_t BASE_SECTOR_SIZE_WITH_ECC = static_cast<u16>(SectorSize::STANDARD) + ECC_BYTES;
static constexpr size_t BASE_PS1_SIZE = static_cast<u8>(SectorSize::PS1) * static_cast<u16>(SectorCount::PS1);
static constexpr size_t BASE_8MB_SIZE = BASE_SECTOR_SIZE_WITH_ECC * static_cast<u32>(SectorCount::STANDARD);
static constexpr size_t MAX_2GB_SIZE = BASE_SECTOR_SIZE_WITH_ECC * static_cast<u32>(SectorCount::X256);

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
static const std::string FOLDER_MEMCARD_INDEX_NAME = "_pcsx2_index";
static const char* SUPERBLOCK_FORMATTED_STRING = "Sony PS2 Memory Card Format ";
static constexpr size_t SUPERBLOCK_FORMATTED_STRING_LENGTH = 28;
// The default Indirect FAT Cluster List in the superblock. Only one entry is defined.
static constexpr u32 SUPERBLOCK_DEFAULT_IFC_LIST = 8;

// Though there are 32 positions reserved for these in an IFAT,
// only one cluster is used on a standard 8 MB card. As capacity
// increases, formatting a memcard will use more and more of these
// positions in order to define its FAT locations.
static constexpr size_t INDIRECT_FAT_CLUSTER_COUNT = 32;
// Size of a cluster in bytes. Applies standard ClusterSize multiplier to standard SectorSize.
static constexpr size_t STANDARD_CLUSTER_SIZE = ((static_cast<u16>(SectorSize::STANDARD) + ECC_BYTES) * static_cast<u16>(ClusterSize::STANDARD));
// Number of clusters on a standard 8 MB card. Used for folder memcards.
static constexpr size_t STANDARD_CLUSTERS_ON_CARD = 8192;

// The location of the Indirect FAT, based on superblock's IFC list and the cluster size.
// Size in bytes.
static constexpr u32 STANDARD_IFAT_OFFSET = SUPERBLOCK_DEFAULT_IFC_LIST * STANDARD_CLUSTER_SIZE;

// The indirect FAT which will appear on any 8 MB card.
// The memory card spec allows the FAT to be placed anywhere on the memcard,
// and it can also be fragmented. However, no 8 MB memcard has been spotted
// in the wild using a non-standard starting location for the FAT, nor
// fragmenting it. As such, it ends up being that this table remains an absolute
// truth for 8 MB memcard sizes, and can be systematically relied on. In our case,
// we will inject this into the standard "Indirect FAT" cluster when loading folder
// memcards off the host filesystem.
static constexpr std::array<u8, INDIRECT_FAT_CLUSTER_COUNT> STANDARD_INDIRECT_FAT =
{
	0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 
	0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
	0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
	0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28
};

// Position of the FAT, in bytes, relative to the front of a 8 MB memcard. Used for folder memcards.
static constexpr u32 STANDARD_FAT_OFFSET = STANDARD_CLUSTER_SIZE * (STANDARD_INDIRECT_FAT.at(0));

namespace FAT
{
	// Value found in a PS2 memcard FAT. Indicates that cluster is currently unused.
	static constexpr u32 AVAILABLE = 0x7fffffff;
	// Value found in a PS2 memcard FAT. Indicates that cluster is the last of a file/directory.
	static constexpr u32 LAST_CLUSTER = 0xffffffff;
	// Mask for the "in use" bit of a FAT entry
	static constexpr u32 IN_USE_MASK = 0x80000000;
}

// Used in a directory entry's "cluster" attribute when the directory entry is for a file,
// but the file is an empty file.
static constexpr u32 EMPTY_FILE_CLUSTER_VALUE = 0xffffffff;

static constexpr std::ios_base::openmode MEMCARD_OPEN_MODE = std::ios_base::in | std::ios_base::out | std::ios_base::binary;

// Cluster index of the first "data cluster" on a standard 8 MB card.
static constexpr size_t STANDARD_DATA_OFFSET_CLUSTERS = 41;
// Byte offset of the first data cluster.
static constexpr u32 STANDARD_DATA_OFFSET = STANDARD_CLUSTER_SIZE * STANDARD_DATA_OFFSET_CLUSTERS;

enum class DirectoryModeFlag
{
	READ = 0x0001,
	WRITE = 0x0002,
	EXECUTE = 0x0004,
	PROTECTED = 0x0008,
	FILE = 0x0010,
	DIRECTORY = 0x0020,
	INTERNAL_DIRECTORY_HELPER = 0x0040,
	UNKNOWN_COPIED = 0x0080, // Unknown, but suspected to indicate if a dir entry was copied
	UNKNOWN_100 = 0x0100,
	INTERNAL_CREATE_HELPER = 0x0200,
	INTERNAL_CREATE = 0x0400, // Set when files and directories are created, otherwise ignored.
	POCKETSTATION = 0x0800, // Pocketstation application file
	PSX = 0x1000, // PlayStation 1 save file
	HIDDEN = 0x2000,
	UNKNOWN_4000 = 0x4000,
	IN_USE = 0x8000 // If clear, file or directory has been deleted
};

static constexpr u16 DEFAULT_DIRECTORY_MODE_FLAGS = static_cast<u16>(DirectoryModeFlag::READ) | static_cast<u16>(DirectoryModeFlag::WRITE) | static_cast<u16>(DirectoryModeFlag::EXECUTE) | static_cast<u16>(DirectoryModeFlag::DIRECTORY) | static_cast<u16>(DirectoryModeFlag::INTERNAL_CREATE) | static_cast<u16>(DirectoryModeFlag::IN_USE);
static constexpr u16 DEFAULT_FILE_MODE_FLAGS = static_cast<u16>(DirectoryModeFlag::READ) | static_cast<u16>(DirectoryModeFlag::WRITE) | static_cast<u16>(DirectoryModeFlag::EXECUTE) | static_cast<u16>(DirectoryModeFlag::FILE) | static_cast<u16>(DirectoryModeFlag::INTERNAL_CREATE) | static_cast<u16>(DirectoryModeFlag::IN_USE);
static constexpr u16 SINGLE_DOT_MODE_FLAGS = DEFAULT_DIRECTORY_MODE_FLAGS;
static constexpr u16 DOUBLE_DOT_MODE_FLAGS = static_cast<u16>(DirectoryModeFlag::WRITE) | static_cast<u16>(DirectoryModeFlag::EXECUTE) | static_cast<u16>(DirectoryModeFlag::DIRECTORY) | static_cast<u16>(DirectoryModeFlag::INTERNAL_CREATE) | static_cast<u16>(DirectoryModeFlag::HIDDEN) | static_cast<u16>(DirectoryModeFlag::IN_USE);
