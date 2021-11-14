
#include "PrecompiledHeader.h"
#include "MemcardPS2.h"

MemcardPS2::MemcardPS2() = default;
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

u32 MemcardPS2::GetSectorAddress()
{
	return sectorAddress;
}

void MemcardPS2::SetTerminator(u8 data)
{
	terminator = data;
}

void MemcardPS2::SetSectorAddress(u32 data)
{
	sectorAddress = data;
}
