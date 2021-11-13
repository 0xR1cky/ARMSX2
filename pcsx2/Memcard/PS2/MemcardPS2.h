
#pragma once

#include "MemcardPS2Types.h"

class MemcardPS2
{
private:
	u8 terminator = static_cast<u8>(Terminator::DEFAULT);
	SectorSize sectorSize = SectorSize::STANDARD;
	EraseBlockSize eraseBlockSize = EraseBlockSize::STANDARD;
	SectorCount sectorCount = SectorCount::STANDARD;

public:
	MemcardPS2();
	~MemcardPS2();

	u8 GetTerminator();
	SectorSize GetSectorSize();
	EraseBlockSize GetEraseBlockSize();
	SectorCount GetSectorCount();

	void SetTerminator(u8 data);
};
