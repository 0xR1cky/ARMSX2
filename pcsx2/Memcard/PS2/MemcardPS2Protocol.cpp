
#include "PrecompiledHeader.h"
#include "MemcardPS2Protocol.h"

MemcardPS2Protocol g_MemcardPS2Protocol;

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
		ret = 0x00;
	}
	else if (currentCommandByte == 2)
	{
		ret = 0x5a;
	}
	else
	{
		switch (mode)
		{
		case MemcardPS2Mode::UNKNOWN_BOOT_PROBE:
			ret = UnknownBootProbe(data);
			break;
		case MemcardPS2Mode::UNKNOWN_WRITE_DELETE:
			ret = UnknownWriteDelete(data);
			break;
		case MemcardPS2Mode::SET_SECTOR_ERASE:
			ret = SetSectorErase(data);
			break;
		case MemcardPS2Mode::SET_SECTOR_WRITE:
			ret = SetSectorWrite(data);
			break;
		case MemcardPS2Mode::SET_SECTOR_READ:
			ret = SetSectorRead(data);
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
		case MemcardPS2Mode::UNKNOWN_COPY_DELETE:
			ret = UnknownCopyDelete(data);
			break;
		case MemcardPS2Mode::UNKNOWN_BOOT:
			ret = UnknownBoot(data);
			break;
		case MemcardPS2Mode::AUTH:
			ret = Auth(data);
			break;
		case MemcardPS2Mode::UNKNOWN_RESET:
			ret = UnknownReset(data);
			break;
		case MemcardPS2Mode::UNKNOWN_NO_IDEA:
			ret = UnknownNoIdea(data);
			break;
		default:
			DevCon.Warning("%s(%02X) Unhandled PadPS2Mode (%02X) (currentCommandByte = %d)", __FUNCTION__, data, static_cast<u8>(mode), currentCommandByte);
			break;
		}
	}

	currentCommandByte++;
	return ret;
}
