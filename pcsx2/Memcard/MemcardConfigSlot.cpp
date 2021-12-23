
#include "PrecompiledHeader.h"
#include "MemcardConfigSlot.h"

#include "fmt/format.h"

MemcardConfigSlot::MemcardConfigSlot(size_t port, size_t slot)
{
	fileName = fmt::format("Memcard_{}{}.ps2", port, slot);
}

MemcardConfigSlot::~MemcardConfigSlot() = default;

ghc::filesystem::path MemcardConfigSlot::GetMemcardFileName()
{
	return fileName;
}

MemcardType MemcardConfigSlot::GetMemcardType()
{
	return type;
}

void MemcardConfigSlot::SetMemcardFileName(ghc::filesystem::path newName)
{
	this->fileName = newName;
}

void MemcardConfigSlot::SetMemcardType(MemcardType type)
{
	this->type = type;
}
