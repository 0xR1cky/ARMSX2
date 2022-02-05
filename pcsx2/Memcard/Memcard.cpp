
#include "PrecompiledHeader.h"
#include "Memcard.h"

#include "IopCommon.h"
#include "fmt/format.h"
#include "Memcard/MemcardConfig.h"
#include "DirectoryHelper.h"

#include <string>

Memcard::Memcard(size_t port, size_t slot)
{
	this->port = port;
	this->slot = slot;
	const size_t sizeBytes = (static_cast<u16>(sectorSize) + ECC_BYTES) * static_cast<u32>(sectorCount);
	memcardData = std::vector<u8>(sizeBytes, 0xff);
	SoftReset();
}

Memcard::~Memcard()
{
	if (stream.is_open())
	{
		stream.close();
	}
}

void Memcard::SoftReset()
{
	terminator = static_cast<u8>(Terminator::DEFAULT);
	sectorSize = SectorSize::STANDARD;
	eraseBlockSize = EraseBlockSize::STANDARD;
	sectorCount = SectorCount::STANDARD;
	sector = 0;
}

void Memcard::FullReset()
{
	SoftReset();

	if (stream.is_open())
	{
		stream.close();
	}

	InitializeOnFileSystem();
	LoadFromFileSystem();
}

void Memcard::InitializeOnFileSystem()
{
	if (memcardData.size() == 0)
	{
		DevCon.Warning("%s() Attempted to initialize memcard on file system, but memcardData is not yet populated! That should be done prior to writing the data to disk!", __FUNCTION__);
		return;
	}

	// TODO: Portable builds, only use the relative path specified in config, do not prefix with home directory
	directory = GetHomeDirectory() / g_MemcardConfig.GetMemcardsFolder();
	fileName = g_MemcardConfig.GetMemcardConfigSlot(port, slot)->GetMemcardFileName();
	fullPath = directory / fileName;
	stream.open(fullPath, std::ios_base::in | std::ios_base::out | std::ios_base::binary);

	if (stream.good())
	{
		// File is already on disk, exit quietly.
		return;
	}

	if (!ghc::filesystem::is_directory(directory) && !ghc::filesystem::create_directories(directory))
	{
		Console.Warning("%s() Failed to create directory for memcard files!", __FUNCTION__);
		return;
	}

	std::ofstream writer;
	writer.open(fullPath);

	if (writer.good())
	{
		const char* buf = reinterpret_cast<char*>(memcardData.data());
		writer.write(buf, memcardData.size());
	}
	else
	{
		Console.Warning("%s() Failed to initialize memcard file (port %d slot %d) on file system!", __FUNCTION__, port, slot);
	}

	writer.close();

	stream.open(fullPath, std::ios_base::in | std::ios_base::out | std::ios_base::binary);

	if (!stream.good())
	{
		Console.Warning("%s() Could not open memcard file (port %d slot %d)!", __FUNCTION__, port, slot);
	}
}

void Memcard::LoadFromFileSystem()
{
	if (!stream.good())
	{
		Console.Warning("%s() Failed to open memcard file (port %d slot %d), ejecting it!", __FUNCTION__, port, slot);
		SetSlottedIn(false);
		return;
	}

	std::vector<char> buf;
	buf.resize(memcardData.size());
	stream.seekg(0);
	stream.read(buf.data(), memcardData.size());
	memcpy(memcardData.data(), buf.data(), memcardData.size());
	SetSlottedIn(true);
}

void Memcard::WriteToFileSystem(u32 address, size_t length)
{
	if (!stream.good())
	{
		Console.Warning("%s(%08x, %d) Failed to open memcard file (port %d slot %d)!", __FUNCTION__, address, length, port, slot);
		Console.Warning("This sector write will persist in memory, but will not be committed to disk!");
		// TODO: Should we eject the card? What's the proper thing to do here...
		return;
	}

	std::vector<char> buf;
	buf.resize(length);
	memcpy(buf.data(), memcardData.data() + address, length);
	stream.seekp(address);
	stream.write(buf.data(), length);
	stream.flush();
}

bool Memcard::IsSlottedIn()
{
	return isSlottedIn;
}

MemcardType Memcard::GetMemcardType()
{
	return memcardType;
}

u8 Memcard::GetFlag()
{
	return flag;
}

u8 Memcard::GetTerminator()
{
	return terminator;
}

SectorSize Memcard::GetSectorSize()
{
	return sectorSize;
}

EraseBlockSize Memcard::GetEraseBlockSize()
{
	return eraseBlockSize;
}

