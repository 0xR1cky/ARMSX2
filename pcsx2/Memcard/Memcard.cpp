
#include "PrecompiledHeader.h"
#include "Memcard.h"

#include "IopCommon.h"
#include "fmt/format.h"
#include "Memcard/MemcardConfig.h"
#include "DirectoryHelper.h"
#include "common/FileSystem.h"
#include "MemcardFileIO.h"
#include "MemcardFolderIO.h"

#include <string>
#include <array>

Memcard::Memcard(size_t port, size_t slot)
{
	this->port = port;
	this->slot = slot;
	SoftReset();
}

Memcard::~Memcard() = default;

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
/*
	if (memcardData.size() == 0)
	{
		DevCon.Warning("%s() Attempted to initialize memcard on file system, but memcardData is not yet populated! That should be done prior to writing the data to disk!", __FUNCTION__);
		return;
	}
*/
	// TODO: Portable builds, only use the relative path specified in config, do not prefix with home directory
	directory = GetHomeDirectory() + g_MemcardConfig.GetMemcardsFolder();
	fileName = g_MemcardConfig.GetMemcardName(port, slot);
	fullPath = directory + fileName;

	if (FileSystem::FileExists(fullPath.c_str()))
	{
		memcardHostType = MemcardHostType::FILE;
	}
	else if (FileSystem::DirectoryExists(fullPath.c_str()))
	{
		memcardHostType = MemcardHostType::FOLDER;
	}
	else
	{
		// The default MemcardHostType is FILE; if neither a file nor folder memcard exists already
		// on the first game launch, this switch will always end up taking the FILE route. However,
		// We have this switch because in the memcard configuration, creating a folder memcard will
		// set the MemcardHostType to FOLDER, prior to invoking this function, thus taking the
		// folder route.
		switch (memcardHostType)
		{
			case MemcardHostType::FILE:
				g_MemcardFileIO.Initialize(this);
				break;
			case MemcardHostType::FOLDER:
				g_MemcardFolderIO.Initialize(this);
				break;
			default:
				DevCon.Warning("%s() Sanity check!", __FUNCTION__);
				break;
		}
	}
}

void Memcard::LoadFromFileSystem()
{
	switch (memcardHostType)
	{
		case MemcardHostType::FILE:
			if (FileSystem::FileExists(fullPath.c_str()))
			{
				g_MemcardFileIO.Load(this);
			}
			else 
			{
				Console.Warning("%s() Configured memcard file %s does not exist on host file system!", __FUNCTION__, fullPath);
			}
			break;
		case MemcardHostType::FOLDER:
			if (FileSystem::DirectoryExists(fullPath.c_str()))
			{
				g_MemcardFolderIO.Load(this);
			}
			else
			{
				Console.Warning("%s() Configured memcard folder %s does not exist on host file system!", __FUNCTION__, fullPath);
			}
			break;
		default:
			DevCon.Warning("%s() Sanity check!", __FUNCTION__);
			break;
	}
}

void Memcard::WriteToFileSystem(u32 address, size_t length)
{
	switch (memcardHostType)
	{
		case MemcardHostType::FILE:
			g_MemcardFileIO.Write(this, address, length);
			break;
		case MemcardHostType::FOLDER:
			g_MemcardFolderIO.Write(this, address, length);
			break;
		default:
			DevCon.Warning("%s() Sanity check!", __FUNCTION__);
			break;
	}
}

ghc::filesystem::fstream& Memcard::GetStreamRef()
{
	return stream;
}

size_t Memcard::GetPort()
{
	return port;
}

size_t Memcard::GetSlot()
{
	return slot;
}

FolderMemcardAttributes& Memcard::GetFolderMemcardAttributesRef()
{
	return fma;
}

std::string Memcard::GetFullPath()
{
	return fullPath;
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

u32 Memcard::GetIndirectFatCluster(size_t position){
	position = std::clamp<size_t>(position, 0, INDIRECT_FAT_CLUSTER_COUNT);
	return indirectFatClusterList.at(position);
}

u32 Memcard::GetSector()
{
	return sector;
}

std::vector<u8>& Memcard::GetMemcardDataRef()
{
	return memcardData;
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

void Memcard::SetSectorCount(SectorCount newSectorCount)
{
	sectorCount = newSectorCount;
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

	if (address + length <= memcardData.size())
	{
		for (size_t i = 0; i < length; i++)
		{
			ret.push(memcardData.at(address + i));
		}
	}
	else
	{
		DevCon.Warning("%s() Calculated read address out of bounds (%08X > %08X)", __FUNCTION__, address + length, memcardData.size());
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

		WriteToFileSystem(address, length);
	}
	else
	{
		DevCon.Warning("%s(queue) Calculated write address out of bounds (%08X > %08X)", __FUNCTION__, address + data.size(), memcardData.size());
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
