
#include "PrecompiledHeader.h"
#include "MemcardPS2Protocol.h"

#include "SioCommon.h"
#include "Sio2.h"

#define MemcardPS2ProtocolAssert(condition, msg) \
	{ \
		DevCon.Warning("MemcardPS2ProtocolAssert: %s", msg); \
		assert(condition); \
	}

MemcardPS2Protocol g_MemcardPS2Protocol;

// A repeated pattern in memcard functions is to use the response
// pattern "0x00, 0x00, 0x2b, terminator.
void MemcardPS2Protocol::The2bTerminator(size_t len)
{
	while (g_Sio2.GetFifoOut().size() < len - 2)
	{
		g_Sio2.GetFifoOut().push(0x00);
	}

	g_Sio2.GetFifoOut().push(0x2b);
	g_Sio2.GetFifoOut().push(activeMemcard->GetTerminator());
}

void MemcardPS2Protocol::Probe()
{
	The2bTerminator(4);
}

void MemcardPS2Protocol::UnknownWriteDeleteEnd()
{
	The2bTerminator(4);
}

void MemcardPS2Protocol::SetSector()
{
	const u8 sectorLSB = g_Sio2.GetFifoIn().front();
	g_Sio2.GetFifoIn().pop();
	const u8 sector2nd = g_Sio2.GetFifoIn().front();
	g_Sio2.GetFifoIn().pop();
	const u8 sector3rd = g_Sio2.GetFifoIn().front();
	g_Sio2.GetFifoIn().pop();
	const u8 sectorMSB = g_Sio2.GetFifoIn().front();
	g_Sio2.GetFifoIn().pop();
	const u8 expectedChecksum = g_Sio2.GetFifoIn().front();
	g_Sio2.GetFifoIn().pop();
	
	u8 computedChecksum = sectorLSB ^ sector2nd ^ sector3rd ^ sectorMSB;
	
	if (computedChecksum != expectedChecksum)
	{
		Console.Warning("%s(queue) Warning! Memcard sector checksum failed! (Expected %02X != Actual %02X) Please report to the PCSX2 team!", __FUNCTION__, expectedChecksum, computedChecksum);
		// Exit the command without filling the terminator bytes;
		// that should be enough of an indicator to the PS2 that this operation failed.
		return;
	}

	u32 newSector = sectorLSB | (sector2nd << 8) | (sector3rd << 16) | (sectorMSB << 24);
	activeMemcard->SetSector(newSector);

	The2bTerminator(9);
}

void MemcardPS2Protocol::GetSpecs()
{
	g_Sio2.GetFifoOut().push(0x2b);
	u8 checksum = 0x00;

	const u8 sectorSizeLSB = static_cast<u16>(activeMemcard->GetSectorSize()) & 0xff;
	checksum ^= sectorSizeLSB;
	g_Sio2.GetFifoOut().push(sectorSizeLSB);

	const u8 sectorSizeMSB = static_cast<u16>(activeMemcard->GetSectorSize()) >> 8;
	checksum ^= sectorSizeMSB;
	g_Sio2.GetFifoOut().push(sectorSizeMSB);

	const u8 eraseBlockSizeLSB = static_cast<u16>(activeMemcard->GetEraseBlockSize()) & 0xff;
	checksum ^= eraseBlockSizeLSB;
	g_Sio2.GetFifoOut().push(eraseBlockSizeLSB);

	const u8 eraseBlockSizeMSB = static_cast<u16>(activeMemcard->GetEraseBlockSize()) >> 8;
	checksum ^= eraseBlockSizeMSB;
	g_Sio2.GetFifoOut().push(eraseBlockSizeMSB);

	const u8 sectorCountLSB = static_cast<u32>(activeMemcard->GetSectorCount()) & 0xff;
	checksum ^= sectorCountLSB;
	g_Sio2.GetFifoOut().push(sectorCountLSB);

	const u8 sectorCount2nd = (static_cast<u32>(activeMemcard->GetSectorCount()) & 0xff00) >> 8;
	checksum ^= sectorCount2nd;
	g_Sio2.GetFifoOut().push(sectorCount2nd);

	const u8 sectorCount3rd = (static_cast<u32>(activeMemcard->GetSectorCount()) & 0xff0000) >> 16;
	checksum ^= sectorCount3rd;
	g_Sio2.GetFifoOut().push(sectorCount3rd);

	const u8 sectorCountMSB = (static_cast<u32>(activeMemcard->GetSectorCount()) & 0xff000000) >> 24;
	checksum ^= sectorCountMSB;
	g_Sio2.GetFifoOut().push(sectorCountMSB);

	g_Sio2.GetFifoOut().push(checksum);
	g_Sio2.GetFifoOut().push(activeMemcard->GetTerminator());
}

