
#include "PrecompiledHeader.h"
#include "MemcardPS2.h"

MemcardPS2::MemcardPS2()
{
	const size_t sizeBytes = (static_cast<u16>(sectorSize) + ECC_BYTES) * static_cast<u32>(sectorCount);
	memcardData = std::vector<u8>(sizeBytes, 0xff);
}

MemcardPS2::~MemcardPS2() = default;

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
	const size_t address = sector * sectorSizeWithECC;
	std::queue<u8> ret;

	if (address + sectorSizeWithECC < memcardData.size())
	{
		for (size_t i = 0; i < sectorSizeWithECC; i++)
		{
			ret.push(memcardData.at(address + i));
		}
	}
	else
	{
		DevCon.Warning("%s(%d) Calculated read address out of bounds");
	}

	return ret;
}

void MemcardPS2::WriteSector(std::queue<u8>& data)
{
	const size_t sectorSizeWithECC = (static_cast<u16>(sectorSize) + ECC_BYTES);
	const size_t address = sector * sectorSizeWithECC;

	if (address + sectorSizeWithECC < memcardData.size())
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
	}
	else
	{
		DevCon.Warning("%s(%d) Calculated write address out of bounds");
	}
}

void MemcardPS2::EraseSector()
{
	const size_t sectorSizeWithECC = (static_cast<u16>(sectorSize) + ECC_BYTES);
	const size_t address = sector * sectorSizeWithECC;

	if (address + sectorSizeWithECC < memcardData.size())
	{
		for (size_t i = 0; i < sectorSizeWithECC; i++)
		{
			memcardData.at(address + i) = 0xff;
		}
	}
	else
	{
		DevCon.Warning("%s(%d) Calculated erase address out of bounds");
	}
}
