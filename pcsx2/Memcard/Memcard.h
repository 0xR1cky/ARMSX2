
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
	MemcardHostType memcardHostType = MemcardHostType::FILE;
	size_t port;
	size_t slot;
	MemcardType memcardType = MemcardType::PS2;
	u8 flag = 0x08;
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

	void InitializeOnFileSystem();
	void LoadFromFileSystem();
	void WriteToFileSystem(u32 address, size_t length);

	ghc::filesystem::fstream& GetStreamRef();
	size_t GetPort();
	size_t GetSlot();
	ghc::filesystem::path GetFullPath();
	MemcardType GetMemcardType();
	u8 GetFlag();
	u8 GetTerminator();
	SectorSize GetSectorSize();
	EraseBlockSize GetEraseBlockSize();
	SectorCount GetSectorCount();
	u32 GetSector();
	std::vector<u8>& GetMemcardDataRef();

	void SetMemcardType(MemcardType newType);
	void SetFlag(u8 newFlag);
	void SetTerminator(u8 data);
	void SetSectorCount(SectorCount newSectorCount);
	void SetSector(u32 data);

	std::queue<u8> Read(size_t length);
	void Write(std::queue<u8>& data);
	void EraseBlock();
};