void MemcardPS2Protocol::SetTerminator()
{
	const u8 newTerminator = g_Sio2.GetFifoIn().front();
	g_Sio2.GetFifoIn().pop();
	const u8 oldTerminator = activeMemcard->GetTerminator();
	activeMemcard->SetTerminator(newTerminator);
	g_Sio2.GetFifoOut().push(0x00);
	g_Sio2.GetFifoOut().push(0x2b);
	g_Sio2.GetFifoOut().push(oldTerminator);
}

void MemcardPS2Protocol::GetTerminator()
{
	g_Sio2.GetFifoOut().push(0x2b);
	g_Sio2.GetFifoOut().push(activeMemcard->GetTerminator());
	g_Sio2.GetFifoOut().push(static_cast<u8>(Terminator::DEFAULT));
}

void MemcardPS2Protocol::WriteData()
{
	g_Sio2.GetFifoOut().push(0x00);
	g_Sio2.GetFifoOut().push(0x2b);
	const u8 writeLength = g_Sio2.GetFifoIn().front();
	g_Sio2.GetFifoIn().pop();
	u8 checksum = 0x00;

	for (size_t writeCounter = 0; writeCounter < writeLength; writeCounter++)
	{
		const u8 writeByte = g_Sio2.GetFifoIn().front();
		g_Sio2.GetFifoIn().pop();
		checksum ^= writeByte;
		readWriteBuffer.push(writeByte);
		g_Sio2.GetFifoOut().push(0x00);
	}

	activeMemcard->Write(readWriteBuffer);
	g_Sio2.GetFifoOut().push(checksum);
	g_Sio2.GetFifoOut().push(activeMemcard->GetTerminator());
}

void MemcardPS2Protocol::ReadData()
{
	const u8 readLength = g_Sio2.GetFifoIn().front();
	g_Sio2.GetFifoIn().pop();
	g_Sio2.GetFifoOut().push(0x00);
	g_Sio2.GetFifoOut().push(0x2b);
	readWriteBuffer = activeMemcard->Read(readLength);
	u8 checksum = 0x00;

	while (!readWriteBuffer.empty())
	{
		const u8 readByte = readWriteBuffer.front();
		readWriteBuffer.pop();
		checksum ^= readByte;
		g_Sio2.GetFifoOut().push(readByte);
	}

	g_Sio2.GetFifoOut().push(checksum);
	g_Sio2.GetFifoOut().push(activeMemcard->GetTerminator());
}

void MemcardPS2Protocol::ReadWriteEnd()
{
	The2bTerminator(4);
}

void MemcardPS2Protocol::EraseBlock()
{
	activeMemcard->EraseBlock();
	The2bTerminator(4);
}

void MemcardPS2Protocol::UnknownBoot()
{
	The2bTerminator(5);
}

// Well, this is certainly a funky one.
// It appears as though this is a conditional "handshake or xor"
// type of command. It has a 5 byte and 14 byte variant.
//
// 5 bytes:  0x81 0xf0 dud  0x00 0x00
// Response: 0x00 0x00 0x00 0x2b terminator
// Handshake mode, just close the response with 0x2b and terminator.
//
// 14 bytes: 0x81 0xf0 doXor dud  (xorMe 8 times) 0x00      0x00
// Response: 0x00 0x00 0x00  0x2b (0x00 8 times)  xorResult terminator
// Here's where things get messy. When the third byte is 0x01, 0x02, 0x04, 0x0f, 0x11 or 0x13,
// we will XOR things. Before the XOR begins, the fourth byte is ignored and its response
// is 0x2b. Starting with the fifth byte the XOR begins. It defaults to 0 and has the sent bytes
// XOR'd against it. The 13th sent byte should be 0 again, and expects the result of the
// XORs. Then lastly the 14th byte also 0 expects the terminator to end the command.
//
// BUT WAIT, THERE'S MORE!
// For no discernable reason, certain values in the doXor field will be sent with a size specified
// in RECV3 of 14, HOWEVER the PS2 will get VERY angry at us if we handle these as XORs. Instead,
// they want us to respond with 0's, and then end on 0x2b and terminator. Attempts to do XORs on
// these will cause the PS2 to stop executing 0xf0 commands and jump straight to 0x52 commands;
// the PS2 thinks this memcard failed to respond correctly to PS2 commands and instead tries to
// probe it as a PS1 memcard. 
void MemcardPS2Protocol::AuthXor()
{
	const u8 modeByte = g_Sio2.GetFifoIn().front();
	g_Sio2.GetFifoIn().pop();

	switch (modeByte)
	{
		// When encountered, the command length in RECV3 is guaranteed to be 14,
		// and the PS2 is expecting us to XOR the data it is about to send.
		case 0x01:
		case 0x02:
		case 0x04:
		case 0x0f:
		case 0x11:
		case 0x13:
		{
			// Long + XOR
			g_Sio2.GetFifoOut().push(0x00);
			g_Sio2.GetFifoOut().push(0x2b);
			u8 xorResult = 0x00;
			
			for (size_t xorCounter = 0; xorCounter < 8; xorCounter++)
			{
				const u8 toXOR = g_Sio2.GetFifoIn().front();
				g_Sio2.GetFifoIn().pop();
				xorResult ^= toXOR;
				g_Sio2.GetFifoOut().push(0x00);
			}

			g_Sio2.GetFifoOut().push(xorResult);
			g_Sio2.GetFifoOut().push(activeMemcard->GetTerminator());
			break;
		}
		// When encountered, the command length in RECV3 is guaranteed to be 5,
		// and there is no attempt to XOR anything.
		case 0x00:
		case 0x03:
		case 0x05:
		case 0x08:
		case 0x09:
		case 0x0a:
		case 0x0c:
		case 0x0d:
		case 0x0e:
		case 0x10:
		case 0x12:
		case 0x14:
		{
			// Short + No XOR
			The2bTerminator(5);
			break;
		}
		// When encountered, the command length in RECV3 is guaranteed to be 14,
		// and the PS2 is about to send us data, BUT the PS2 does NOT want us
		// to send the XOR, it wants us to send the 0x2b and terminator as the
		// last two bytes.
		case 0x06:
		case 0x07:
		case 0x0b:
		{
			// Long + No XOR
			The2bTerminator(14);
			break;
		}
		default:
			DevCon.Warning("%s(queue) Unexpected modeByte (%02X), please report to the PCSX2 team", __FUNCTION__, modeByte);
			break;
	}
}

