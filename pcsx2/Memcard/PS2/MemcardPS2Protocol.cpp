
#include "PrecompiledHeader.h"
#include "MemcardPS2Protocol.h"

#include "Sio2.h"

MemcardPS2Protocol g_MemcardPS2Protocol;

// This define is purely a convenience thing so I don't have to put
// the command byte counter and terminator params on each call to
// the inlined function in the source. Am I lazy? Yup. Feel bad about it?
// Nope!
#define The2bTerminator(len) _The2bTerminator(len, currentCommandByte, activeMemcard->GetTerminator());

u8 MemcardPS2Protocol::Probe(u8 data)
{
	return The2bTerminator(4);
}

u8 MemcardPS2Protocol::UnknownWriteDeleteEnd(u8 data)
{
	return The2bTerminator(4);
}

u8 MemcardPS2Protocol::SetSector(u8 data)
{
	static u32 newSector = 0;
	static u8 checksum = 0;

	switch (currentCommandByte)
	{
		case 2:
			newSector = data;
			checksum = data;
			lastSectorMode = mode;
			break;
		case 3:
			newSector |= (data << 8);
			checksum ^= data;
			break;
		case 4:
			newSector |= (data << 16);
			checksum ^= data;
			break;
		case 5:
			newSector |= (data << 24);
			checksum ^= data;
			activeMemcard->SetSector(newSector);
			break;
		case 6:
			if (checksum != data)
			{
				Console.Warning("%s(%02X) Warning! Memcard sector checksum failed! (Expected %02X != Actual %02X) Please report to the PCSX2 team!", __FUNCTION__, data, data, checksum);
			}
			break;
		default:
			break;
	}

	return The2bTerminator(9);
}

u8 MemcardPS2Protocol::GetSpecs(u8 data)
{
	static u8 checksum = 0x00;
	u8 ret = 0x00;

	switch (currentCommandByte)
	{
		case 2:
			return 0x2b;
		case 3: // Sector size, LSB
			ret = static_cast<u16>(activeMemcard->GetSectorSize()) & 0xff;
			checksum ^= ret;
			return ret;
		case 4: // Sector size, MSB
			ret = static_cast<u16>(activeMemcard->GetSectorSize()) >> 8;
			checksum ^= ret;
			return ret;
		case 5: // Erase block size, LSB
			ret = static_cast<u16>(activeMemcard->GetEraseBlockSize()) & 0xff;
			checksum ^= ret;
			return ret;
		case 6: // Erase block size, MSB
			ret = static_cast<u16>(activeMemcard->GetEraseBlockSize()) >> 8;
			checksum ^= ret;
			return ret;
		case 7: // Sector count, LSB
			ret = static_cast<u32>(activeMemcard->GetSectorCount()) & 0xff;
			checksum ^= ret;
			return ret;
		case 8: // Sector count, second byte
			ret = (static_cast<u32>(activeMemcard->GetSectorCount()) & 0xff00) >> 8;
			checksum ^= ret;
			return ret;
		case 9: // Sector count, third byte
			ret = (static_cast<u32>(activeMemcard->GetSectorCount()) & 0xff0000) >> 16;
			checksum ^= ret;
			return ret;
		case 10: // Sector count, MSB
			ret = (static_cast<u32>(activeMemcard->GetSectorCount()) & 0xff000000) >> 24;
			checksum ^= ret;
			return ret;
		case 11:
			return checksum;
		case 12:
			checksum = 0x00;
			return activeMemcard->GetTerminator();
		default:
			return 0x00;
	}
}

u8 MemcardPS2Protocol::SetTerminator(u8 data)
{
	static u8 oldTerminator = activeMemcard->GetTerminator();

	switch (currentCommandByte)
	{
		case 2:
			oldTerminator = activeMemcard->GetTerminator();
			activeMemcard->SetTerminator(data);
			return 0x00;
		case 3:
			return 0x2b;
		case 4:
			return oldTerminator;
		default:
			return 0x00;
	}
}

