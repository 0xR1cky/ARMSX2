
#pragma once

#include "ghc/filesystem.h"
#include "MemcardTypes.h"

class MemcardConfigSlot
{
private:
	ghc::filesystem::path fileName;
	MemcardType type = MemcardType::PS2;

public:
	MemcardConfigSlot(size_t port, size_t slot);
	~MemcardConfigSlot();

	ghc::filesystem::path GetMemcardFileName();
	MemcardType GetMemcardType();

	void SetMemcardFileName(ghc::filesystem::path newName);
	void SetMemcardType(MemcardType type);
};
