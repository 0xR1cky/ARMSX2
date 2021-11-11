
#include "PrecompiledHeader.h"
#include "MemcardPS2Protocol.h"

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

// Well, this is certainly a funky one.
// It appears as though this is a conditional "handshake or xor"
// type of command. It has a 5 byte and 14 byte variant.
// 
// 5 bytes:  0x81 0xf0 0x00 0x00 0x00
// Response: 0x00 0x00 0x00 0x2b terminator
// When the third byte sent is zero, the command is functionally identical to all the other auth
// commands in that it basically does nothing but act as a heartbeat or handshake type of deal.
// Zero padding on the front, 0x2b and terminator to complete.
// 
// 14 bytes: 0x81 0xf0 0x?? 0x00 (0x?? 8 times) 0x00      0x00
// Response: 0x00 0x00 0x00 0x2b (0x00 8 times) xorResult terminator
// Here's where things get messy. When the third byte is nonzero, we begin XOR-ing things.
// We still get a 0 sent for the fourth byte and are expected to reply 0x2b, after which
// the XOR begins. It defaults to 0 and has the sent bytes (0x??) XOR'd against it incrementally.
// The 13th sent byte should be 0 again, and expects the result of the XORs. Then lastly the
// 14th byte also 0 expects the terminator to end the command.

u8 MemcardPS2Protocol::AuthXor(u8 data)
{
	static bool doXor = false;
	static u8 xorResult = 0x00;
	
	u8 ret = 0x00;

	switch (currentCommandByte)
	{
	case 2:
		// While we are saying doXor = true if data is any nonzero value,
		// it is worth noting old code only did this for 0x01, 0x02, 0x04,
		// 0x0f, 0x11 and 0x13.
		doXor = data;
		xorResult = 0;
		return ret;
	case 3:
		ret = 0x2b;
		return;
	case 4:
		if (!doXor)
		{
			return activeMemcard->GetTerminator();
		}
	case 12:
		if (doXor)
		{
			return xorResult;
		}
	case 13:
		if (doXor)
		{
			return activeMemcard->GetTerminator();
		}
	// Fallthrough
	default:
		if (doXor)
		{
			xorResult ^= data;
		}
		return 0x00;
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
			memcards.at(i).at(j) = std::make_unique<MemcardPS2>();
		}
	}
}

MemcardPS2Protocol::~MemcardPS2Protocol() = default;

void MemcardPS2Protocol::Reset()
{
	mode = MemcardPS2Mode::NOT_SET;
	currentCommandByte = 1;
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
	}

	// We have a bit of a different strategy to play here than PS2 pads.
	// Pads have a nice and predictable "header" group. But memcards, each
	// mode has more or less its own "header". These cases are ordered 
	// by their reply first, THEN by their command values!
	switch (mode)
	{
	case MemcardPS2Mode::PROBE:
		ret = Probe(data);
		break;
/*
	case MemcardPS2Mode::UNKNOWN_WRITE_DELETE:
	case MemcardPS2Mode::GET_SPECS:
	case MemcardPS2Mode::GET_TERMINATOR:
	case MemcardPS2Mode::WRITE_DATA:
	case MemcardPS2Mode::READ_DATA:
	case MemcardPS2Mode::READ_WRITE_END:
	case MemcardPS2Mode::ERASE_BLOCK:
	case MemcardPS2Mode::SET_SECTOR_ERASE:
	case MemcardPS2Mode::SET_SECTOR_WRITE:
	case MemcardPS2Mode::SET_SECTOR_READ:
	case MemcardPS2Mode::SET_TERMINATOR:
	case MemcardPS2Mode::UNKNOWN_BOOT:
		break;
*/
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
		DevCon.Warning("%s(%02X) Unhandled PadPS2Mode (%02X) (currentCommandByte = %d)", __FUNCTION__, data, static_cast<u8>(mode), currentCommandByte);
		break;
	}

	currentCommandByte++;
	return ret;
}

#undef The2bTerminator
