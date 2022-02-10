
#pragma once

#include "Memcard.h"

class MemcardFolderIO
{
private:

public:
	MemcardFolderIO();
	~MemcardFolderIO();

	void Initialize(Memcard* memcard);
	void Load(Memcard* memcard);
	void Write(Memcard* memcard, u32 address, size_t length);
};

extern MemcardFolderIO g_MemcardFolderIO;
