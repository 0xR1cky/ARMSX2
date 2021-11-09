
#include "PrecompiledHeader.h"
#include "MemcardPS2Protocol.h"

MemcardPS2Protocol g_MemcardPS2Protocol;

MemcardPS2Protocol::MemcardPS2Protocol() = default;
MemcardPS2Protocol::~MemcardPS2Protocol() = default;

MemcardPS2Mode MemcardPS2Protocol::GetMemcardMode()
{
	return mode;
}

u8 MemcardPS2Protocol::SendToMemcard(u8 data)
{
	return 0xff;
}
