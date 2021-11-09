
#pragma once

#include "MemcardPS1Types.h"
#include <array>
#include <fstream>


class MemcardPS1
{
private:
	u8 flag = 0x08;
	std::array<u8, MEMCARD_SIZE> memcardData = {};
	
	bool FetchFromDisk();
	bool CommitToDisk();
public:
	MemcardPS1();
	~MemcardPS1();

	void Init();
	u8 GetFlag();
	void SetFlag(u8 data);
	u8* GetMemcardDataPointer();
	void Read(u8* dest, u16 offset, u8 length);
	void Write(u8* src, u16 offset, u8 length);
};
