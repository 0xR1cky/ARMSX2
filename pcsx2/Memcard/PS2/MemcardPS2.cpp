
#include "PrecompiledHeader.h"
#include "MemcardPS2.h"

#include "IopCommon.h"
#include "fmt/format.h"
#include "Memcard/MemcardConfig.h"
#include <string>

MemcardPS2::MemcardPS2(int port, int slot)
{
	this->port = port;
	this->slot = slot;
	const size_t sizeBytes = (static_cast<u16>(sectorSize) + ECC_BYTES) * static_cast<u32>(sectorCount);
	memcardData = std::vector<u8>(sizeBytes, 0xff);
	SoftReset();
}

MemcardPS2::~MemcardPS2()
{
	if (stream.is_open())
	{
		stream.close();
	}
}

void MemcardPS2::SoftReset()
{
	terminator = static_cast<u8>(Terminator::DEFAULT);
	sectorSize = SectorSize::STANDARD;
	eraseBlockSize = EraseBlockSize::STANDARD;
	sectorCount = SectorCount::STANDARD;
	sector = 0;
}

void MemcardPS2::FullReset()
{
	if (stream.is_open())
	{
		stream.close();
	}

	SoftReset();
	InitializeOnFileSystem();
	LoadFromFileSystem();
}

bool MemcardPS2::IsSlottedIn()
{
	return isSlottedIn;
}

void MemcardPS2::SetSlottedIn(bool value)
{
	isSlottedIn = value;
}

void MemcardPS2::InitializeOnFileSystem()
{
	if (memcardData.size() == 0)
	{
		DevCon.Warning("%s() Attempted to initialize memcard on file system, but memcardData is not yet populated! That should be done prior to writing the data to disk!", __FUNCTION__);
		return;
	}
		
	directory = g_MemcardConfig.GetMemcardsFolder();
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

void MemcardPS2::LoadFromFileSystem()
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

void MemcardPS2::WriteSectorToFileSystem(u32 address, size_t length)
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

u8 MemcardPS2::GetTerminator()
{
	return terminator;
}

SectorSize MemcardPS2::GetSectorSize()
{
	return sectorSize;
}

EraseBlockSize MemcardPS2::GetEraseBlockSize()
{
	return eraseBlockSize;
}

SectorCount MemcardPS2::GetSectorCount()
{
	return sectorCount;
}

u32 MemcardPS2::GetSector()
{
	return sector;
}

void MemcardPS2::SetTerminator(u8 data)
{
	terminator = data;
}

void MemcardPS2::SetSector(u32 data)
{
	sector = data;
}

std::queue<u8> MemcardPS2::ReadSector()
{
	const size_t sectorSizeWithECC = (static_cast<u16>(sectorSize) + ECC_BYTES);
	const u32 address = sector * sectorSizeWithECC;
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
		for (size_t i = 0; i < sectorSizeWithECC; i++)
		{
			ret.push(memcardData.at(address + i));
		}
	}
	else
	{
		DevCon.Warning("%s() Calculated read address out of bounds (%08X > %08X)", __FUNCTION__, address + sectorSizeWithECC, memcardData.size());
	}

	return ret;
}

void MemcardPS2::WriteSector(std::queue<u8>& data)
{
	const size_t sectorSizeWithECC = (static_cast<u16>(sectorSize) + ECC_BYTES);
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

	if (address + sectorSizeWithECC <= memcardData.size())
	{
		for (size_t i = 0; i < sectorSizeWithECC; i++)
		{
			u8 toWrite = 0xff;

			if (!data.empty())
			{
				toWrite = data.front();
				data.pop();
			}

			memcardData.at(address + i) = toWrite;
		}

		WriteSectorToFileSystem(address, sectorSizeWithECC);
	}
	else
	{
		DevCon.Warning("%s(queue) Calculated write address out of bounds (%08X > %08X)", __FUNCTION__, address + sectorSizeWithECC, memcardData.size());
	}
}

void MemcardPS2::EraseBlock()
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

		WriteSectorToFileSystem(address, eraseBlockSizeWithECC);
	}
	else
	{
		DevCon.Warning("%s() Calculated erase address out of bounds (%08X > %08X)", __FUNCTION__, address + eraseBlockSizeWithECC, memcardData.size());
	}
}
