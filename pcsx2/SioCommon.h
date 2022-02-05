
#pragma once

#include "SioTypes.h"
#include "Memcard/Memcard.h"
#include <array>

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

	Memcard* GetMemcard(size_t port, size_t slot);
};

extern SioCommon g_SioCommon;
