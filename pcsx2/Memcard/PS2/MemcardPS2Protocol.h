
#pragma once

#include "MemcardPS2.h"
#include "SioTypes.h"
#include <array>
#include <queue>

class MemcardPS2Protocol
{
private:
	MemcardPS2* activeMemcard;
	MemcardPS2Mode mode = MemcardPS2Mode::NOT_SET;
	size_t currentCommandByte = 1;
	// Temporary buffer to copy sector contents to.
	std::queue<u8> readBuffer;
	MemcardPS2Mode lastSectorMode = MemcardPS2Mode::NOT_SET;
	std::queue<u8> responseBuffer;

	void The2bTerminator(size_t len);

	void Probe();
	void UnknownWriteDeleteEnd();
	u8 SetSector(u8 data);
	u8 GetSpecs(u8 data);
	u8 SetTerminator(u8 data);
	u8 GetTerminator(u8 data);
	u8 WriteData(u8 data);
	u8 ReadData(u8 data);
	u8 ReadWriteEnd(u8 data);
	u8 EraseBlock(u8 data);
	u8 UnknownBoot(u8 data);
	std::queue<u8> AuthXor(std::queue<u8> &data);
	void AuthF3();
	void AuthF7();
public:
	MemcardPS2Protocol();
	~MemcardPS2Protocol();

	void FullReset();
	void SoftReset();
	MemcardPS2Mode GetMemcardMode();
	MemcardPS2* GetMemcard(size_t port, size_t slot);
	void SetActiveMemcard(MemcardPS2* memcard);

	std::queue<u8> SendToMemcard(std::queue<u8> data);
};

extern MemcardPS2Protocol g_MemcardPS2Protocol;
