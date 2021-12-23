
#pragma once

#include <array>
#include "Memcard/Memcard.h"
#include "Memcard/PS2/MemcardPS2.h"
#include "SioTypes.h"

using MemcardArray = std::array<std::array<std::unique_ptr<Memcard>, MAX_SLOTS>, MAX_PORTS>;

class SioCommon
{
private:
	MemcardArray memcards;

public:
	SioCommon();
	~SioCommon();

	void SoftReset();
	void FullReset();

	MemcardType GetMemcardType(size_t port, size_t slot);
	MemcardPS1* GetMemcardPS1(size_t port, size_t slot);
	MemcardPS2* GetMemcardPS2(size_t port, size_t slot);
};

extern SioCommon g_SioCommon;
