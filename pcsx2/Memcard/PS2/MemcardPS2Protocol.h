
#pragma once

#include "MemcardPS2.h"
#include "SioTypes.h"
#include <array>
#include <queue>

class MemcardPS2Protocol
{
private:
	MemcardPS2* activeMemcard;
	// Temporary buffer to copy sector contents to.
	std::queue<u8> readWriteBuffer;
	// Temporary buffer to write response values into.
	std::queue<u8> responseBuffer;

	void The2bTerminator(size_t len);

	void Probe();
	void UnknownWriteDeleteEnd();
	void SetSector(std::queue<u8> &data);
	void GetSpecs();
	void SetTerminator(u8 newTerminator);
	void GetTerminator();
	void WriteData(std::queue<u8> &data);
	void ReadData(u8 readLength);
	void ReadWriteEnd();
	void EraseBlock();
	void UnknownBoot();
	void AuthXor(std::queue<u8> &data);
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

	std::queue<u8> SendToMemcard(std::queue<u8> &data);
};

extern MemcardPS2Protocol g_MemcardPS2Protocol;