void MemcardPS2Protocol::AuthF3()
{
	The2bTerminator(5);
}

void MemcardPS2Protocol::AuthF7()
{
	The2bTerminator(5);
}

MemcardPS2Protocol::MemcardPS2Protocol() = default;
MemcardPS2Protocol::~MemcardPS2Protocol() = default;

void MemcardPS2Protocol::SoftReset()
{
	
}

void MemcardPS2Protocol::FullReset()
{
	SoftReset();

	for (size_t port = 0; port < MAX_PORTS; port++)
	{
		for (size_t slot = 0; slot < MAX_SLOTS; slot++)
		{
			MemcardPS2* memcardPS2 = g_SioCommon.GetMemcardPS2(port, slot);
			
			if (memcardPS2 != nullptr)
			{
				memcardPS2->FullReset();
			}	
		}
	}
}

MemcardPS2* MemcardPS2Protocol::GetMemcard(size_t port, size_t slot)
{
	port = std::clamp<size_t>(port, 0, MAX_PORTS);
	slot = std::clamp<size_t>(slot, 0, MAX_SLOTS);
	return g_SioCommon.GetMemcardPS2(port, slot);
}

void MemcardPS2Protocol::SetActiveMemcard(MemcardPS2* memcard)
{
	activeMemcard = memcard;
}

void MemcardPS2Protocol::SendToMemcard()
{
	const u8 deviceTypeByte = g_Sio2.GetFifoIn().front();
	MemcardPS2ProtocolAssert(static_cast<Sio2Mode>(deviceTypeByte) == Sio2Mode::MEMCARD, "MemcardPS2Protocol was initiated, but this SIO2 command is targeting another device!");
	g_Sio2.GetFifoIn().pop();
	g_Sio2.GetFifoOut().push(0x00);
	
	const u8 commandByte = g_Sio2.GetFifoIn().front();
	g_Sio2.GetFifoIn().pop();
	g_Sio2.GetFifoOut().push(0x00);

	switch (static_cast<MemcardPS2Mode>(commandByte))
	{
		case MemcardPS2Mode::PROBE:
			Probe();
			break;
		case MemcardPS2Mode::UNKNOWN_WRITE_DELETE_END:
			UnknownWriteDeleteEnd();
			break;
		case MemcardPS2Mode::SET_ERASE_SECTOR:
		case MemcardPS2Mode::SET_WRITE_SECTOR:
		case MemcardPS2Mode::SET_READ_SECTOR:
			SetSector();
			break;
		case MemcardPS2Mode::GET_SPECS:
			GetSpecs();
			break;
		case MemcardPS2Mode::SET_TERMINATOR:
			SetTerminator();
			break;
		case MemcardPS2Mode::GET_TERMINATOR:
			GetTerminator();
			break;
		case MemcardPS2Mode::WRITE_DATA:
			WriteData();
			break;
		case MemcardPS2Mode::READ_DATA:
			ReadData();
			break;
		case MemcardPS2Mode::READ_WRITE_END:
			ReadWriteEnd();
			break;
		case MemcardPS2Mode::ERASE_BLOCK:
			EraseBlock();
			break;
		case MemcardPS2Mode::UNKNOWN_BOOT:
			UnknownBoot();
			break;
		case MemcardPS2Mode::AUTH_XOR:
			AuthXor();
			break;
		case MemcardPS2Mode::AUTH_F3:
			AuthF3();
			break;
		case MemcardPS2Mode::AUTH_F7:
			AuthF7();
			break;
		default:
			DevCon.Warning("%s(queue) Unhandled MemcardPS2Mode (%02X)", __FUNCTION__, commandByte);
			break;
	}
}
