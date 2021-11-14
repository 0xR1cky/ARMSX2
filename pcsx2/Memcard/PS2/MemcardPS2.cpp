
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

std::array<u8, 128> MemcardPS2::Read(size_t offset)
{
	const u32 address = sector * (static_cast<u16>(sectorSize) + ECC_BYTES) + (offset * 128);
	std::array<u8, 128> ret{};

	if (address >= 0 && address + 128 < memcardData.size())
	{
		for (size_t i = 0; i < 128; i++)
		{
			ret.at(i) = memcardData.at(address + i);
		}
	}
	else
	{
		DevCon.Warning("%s(%d) Calculated read address out of bounds");
	}

	return ret;
}

void MemcardPS2::Write(size_t offset, std::array<u8, 128> data)
{
	const u32 address = sector * (static_cast<u16>(sectorSize) + ECC_BYTES) + (offset * 128);

	if (address >= 0 && address + 128 < memcardData.size())
	{
		for (size_t i = 0; i < 128; i++)
		{
			memcardData.at(address + i) = data.at(i);
		}
	}
	else
	{
		DevCon.Warning("%s(%d) Calculated write address out of bounds");
	}
}
