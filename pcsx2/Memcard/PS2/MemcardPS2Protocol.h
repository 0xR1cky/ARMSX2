
#pragma once

#include "Memcard/Memcard.h"
#include "SioTypes.h"
#include <array>
#include <queue>

class MemcardPS2Protocol
{
private:
	Memcard* activeMemcard;
	// Temporary buffer to copy sector contents to.
	std::queue<u8> readWriteBuffer;

	void The2bTerminator(size_t len);

	void Probe();
	void UnknownWriteDeleteEnd();
	void SetSector();
	void GetSpecs();
	void SetTerminator();
	void GetTerminator();
	void WriteData();
	void ReadData();
	void ReadWriteEnd();
	void EraseBlock();
	void UnknownBoot();
	void AuthXor();
	void AuthF3();
	void AuthF7();
public:
	MemcardPS2Protocol();
	~MemcardPS2Protocol();

	void FullReset();
	void SoftReset();
	MemcardPS2Mode GetMemcardMode();
	void SetActiveMemcard(Memcard* memcard);

	void SendToMemcard();
};

extern MemcardPS2Protocol g_MemcardPS2Protocol;
