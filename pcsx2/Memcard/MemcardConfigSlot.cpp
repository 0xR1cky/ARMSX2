
#include "PrecompiledHeader.h"
#include "MemcardConfigSlot.h"

#include "fmt/format.h"

MemcardConfigSlot::MemcardConfigSlot(int port, int slot)
{
	fileName = fmt::format("Memcard_{}{}.ps2", port, slot);
}

MemcardConfigSlot::~MemcardConfigSlot() = default;

ghc::filesystem::path MemcardConfigSlot::GetMemcardFileName()
{
	return fileName;
}

void MemcardConfigSlot::SetMemcardFileName(ghc::filesystem::path newName)
{
	fileName = newName;
}