u8 MemcardPS2Protocol::GetTerminator(u8 data)
{
	switch (currentCommandByte)
	{
		case 2:
			return 0x2b;
		case 3:
			return activeMemcard->GetTerminator();
		case 4:
			return static_cast<u8>(Terminator::DEFAULT);
		default:
			return 0x00;
	}
}

u8 MemcardPS2Protocol::WriteData(u8 data)
{
	static u8 writeSize = 0;
	static u8 bytesWritten = 0;
	static u8 checksum = 0x00;

	switch (currentCommandByte)
	{
		case 0:
		case 1:
			return 0x00;
		case 2:
			writeSize = data;
			return 0x00;
		case 3:
			sectorBuffer.push(data);
			checksum = data;
			bytesWritten = 1;
			return 0x2b;
		case 19:
			if (writeSize == ECC_BYTES)
			{
				return 0x00;
			}
			else if (bytesWritten++ < writeSize)
			{
				sectorBuffer.push(data);
				checksum ^= data;
			}
			return data;
		case 20:
			if (writeSize == ECC_BYTES)
			{
				return checksum;
			}
			else if (bytesWritten++ < writeSize)
			{
				sectorBuffer.push(data);
				checksum ^= data;
			}
			return data;
		case 21:
			if (writeSize == ECC_BYTES)
			{
				return activeMemcard->GetTerminator();
			}
			else if (bytesWritten++ < writeSize)
			{
				sectorBuffer.push(data);
				checksum ^= data;				
			}
			return data;
		case 131:
			return 0x00;
		case 132:
			return checksum;
		case 133:
			return activeMemcard->GetTerminator();
		default:
			// Sanity check, actually should not be possible but
			// in case of emergency
			if (currentCommandByte > 133)
			{
				DevCon.Warning("%s(%02X) Write overflow!!!", __FUNCTION__, data);
				return 0x00;
			}

			if (bytesWritten++ < writeSize)
			{
				sectorBuffer.push(data);
				checksum ^= data;
			}

			if (writeSize == ECC_BYTES && bytesWritten == writeSize)
			{
				activeMemcard->WriteSector(sectorBuffer);
			}
			
			return 0x00;
	}
}

u8 MemcardPS2Protocol::ReadData(u8 data)
{
	static u8 readSize = 0;
	static u8 bytesRead = 0;
	static u8 checksum = 0x00;

	switch (currentCommandByte)
	{
		case 0:
		case 1:
			return 0x00;
		case 2:
			readSize = data;
			return 0x00;
		case 3:
			if (sectorBuffer.empty())
			{
				sectorBuffer = activeMemcard->ReadSector();
			}
			return 0x2b;
		case 4:
			checksum = sectorBuffer.front();
			sectorBuffer.pop();
			bytesRead = 1;
			return checksum;
		case 20:
			if (readSize == ECC_BYTES)
			{
				return checksum;
			}
			else if (bytesRead++ < readSize)
			{
				const u8 ret = sectorBuffer.front();
				checksum ^= ret;
				sectorBuffer.pop();
				return ret;
			}
		case 21:
			if (readSize == ECC_BYTES)
			{
				return activeMemcard->GetTerminator();
			}
			else if (bytesRead++ < readSize)
			{
				const u8 ret = sectorBuffer.front();
				checksum ^= ret;
				sectorBuffer.pop();
				return ret;
			}
			else
			{
				DevCon.Warning("%s(%02X) Sanity check, please report to PCSX2 team if this message is found", __FUNCTION__, data);
				return 0x00;
			}
		case 132:
			return checksum;
		case 133:
			return activeMemcard->GetTerminator();
		default:
			// Sanity check, actually should not be possible but
			// in case of emergency
			if (currentCommandByte > 133)
			{
				DevCon.Warning("%s(%02X) Read overflow!!!", __FUNCTION__, data);
				return 0x00;
			}

			u8 ret = 0xff;

			if (bytesRead++ < readSize)
			{
				if (!sectorBuffer.empty())
				{
					ret = sectorBuffer.front();
					sectorBuffer.pop();
				}
			}
			
			checksum ^= ret;
			return ret;
	}
}

