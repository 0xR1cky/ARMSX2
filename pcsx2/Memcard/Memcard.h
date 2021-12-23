
#pragma once

#include "MemcardTypes.h"
#include "Memcard/PS2/MemcardPS2.h"
#include "Memcard/PS1/MemcardPS1.h"

class Memcard
{
private:
	size_t port;
	size_t slot;
	MemcardType type = MemcardType::EJECTED;
	std::unique_ptr<MemcardPS1> memcardPS1 = nullptr;
	std::unique_ptr<MemcardPS2> memcardPS2 = nullptr;

public:
	Memcard(size_t port, size_t slot);
	~Memcard();

	MemcardType GetMemcardType();
	MemcardPS1* GetMemcardPS1();
	MemcardPS2* GetMemcardPS2();

	void SetMemcardType(MemcardType type);
};
