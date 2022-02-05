
#include "PrecompiledHeader.h"
#include "MultitapPS2Protocol.h"

#include "Sio2.h"
#include "SioTypes.h"

#define MultitapPS2ProtocolAssert(condition, msg) \
	{ \
		if (!(condition)) DevCon.Warning("MultitapPS2ProtocolAssert: %s", msg); \
		assert(condition); \
	}

MultitapPS2Protocol g_MultitapPS2Protocol;

void MultitapPS2Protocol::SupportCheck()
{
	g_Sio2.GetFifoOut().push(0x5a);
	g_Sio2.GetFifoOut().push(0x04);
	g_Sio2.GetFifoOut().push(0x00);
	g_Sio2.GetFifoOut().push(0x5a);
}

void MultitapPS2Protocol::Select()
{
	const u8 newSlot = g_Sio2.GetFifoIn().front();
	g_Sio2.GetFifoIn().pop();
	const bool isInBounds = (newSlot >= 0 && newSlot < MAX_SLOTS);

	if (isInBounds)
	{
		activeSlot = newSlot;
	}

	g_Sio2.GetFifoOut().push(0x5a);
	g_Sio2.GetFifoOut().push(0x00);
	g_Sio2.GetFifoOut().push(0x00);
	g_Sio2.GetFifoOut().push(isInBounds ? newSlot : 0xff);
	g_Sio2.GetFifoOut().push(isInBounds ? 0x5a : 0x66);
}

MultitapPS2Protocol::MultitapPS2Protocol() = default;
MultitapPS2Protocol::~MultitapPS2Protocol() = default;

void MultitapPS2Protocol::SoftReset()
{
	
}

void MultitapPS2Protocol::FullReset()
{
	SoftReset();

	activeSlot = 0;
}

u8 MultitapPS2Protocol::GetActiveSlot()
{
	return activeSlot;
}

void MultitapPS2Protocol::SendToMultitap()
{
	const u8 deviceTypeByte = g_Sio2.GetFifoIn().front();
	MultitapPS2ProtocolAssert(static_cast<Sio2Mode>(deviceTypeByte) == Sio2Mode::MULTITAP, "MultitapPS2Protocol was initiated, but this SIO2 command is targeting another device!");
	g_Sio2.GetFifoIn().pop();
	g_Sio2.GetFifoOut().push(0x00);

	const u8 commandByte = g_Sio2.GetFifoIn().front();
	g_Sio2.GetFifoIn().pop();
	g_Sio2.GetFifoOut().push(0x80);

	switch (static_cast<MultitapPS2Mode>(commandByte))
	{
		case MultitapPS2Mode::PAD_SUPPORT_CHECK:
		case MultitapPS2Mode::MEMCARD_SUPPORT_CHECK:
			SupportCheck();
			break;
		case MultitapPS2Mode::SELECT_PAD:
		case MultitapPS2Mode::SELECT_MEMCARD:
			Select();
			break;
		default:
			DevCon.Warning("%s(queue) Unhandled MultitapPS2Mode (%02X)", __FUNCTION__, commandByte);
			break;
	}
}
