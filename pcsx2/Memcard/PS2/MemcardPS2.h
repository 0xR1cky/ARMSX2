
#pragma once

#include "MemcardPS2Types.h"
#include <vector>
#include <array>
#include <queue>
#include "ghc/filesystem.h"

class MemcardPS2
{
private:
	ghc::filesystem::fstream stream;
	ghc::filesystem::path directory;
	ghc::filesystem::path fileName;
	ghc::filesystem::path fullPath;
	int port, slot;
	bool isSlottedIn = false;
	
	u8 terminator = static_cast<u8>(Terminator::DEFAULT);
	SectorSize sectorSize = SectorSize::STANDARD;
	EraseBlockSize eraseBlockSize = EraseBlockSize::STANDARD;
	SectorCount sectorCount = SectorCount::STANDARD;
	u32 sector = 0;
	std::vector<u8> memcardData;

public:
	MemcardPS2(size_t port, size_t slot);
	~MemcardPS2();

	void SoftReset();
	void FullReset();
	bool IsSlottedIn();
	void SetSlottedIn(bool value);
	void InitializeOnFileSystem();
	void LoadFromFileSystem();
	void WriteToFileSystem(u32 address, size_t length);

	u8 GetTerminator();
	SectorSize GetSectorSize();
	EraseBlockSize GetEraseBlockSize();
	SectorCount GetSectorCount();
	u32 GetSector();

	void SetTerminator(u8 data);
	void SetSector(u32 data);

	std::queue<u8> Read(size_t length);
	std::queue<u8> ReadSector();
	void Write(std::queue<u8>& data);
	void WriteSector(std::queue<u8>& data);
	void EraseBlock();
};
