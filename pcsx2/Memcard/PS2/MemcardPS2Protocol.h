
#pragma once

#include "MemcardPS2.h"
#include "SioTypes.h"
#include <array>

using MemcardPS2Array = std::array<std::array<std::unique_ptr<MemcardPS2>, MAX_SLOTS>, MAX_PORTS>;

// A repeated pattern in memcard functions is to use the response
// pattern "0x00, 0x00, 0x2b, terminator. We'll inline this here
// so we can quickly jam it into such functions without redefining
// it all the time.
inline u8 _The2bTerminator(size_t len, size_t currentCommandByte, u8 terminator)
{
	if (currentCommandByte == (len - 2))
	{
		return 0x2b;
	} 
	else if (currentCommandByte == (len - 1))
	{
		return terminator;
	}

	return 0x00;
}

class MemcardPS2Protocol
{
private:
	MemcardPS2Array memcards;
	MemcardPS2* activeMemcard;
	MemcardPS2Mode mode = MemcardPS2Mode::NOT_SET;
	size_t currentCommandByte = 1;

	u8 Probe(u8 data);
	u8 GetSpecs(u8 data);
	u8 SetTerminator(u8 data);
	u8 GetTerminator(u8 data);
	u8 UnknownBoot(u8 data);
	u8 AuthXor(u8 data);	
	u8 AuthF3(u8 data);
	u8 AuthF7(u8 data);
public:
	MemcardPS2Protocol();
	~MemcardPS2Protocol();

	void Reset();
	MemcardPS2Mode GetMemcardMode();
	MemcardPS2* GetMemcard(size_t port, size_t slot);
	void SetActiveMemcard(MemcardPS2* memcard);

	u8 SendToMemcard(u8 data);
};

extern MemcardPS2Protocol g_MemcardPS2Protocol;
