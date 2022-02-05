
#pragma once

#include "MemcardTypes.h"
#include "ghc/filesystem.h"

#include <vector>
#include <queue>

class Memcard
{
private:
	ghc::filesystem::fstream stream;
	ghc::filesystem::path directory;
	ghc::filesystem::path fileName;
	ghc::filesystem::path fullPath;
	int port, slot;
	
	bool isSlottedIn = false;
	MemcardType memcardType = MemcardType::EJECTED;

	u8 terminator = static_cast<u8>(Terminator::DEFAULT);
	SectorSize sectorSize = SectorSize::STANDARD;
	EraseBlockSize eraseBlockSize = EraseBlockSize::STANDARD;
	SectorCount sectorCount = SectorCount::STANDARD;
	u32 sector = 0;
	u32 offset = 0;
	std::vector<u8> memcardData;

public:
	Memcard(size_t port, size_t slot);
	~Memcard();

	void SoftReset();
	void FullReset();

	bool IsSlottedIn();
	void SetSlottedIn(bool value);
	MemcardType GetMemcardType();
	void SetMemcardType(MemcardType newType);
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
	void Write(std::queue<u8>& data);
	void EraseBlock();
};
