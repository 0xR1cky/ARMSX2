
#pragma once


#include "MemcardConfigSlot.h"
#include "SioTypes.h"
#include "ghc/filesystem.h"
#include <array>

using MemcardPS2SlotConfigs = std::array<std::array<std::unique_ptr<MemcardConfigSlot>, MAX_SLOTS>, MAX_PORTS>;

class MemcardConfig
{
private:
	MemcardPS2SlotConfigs slots;

	ghc::filesystem::path memcardsFolder = "./memcards_v2/";

public:
	MemcardConfig();
	~MemcardConfig();

	MemcardConfigSlot* GetMemcardConfigSlot(int port, int slot);

	ghc::filesystem::path GetMemcardsFolder();
	void SetMemcardsFolder(ghc::filesystem::path newPath);
};

extern MemcardConfig g_MemcardConfig;
