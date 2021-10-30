
#include "PrecompiledHeader.h" 
#include "Sio2.h"

#include "IopDma.h"

Sio2 g_sio2;

Sio2::Sio2() = default;
Sio2::~Sio2() = default;

void Sio2::Reset()
{
	// SIO2MAN provided by the BIOS resets SIO2_CTRL to 0x3bc. Thanks ps2tek!
	g_sio2.SetCtrl(0x000003bc);
	g_sio2.SetRecv1(Recv1::DISCONNECTED);
	g_sio2.SetRecv2(Recv2::DEFAULT);
	g_sio2.SetRecv3(Recv3::DEFAULT);
}

void Sio2::SetInterrupt()
{
	iopIntcIrq(17);
}

void Sio2::Sio2Write(u8 data)
{
	PadPS2* pad = nullptr;
	MemcardPS2* memcard = nullptr;

	switch (mode)
	{
	case Sio2Mode::NOT_SET:
		mode = static_cast<Sio2Mode>(data);
		fifoPosition = 0;
		fifoOut.clear();
		fifoOut.push_back(0xff);
		break;
	case Sio2Mode::PAD:
		g_sio2.SetRecv1(Recv1::CONNECTED);
		pad = g_padPS2Protocol.GetPad(GetCtrl() & Sio2Ctrl::PORT, 0);
		g_padPS2Protocol.SetActivePad(pad);
		fifoOut.push_back(g_padPS2Protocol.SendToPad(data));

		if (g_padPS2Protocol.IsReset())
		{
			mode = Sio2Mode::NOT_SET;
		}
		break;
	case Sio2Mode::MULTITAP:
	case Sio2Mode::INFRARED:
		g_sio2.SetRecv1(Recv1::DISCONNECTED);
		fifoOut.push_back(0xff);
		break;
	case Sio2Mode::MEMCARD:
		// FIXME
		g_sio2.SetRecv1(Recv1::DISCONNECTED); 
		fifoOut.push_back(g_memcardPS2Protocol.SendToMemcard(data));
		break;
	default:
		DevCon.Warning("%s(%02X) Unhandled SIO2 Mode", __FUNCTION__, data);
		break;
	}
}

u8 Sio2::Sio2Read()
{
	if (fifoPosition >= fifoOut.size())
	{
		/*
		DevCon.Warning("%s Attempted to read beyond FIFO contents", __FUNCTION__);
		return 0xff;
		*/
		// For reasons unknown, the same command is sometimes written twice sequentially,
		// with no read in between. This triggers a protocol reset (and hence SIO2 mode reset)
		// which in turn dumps the FIFO contents. To work around this, wrap the fifoPosition
		// back to 0 when we detect an overflow.
		fifoPosition = 0;
	}

	return fifoOut.at(fifoPosition++);
}

u32 Sio2::GetSend1(u8 index)
{
	return send1.at(index);
}

u32 Sio2::GetSend2(u8 index)
{
	return send2.at(index);
}

u32 Sio2::GetSend3(u8 index)
{
	return send3.at(index);
}

u32 Sio2::GetCtrl()
{
	return ctrl;
}

u32 Sio2::GetRecv1()
{
	// Access to RECV1 indicates that writes are all sent,
	// and the replies are about to be read. After writes
	// are complete, Sio2Mode does not matter for reads,
	// and should be reset to prepare for the next writes.
	mode = Sio2Mode::NOT_SET;
	return recv1;
}

u32 Sio2::GetRecv2()
{
	return recv2;
}

u32 Sio2::GetRecv3()
{
	return recv3;
}

u32 Sio2::GetUnknown1()
{
	return unknown1;
}

u32 Sio2::GetUnknown2()
{
	return unknown2;
}

u32 Sio2::GetIStat()
{
	return iStat;
}

void Sio2::SetSend1(u8 index, u32 data)
{
	send1.at(index) = data;
}

void Sio2::SetSend2(u8 index, u32 data)
{
	send2.at(index) = data;
}

void Sio2::SetSend3(u8 index, u32 data)
{
	send3.at(index) = data;
}

void Sio2::SetCtrl(u32 data)
{
	ctrl = data;

	// Bit 0 signals to start transfer. Interrupt is raised after this bit is set. 
	if (ctrl & Sio2Ctrl::START_TRANSFER)
	{
		SetInterrupt();
	}
}

void Sio2::SetRecv1(u32 data)
{
	recv1 = data;
}

void Sio2::SetRecv2(u32 data)
{
	recv2 = data;
}

void Sio2::SetRecv3(u32 data)
{
	recv3 = data;
}

void Sio2::SetUnknown1(u32 data)
{
	unknown1 = data;
}

void Sio2::SetUnknown2(u32 data)
{
	unknown2 = data;
}

void Sio2::SetIStat(u32 data)
{
	iStat = data;
}
