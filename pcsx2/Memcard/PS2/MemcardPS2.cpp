
#include "PrecompiledHeader.h"
#include "MemcardPS2.h"

MemcardPS2::MemcardPS2() = default;
MemcardPS2::~MemcardPS2() = default;

u8 MemcardPS2::GetTerminator()
{
	return terminator;
}

void MemcardPS2::SetTerminator(u8 data)
{
	terminator = data;
}