u8 MemcardPS2Protocol::ReadWriteEnd(u8 data)
{
	return The2bTerminator(4);
}

u8 MemcardPS2Protocol::EraseBlock(u8 data)
{
	if (currentCommandByte == 1)
	{
		activeMemcard->EraseBlock();
	}

	return The2bTerminator(4);
}

u8 MemcardPS2Protocol::UnknownBoot(u8 data)
{
	return The2bTerminator(5);
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
// we will XOR-ing things. Before the XOR begins, the fourth byte is ignored and its response
// is 0x2b. Starting with the fifth byte the XOR begins. It defaults to 0 and has the sent bytes
// (xorMe) XOR'd against it. The 13th sent byte should be 0 again, and expects the result of the
// XORs. Then lastly the 14th byte also 0 expects the terminator to end the command.
//
// BUT WAIT, THERE'S MORE!
// For no discernable reason, certain values in the doXor field will be sent with a size specified
// in RECV3 of 14, HOWEVER the PS2 will get VERY angry at us if we handle these as XORs. Instead,
// they want us to respond with 0's, and then end on 0x2b and terminator. Attempts to do XORs on
// these will cause the PS2 to stop executing 0xf0 commands and jump straight to 0x52 commands;
// the PS2 thinks this memcard failed to respond correctly to PS2 commands and instead tries to
// probe it as a PS1 memcard. The doXor values are grouped and labelled accordingly in the
// function body.
u8 MemcardPS2Protocol::AuthXor(u8 data)
{
	static bool doXor = false;
	static bool isShort = false;
	static u8 xorResult = 0x00;

	if (currentCommandByte == 2)
	{
		switch (data)
		{
			// When encountered, the command length in RECV3 is guaranteed to be 14,
			// and the PS2 is expecting us to XOR the data it is about to send.
			case 0x01:
			case 0x02:
			case 0x04:
			case 0x0f:
			case 0x11:
			case 0x13:
				doXor = true;
				isShort = false;
				break;
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
				doXor = false;
				isShort = true;
				break;
			// When encountered, the command length in RECV3 is guaranteed to be 14,
			// and the PS2 is about to send us data, BUT the PS2 does NOT want us
			// to send the XOR, it wants us to send the 0x2b and terminator as the
			// last two bytes.
			case 0x06:
			case 0x07:
			case 0x0b:
				doXor = false;
				isShort = false;
				break;
			default:
				DevCon.Warning("%s(%02X) Unexpected doXor value, please report to the PCSX2 team", __FUNCTION__, data);
				doXor = false;
				isShort = false;
				break;
		}
		xorResult = 0;
		return 0x00;
	}
	else if (doXor)
	{
		switch (currentCommandByte)
		{
			case 3:
				return 0x2b;
			case 12:
				return xorResult;
			case 13:
				// Reset after completion, prevents below default label
				// from running on currentCommandByte = 1.
				doXor = false; 
				return activeMemcard->GetTerminator();
			default:
				xorResult ^= data;
				return 0x00;
		}
	}
	else
	{
		return The2bTerminator(isShort ? 5 : 14);
	}
}

u8 MemcardPS2Protocol::AuthF3(u8 data)
{
	return The2bTerminator(5);
}

u8 MemcardPS2Protocol::AuthF7(u8 data)
{
	return The2bTerminator(5);
}

MemcardPS2Protocol::MemcardPS2Protocol()
{
	for (size_t i = 0; i < MAX_PORTS; i++)
	{
		for (size_t j = 0; j < MAX_SLOTS; j++)
		{
			memcards.at(i).at(j) = std::make_unique<MemcardPS2>(i, j);
		}
	}
}

MemcardPS2Protocol::~MemcardPS2Protocol() = default;

void MemcardPS2Protocol::SoftReset()
{
	mode = MemcardPS2Mode::NOT_SET;
	currentCommandByte = 1;
}

void MemcardPS2Protocol::FullReset()
{
	SoftReset();

	for (size_t i = 0; i < MAX_PORTS; i++)
	{
		for (size_t j = 0; j < MAX_SLOTS; j++)
		{
			memcards.at(i).at(j)->FullReset();
		}
	}
}

MemcardPS2Mode MemcardPS2Protocol::GetMemcardMode()
{
	return mode;
}

MemcardPS2* MemcardPS2Protocol::GetMemcard(size_t port, size_t slot)
{
	port = std::clamp<size_t>(port, 0, MAX_PORTS);
	slot = std::clamp<size_t>(slot, 0, MAX_SLOTS);
	return memcards.at(port).at(slot).get();
}

void MemcardPS2Protocol::SetActiveMemcard(MemcardPS2* memcard)
{
	activeMemcard = memcard;
}

u8 MemcardPS2Protocol::SendToMemcard(u8 data)
{
	u8 ret = 0xff;

	if (currentCommandByte == 1)
	{
		mode = static_cast<MemcardPS2Mode>(data);
		debug_fifoin.clear();
		debug_fifoin.push_back(0xff);
		debug_fifoout.clear();
		debug_fifoout.push_back(0xff);
	}

	// We have a bit of a different strategy to play here than PS2 pads.
	// Pads have a nice and predictable "header" group. But memcards, each
	// mode has more or less its own "header".
	switch (mode)
	{
		case MemcardPS2Mode::PROBE:
			ret = Probe(data);
			break;
		case MemcardPS2Mode::UNKNOWN_WRITE_DELETE_END:
			ret = UnknownWriteDeleteEnd(data);
			break;
		case MemcardPS2Mode::SET_ERASE_SECTOR:
			ret = SetSector(data);
			break;
		case MemcardPS2Mode::SET_WRITE_SECTOR:
			ret = SetSector(data);
			break;
		case MemcardPS2Mode::SET_READ_SECTOR:
			ret = SetSector(data);
			break;
		case MemcardPS2Mode::GET_SPECS:
			ret = GetSpecs(data);
			break;
		case MemcardPS2Mode::SET_TERMINATOR:
			ret = SetTerminator(data);
			break;
		case MemcardPS2Mode::GET_TERMINATOR:
			ret = GetTerminator(data);
			break;
		case MemcardPS2Mode::WRITE_DATA:
			ret = WriteData(data);
			break;
		case MemcardPS2Mode::READ_DATA:
			ret = ReadData(data);
			break;
		case MemcardPS2Mode::READ_WRITE_END:
			ret = ReadWriteEnd(data);
			break;
		case MemcardPS2Mode::ERASE_BLOCK:
			ret = EraseBlock(data);
			break;
		case MemcardPS2Mode::UNKNOWN_BOOT:
			ret = UnknownBoot(data);
			break;
		case MemcardPS2Mode::AUTH_XOR:
			ret = AuthXor(data);
			break;
		case MemcardPS2Mode::AUTH_F3:
			ret = AuthF3(data);
			break;
		case MemcardPS2Mode::AUTH_F7:
			ret = AuthF7(data);
			break;
		default:
			DevCon.Warning("%s(%02X) Unhandled MemcardPS2Mode (%02X) (currentCommandByte = %d)", __FUNCTION__, data, static_cast<u8>(mode), currentCommandByte);
			break;
	}

	currentCommandByte++;
	debug_fifoin.push_back(data);
	debug_fifoout.push_back(ret);

	if (currentCommandByte > 133)
	{
		currentCommandByte = currentCommandByte;
	}

	return ret;
}

#undef The2bTerminator
