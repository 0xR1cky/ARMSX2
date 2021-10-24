
#include "PrecompiledHeader.h"
#include "PadPS2Protocol.h"

PadPS2Protocol g_padPS2Protocol;

PadPS2Protocol::PadPS2Protocol() = default;
PadPS2Protocol::~PadPS2Protocol() = default;

PadPS2Mode PadPS2Protocol::GetPadMode()
{
	return mode;
}

u8 PadPS2Protocol::SendToPad(u8 data)
{
	return 0xff;
}

