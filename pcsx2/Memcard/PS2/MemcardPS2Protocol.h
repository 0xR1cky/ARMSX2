
#pragma once

#include "MemcardPS2.h"
#include "SioTypes.h"
#include <array>

using MemcardPS2Array = std::array<std::array<std::unique_ptr<MemcardPS2>, MAX_SLOTS>, MAX_PORTS>;

class MemcardPS2Protocol
{
private:
	MemcardPS2Array memcards;
	MemcardPS2* activeMemcard;
	MemcardPS2Mode mode = MemcardPS2Mode::NOT_SET;
	size_t currentCommandByte = 1;

	u8 UnknownBootProbe(u8 data);
	u8 UnknownWriteDelete(u8 data);
	u8 SetSectorErase(u8 data);
	u8 SetSectorWrite(u8 data);
	u8 SetSectorRead(u8 data);
	u8 GetSpecs(u8 data);
	u8 SetTerminator(u8 data);
	u8 GetTerminator(u8 data);
	u8 UnknownCopyDelete(u8 data);
	u8 UnknownBoot(u8 data);
	u8 Auth(u8 data);
	u8 UnknownReset(u8 data);
	u8 UnknownNoIdea(u8 data);
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
