
#pragma once

#include "ghc/filesystem.h"

class MemcardConfigSlot
{
private:
	ghc::filesystem::path fileName;

public:
	MemcardConfigSlot(int port, int slot);
	~MemcardConfigSlot();

	ghc::filesystem::path GetMemcardFileName();
	void SetMemcardFileName(ghc::filesystem::path newName);
};
