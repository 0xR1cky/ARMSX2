
#pragma once

#include "MemcardPS2Types.h"

class MemcardPS2
{
private:
	u8 terminator = static_cast<u8>(Terminator::DEFAULT);
	SectorSize sectorSize = SectorSize::STANDARD;
	EraseBlockSize eraseBlockSize = EraseBlockSize::STANDARD;
	SectorCount sectorCount = SectorCount::STANDARD;
	u32 sectorAddress = 0x00000000;

public:
	MemcardPS2();
	~MemcardPS2();

	u8 GetTerminator();
	SectorSize GetSectorSize();
	EraseBlockSize GetEraseBlockSize();
	SectorCount GetSectorCount();
	u32 GetSectorAddress();

	void SetTerminator(u8 data);
	void SetSectorAddress(u32 data);
};
