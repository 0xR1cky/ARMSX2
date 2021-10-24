
#pragma once

#include "MemcardTypes.h"

static constexpr u8 MEMCARD_PORTS = 2;
static constexpr u8 MEMCARD_SLOTS = 4;

class Memcard
{
private:

public:
	virtual Type GetMemcardType() = 0;
};

//extern std::array<std::array<Memcard, MEMCARD_SLOTS>, MEMCARD_PORTS> g_memcards;
