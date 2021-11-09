
#include "PrecompiledHeader.h"
#include "MultitapPS2Protocol.h"

MultitapPS2Protocol g_MultitapPS2Protocol;

MultitapPS2Protocol::MultitapPS2Protocol() = default;
MultitapPS2Protocol::~MultitapPS2Protocol() = default;

void MultitapPS2Protocol::Reset()
{

}

u8 MultitapPS2Protocol::SendToMultitap(u8 data)
{
	return 0x00;
}
