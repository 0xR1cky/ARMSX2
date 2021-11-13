
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

void MemcardPS2::SetTerminator(u8 data)
{
	terminator = data;
}
