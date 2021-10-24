
#include "PrecompiledHeader.h"
#include "Sio0.h"

#include "R3000A.h"
#include "IopHw.h"

Sio0 g_sio0;

Sio0::Sio0()
{
	SetInterrupt();
}

Sio0::~Sio0() = default;

u8 Sio0::GetSioData()
{
	return sioData;
}

u32 Sio0::GetSioStat()
{
	return sioStat;
}

u16 Sio0::GetSioMode()
{
	return sioMode;
}

u16 Sio0::GetSioCtrl()
{
	return sioCtrl;
}

u16 Sio0::GetSioBaud()
{
	return sioBaud;
}

void Sio0::SetData(u8 data)
{
	u8 slot = 0;
	
	switch (mode)
	{
	case Sio0Mode::NOT_SET:
		mode = static_cast<Sio0Mode>(data);
		sioData = 0x00;
		DevCon.WriteLn("%s(%02X) // %02X", __FUNCTION__, data, sioData);
		SetInterrupt();
		break;
	case Sio0Mode::PAD:
	case Sio0Mode::PAD_MULTITAP_2:
	case Sio0Mode::PAD_MULTITAP_3:
	case Sio0Mode::PAD_MULTITAP_4:
	{
		// Slot is derived from the SIO Mode; 0x01 = slot 0 (no multitap)
		// 0x02 = slot 1 (multitapped); subtracting the base pad value
		// will yield the slot.
		slot = static_cast<u8>(mode) - static_cast<u8>(Sio0Mode::PAD);
		// Port is the 13th bit of the control register. Games will update
		// the control register as they need to; we just need to read and mask.
		const size_t port = (GetSioCtrl() & SioCtrl::PORT) >> 13;
		g_padPS1Protocol.SetActivePort(port);
		PadPS1* padPS1 = g_padPS1Protocol.GetPad(port, slot);
		g_padPS1Protocol.SetActivePad(padPS1);
		// Forward the command data to the pad, write its response to the
		// sioData register.
		sioData = g_padPS1Protocol.SendToPad(data);
		DevCon.WriteLn("%s(%02X) // %02X", __FUNCTION__, data, sioData);
		
		if (g_padPS1Protocol.GetPadMode() == PadPS1Mode::NOT_SET)
		{
			mode = Sio0Mode::NOT_SET;
		}
		else
		{
			SetInterrupt();
		}

		break;
	}
	case Sio0Mode::MEMCARD:
	case Sio0Mode::MEMCARD_MULTITAP_2:
	case Sio0Mode::MEMCARD_MULTITAP_3:
	case Sio0Mode::MEMCARD_MULTITAP_4:
	{
		// Slot is derived from the SIO Mode; 0x81 = slot 0 (no multitap)
		// 0x82 = slot 1 (multitapped); subtracting the base memcard value
		// will yield the slot.
		slot = static_cast<u8>(mode) - static_cast<u8>(Sio0Mode::MEMCARD);
		// Port is the 13th bit of the control register. Games will update
		// the control register as they need to; we just need to read and mask.
		MemcardPS1* memcardPS1 = g_memcardPS1Protocol.GetMemcard((GetSioCtrl() & SioCtrl::PORT) >> 13, slot);
		g_memcardPS1Protocol.SetActiveMemcard(memcardPS1);
		// Forward the command data to the memcard, write its response to the
		// sioData register.
		sioData = g_memcardPS1Protocol.SendToMemcard(data);
		DevCon.WriteLn("%s(%02X) // %02X", __FUNCTION__, data, sioData);

		if (g_memcardPS1Protocol.GetMemcardMode() == MemcardPS1Mode::NOT_SET)
		{
			mode = Sio0Mode::NOT_SET;
		}
		else
		{
			SetInterrupt();
		}

		break;
	}
	default:
		DevCon.Warning("%s(%02X) Unhandled Sio0 mode %02X", __FUNCTION__, data, mode);
		break;
	}
}

void Sio0::SetStat(u32 data)
{
	sioStat = data;
}

void Sio0::SetMode(u16 data)
{
	sioMode = data;
}

void Sio0::SetCtrl(u16 data)
{
	sioCtrl = data;
}

void Sio0::SetBaud(u16 data)
{
	sioBaud = data;
}

void Sio0::Reset()
{
	sioStat = 0;
	sioStat |= SioStat::TX_READY;
	sioStat |= SioStat::TX_DONE;
	sioMode = 0;
	sioCtrl = 0;
	sioBaud = 0x88;
}
#include "IopDma.h"

void Sio0::SetInterrupt()
{
	//DevCon.WriteLn("%s", __FUNCTION__);
	//sioStat |= SioStat::IRQ;
	//PSX_INT(IopEvt_SIO, 64); // PSXCLK/250000);
	PSX_INT(IopEvt_SIO, 64);
}

void Sio0::ClearInterrupt()
{
	//sioStat &= ~SioStat::IRQ;
	psxRegs.interrupt &= ~(1 << IopEvt_SIO);
}
