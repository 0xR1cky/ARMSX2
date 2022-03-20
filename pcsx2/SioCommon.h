
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
	// Forces folder memcards to reload their contents from the host filesystem.
	// Typically invoked after the core notifies MemcardFolderIO about a new serial
	// and memcard filters
	void FolderReload();

	Memcard* GetMemcard(size_t port, size_t slot);
};

extern SioCommon g_SioCommon;
