
#include "PrecompiledHeader.h"
#include "Sio2.h"

#include "SioCommon.h"
#include "PAD/PS2/PadPS2Protocol.h"
#include "Memcard/PS2/MemcardPS2Protocol.h"
#include "Multitap/PS2/MultitapPS2Protocol.h"
#include "Memcard/MemcardConfig.h"
#include "Multitap/MultitapConfig.h"
#include "IopDma.h"

Sio2 g_Sio2;

Sio2::Sio2() = default;
Sio2::~Sio2() = default;

void Sio2::Reset()
{
	mode = Sio2Mode::NOT_SET;
	
	for (size_t j = 0; j < 16; j++)
	{
		g_Sio2.SetSend3(j, 0);
	}

	for (size_t i = 0; i < 4; i++)
	{
		g_Sio2.SetSend1(i, 0);
		g_Sio2.SetSend2(i, 0);
	}

	std::queue<u8> emptyQueue;
	fifoOut.swap(emptyQueue);

	// SIO2MAN provided by the BIOS resets SIO2_CTRL to 0x3bc. Thanks ps2tek!
	SetCtrl(0x000003bc);
	SetRecv1(Recv1::DISCONNECTED);
	SetRecv2(Recv2::DEFAULT);
	SetRecv3(Recv3::DEFAULT);
	SetUnknown1(0);
	SetUnknown2(0);
	SetIStat(0);
	
	activePort = 0;
	send3Read = false;
	send3Position = 0;
	commandLength = 0;
	processedLength = 0;

	g_PadPS2Protocol.Reset();
	g_MultitapPS2Protocol.FullReset();
	g_MemcardPS2Protocol.FullReset();
}

void Sio2::SetInterrupt()
{
	iopIntcIrq(17);
}

size_t Sio2::GetDMABlockSize()
{
	return dmaBlockSize;
}

void Sio2::SetDMABlockSize(size_t size)
{
	dmaBlockSize = size;
}

void Sio2::Sio2Write(u8 data)
{
	PadPS2* pad = nullptr;
	Memcard* memcard = nullptr;
	MemcardPS2* memcardPS2 = nullptr;
	MemcardConfigSlot* mcs = nullptr;

	// If SEND3 contents at index send3Position have not been read, do so now.
	// This tells us what physical port we are operating on, and the length of
	// the command.
	if (!send3Read)
	{
		// If send3Position somehow goes out of bounds, warn and exit.
		if (send3Position >= send3.size())
		{
			DevCon.Warning("%s(%02X) SEND3 Overflow! SIO2 has processed commands described by all 16 SEND3 registers, and is still receiving command bytes!", __FUNCTION__, data);
			return;
		}

		const u32 s3 = send3.at(send3Position);

		// The first zero'd SEND3 value indicates the end of all commands.
		if (s3 == 0)
		{
			return;
		}

		send3Position++;
		activePort = (s3 & Send3::PORT);
		commandLength = (s3 >> 8) & Send3::COMMAND_LENGTH_MASK;
		send3Read = true;
	}

	fifoIn.push(data);

	/*
	switch (mode)
	{
		case Sio2Mode::NOT_SET:
			if (data)
			{
				mode = static_cast<Sio2Mode>(data);
				fifoOut.push_back(0xff);
				break;
			}
			else
			{
				fifoOut.push_back(0x00);
				return;
			}
		case Sio2Mode::PAD:
			g_Sio2.SetRecv1(Recv1::CONNECTED);
			pad = g_PadPS2Protocol.GetPad(activePort, g_MultitapPS2Protocol.GetActiveSlot());
			g_PadPS2Protocol.SetActivePad(pad);
			fifoOut.push_back(g_PadPS2Protocol.SendToPad(data));
			break;
		case Sio2Mode::MULTITAP:
			if (g_MultitapConfig.IsMultitapEnabled(activePort))
			{
				g_Sio2.SetRecv1(Recv1::CONNECTED);
				fifoOut.push_back(g_MultitapPS2Protocol.SendToMultitap(data));	
			}
			else 
			{
				g_Sio2.SetRecv1(Recv1::DISCONNECTED);
				fifoOut.push_back(0x00);
			}
			break;
		case Sio2Mode::INFRARED:
			g_Sio2.SetRecv1(Recv1::DISCONNECTED);
			fifoOut.push_back(0x00);
			break;
		case Sio2Mode::MEMCARD:
			switch (g_SioCommon.GetMemcardType(activePort, g_MultitapPS2Protocol.GetActiveSlot()))
			{
				case MemcardType::PS2:
					memcardPS2 = g_SioCommon.GetMemcardPS2(activePort, g_MultitapPS2Protocol.GetActiveSlot());
					g_MemcardPS2Protocol.SetActiveMemcard(memcardPS2);
					g_Sio2.SetRecv1(memcardPS2->IsSlottedIn() ? Recv1::CONNECTED : Recv1::DISCONNECTED);
					fifoOut.push_back(g_MemcardPS2Protocol.SendToMemcard(data));
					break;
				default:
					DevCon.Warning("%s(%02X) Non-PS2 memcard access from SIO2!", __FUNCTION__, data);
					fifoOut.push_back(0x00);
					break;
			}
			break;
		default:
			DevCon.Warning("%s(%02X) Unhandled SIO2 Mode", __FUNCTION__, data);
			break;
	}
	*/

	processedLength++;

	// If we've reached the command length specified by SEND3, and condition 1 or 2 are true,
	// then forward the command to the peripheral, queue its response in fifoOut,
	// and reset SIO2 for the next command.
	// 1) We've reached the block size specified by DMA
	// 2) This transaction was not DMA (block size will be zero and thus processed length always greater)
	if (processedLength >= commandLength && processedLength >= GetDMABlockSize())
	{
		send3Read = false;
		processedLength = 0;

		if (fifoIn.empty())
		{
			DevCon.Warning("%s(%02X) Sanity check, SIO2 fifoIn empty. Please report if you see this message.");
			return;
		}

		mode = static_cast<Sio2Mode>(fifoIn.front());

		switch (mode)
		{
			case Sio2Mode::PAD:
				g_PadPS2Protocol.Reset();
				break;
			case Sio2Mode::MULTITAP:
				g_MultitapPS2Protocol.SoftReset();
				break;
			case Sio2Mode::INFRARED:
				break;
			case Sio2Mode::MEMCARD:
				fifoOut = g_MemcardPS2Protocol.SendToMemcard(fifoIn);
				g_MemcardPS2Protocol.SoftReset();
				//DevCon.WriteLn("%s(%02X) SIO2 mode reset", __FUNCTION__, data);
				break;
		}

		mode = Sio2Mode::NOT_SET;
	}
}

u8 Sio2::Sio2Read()
{
	if (fifoPosition >= fifoOut.size())
	{
		DevCon.Warning("%s Attempted to read beyond FIFO contents", __FUNCTION__);
		return 0xff;
	}

	const u8 ret = fifoOut.front();
	fifoOut.pop();
	return ret;
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
	assert(fifoOut.empty(), "Game is issuing its next SEND3 writes, but never fully cleared fifoOut contents!");

	// This function is only invoked by SEND3 writes and indicates
	// a new wave of commands inbound. Reset send3Position to 0 so
	// that we are ready to read those.
	if (index == 0)
	{
		send3Position = 0;
	}

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