SectorCount Memcard::GetSectorCount()
{
	return sectorCount;
}

u32 Memcard::GetSector()
{
	return sector;
}

void Memcard::SetSlottedIn(bool value)
{
	isSlottedIn = value;
}

void Memcard::SetMemcardType(MemcardType newType)
{
	memcardType = newType;
}

void Memcard::SetFlag(u8 newFlag)
{
	flag = newFlag;
}

void Memcard::SetTerminator(u8 data)
{
	terminator = data;
}

void Memcard::SetSector(u32 data)
{
	sector = data;
	offset = 0;
}

std::queue<u8> Memcard::Read(size_t length)
{
	const size_t sectorSizeWithECC = (static_cast<u16>(sectorSize) + ECC_BYTES);
	const u32 address = (sector * sectorSizeWithECC) + offset;
	std::queue<u8> ret;

	if (sector == 0)
	{
		MEMCARDS_LOG("%s() Superblock (%08X)", __FUNCTION__, sector);
	}
	else if (sector >= 0x10 && sector < 0x12)
	{
		MEMCARDS_LOG("%s() Indirect FAT (%08X)", __FUNCTION__, sector);
	}
	else if (sector >= 0x12 && sector < 0x52)
	{
		MEMCARDS_LOG("%s() FAT (%08X)", __FUNCTION__, sector);
	}

	if (address + sectorSizeWithECC <= memcardData.size())
	{
		for (size_t i = 0; i < length; i++)
		{
			ret.push(memcardData.at(address + i));
		}
	}
	else
	{
		DevCon.Warning("%s() Calculated read address out of bounds (%08X > %08X)", __FUNCTION__, address + sectorSizeWithECC, memcardData.size());
	}

	// Memcard commands issue a single sector assignment, then multiple reads. Offset the sector
	// so the next read starts at the correct offset.
	offset += length;
	return ret;
}

void Memcard::Write(std::queue<u8>& data)
{
	const size_t length = data.size();
	const size_t sectorSizeWithECC = (static_cast<u16>(sectorSize) + ECC_BYTES);
	const u32 address = (sector * sectorSizeWithECC) + offset;

	if (sector == 0)
	{
		MEMCARDS_LOG("%s() Superblock (%08X)", __FUNCTION__, sector);
	}
	else if (sector >= 0x10 && sector < 0x12)
	{
		MEMCARDS_LOG("%s() Indirect FAT (%08X)", __FUNCTION__, sector);
	}
	else if (sector >= 0x12 && sector < 0x52)
	{
		MEMCARDS_LOG("%s() FAT (%08X)", __FUNCTION__, sector);
	}

	if (address + data.size() <= memcardData.size())
	{
		size_t bytesWritten = 0;

		while (!data.empty())
		{
			const u8 toWrite = data.front();
			data.pop();

			memcardData.at(address + bytesWritten++) = toWrite;
		}

		WriteToFileSystem(address, sectorSizeWithECC);
	}
	else
	{
		DevCon.Warning("%s(queue) Calculated write address out of bounds (%08X > %08X)", __FUNCTION__, address + sectorSizeWithECC, memcardData.size());
	}

	// Memcard commands issue a single sector assignment, then multiple writes. Offset the sector
	// so the next write starts at the correct offset.
	offset += length;
}

void Memcard::EraseBlock()
{
	const size_t sectorSizeWithECC = (static_cast<u16>(sectorSize) + ECC_BYTES);
	const size_t eraseBlockSizeWithECC = sectorSizeWithECC * static_cast<u16>(eraseBlockSize);
	const u32 address = sector * sectorSizeWithECC;

	if (sector == 0)
	{
		MEMCARDS_LOG("%s() Superblock (%08X)", __FUNCTION__, sector);
	}
	else if (sector >= 0x10 && sector < 0x12)
	{
		MEMCARDS_LOG("%s() Indirect FAT (%08X)", __FUNCTION__, sector);
	}
	else if (sector >= 0x12 && sector < 0x52)
	{
		MEMCARDS_LOG("%s() FAT (%08X)", __FUNCTION__, sector);
	}

	if (address + eraseBlockSizeWithECC <= memcardData.size())
	{
		for (size_t i = 0; i < eraseBlockSizeWithECC; i++)
		{
			memcardData.at(address + i) = 0xff;
		}

		WriteToFileSystem(address, eraseBlockSizeWithECC);
	}
	else
	{
		DevCon.Warning("%s() Calculated erase address out of bounds (%08X > %08X)", __FUNCTION__, address + eraseBlockSizeWithECC, memcardData.size());
	}
}
