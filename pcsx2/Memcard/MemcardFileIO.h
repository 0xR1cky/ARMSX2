
#pragma once

#include "Memcard.h"

class MemcardFileIO
{
private:
	bool IsPS2Size(size_t size);
	bool IsPS1Size(size_t size);

public:
	MemcardFileIO();
	~MemcardFileIO();

	void Initialize(Memcard* memcard);
	void Load(Memcard* memcard);
	void Write(Memcard* memcard, u32 address, size_t length);
};

extern MemcardFileIO g_MemcardFileIO;
