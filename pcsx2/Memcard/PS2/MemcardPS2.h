
#pragma once

#include "MemcardPS2Types.h"
#include <vector>
#include <array>

class MemcardPS2
{
private:
	u8 terminator = static_cast<u8>(Terminator::DEFAULT);
	SectorSize sectorSize = SectorSize::STANDARD;
	EraseBlockSize eraseBlockSize = EraseBlockSize::STANDARD;
	SectorCount sectorCount = SectorCount::STANDARD;
	u32 sector = 0;
	std::vector<u8> memcardData;

public:
	MemcardPS2();
	~MemcardPS2();

	u8 GetTerminator();
	SectorSize GetSectorSize();
	EraseBlockSize GetEraseBlockSize();
	SectorCount GetSectorCount();
	u32 GetSector();

	void SetTerminator(u8 data);
	void SetSector(u32 data);

	std::array<u8, 128> Read(size_t offset);
	void Write(size_t offset, std::array<u8, 128> data);
};
