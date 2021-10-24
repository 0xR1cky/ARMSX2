
#include "PrecompiledHeader.h"
#include "MemcardPS1Protocol.h"

MemcardPS1Protocol g_memcardPS1Protocol;

u8 MemcardPS1Protocol::GetMSB()
{
	return address & 0x00ff;
}

void MemcardPS1Protocol::SetMSB(u8 data)
{
	u16 mask = data;
	address &= mask;
}

u8 MemcardPS1Protocol::GetLSB()
{
	return ((address & 0xff00) >> 8);
}

void MemcardPS1Protocol::SetLSB(u8 data)
{
	u16 mask = (data << 8);
	address &= mask;
}

u8 MemcardPS1Protocol::CalculateChecksum()
{
	u8 ret = GetMSB() ^ GetLSB();

	for (u8 sectorByte : sectorBuffer)
	{
		ret ^= sectorByte;
	}

	return ret;
}

u8 MemcardPS1Protocol::CommandRead(u8 data)
{
	u8 ret = 0xff;

	switch (currentCommandByte)
	{
	case 2: // Memcard ID 1, const value 
		ret = 0x5a;
		break;
	case 3: // Memcard ID 2, const value
		ret = 0x5d;
		break;
	case 4: // MSB, no response
		SetMSB(data);
		ret = 0x00;
		break;
	case 5: // LSB, no response
		SetLSB(data);
		ret = 0x00;
		break;
	case 6: // Acknowledge 1, const value 
		ret = 0x5c;
		break;
	case 7: // Acknowledge 2, const value 
		ret = 0x5d;
		break;
	case 8: // Echo back MSB 
		ret = GetMSB();
		break;
	case 9: // Echo back LSB
		ret = GetLSB();
		break;
	case 10: // Read sector data
		activeMemcard->Read(sectorBuffer.data(), address, SECTOR_SIZE);
		ret = sectorBuffer.at(0);
		break;
	case 138: // Checksum 
		ret = CalculateChecksum();
		break;
	case 139: // End byte, const value
		ret = 0x47;
		Reset();
		break;
	default: // 11-137: Continue to reply from the read buffer
		ret = sectorBuffer.at(currentCommandByte - 10);
		break;
	}

	return ret;
}

u8 MemcardPS1Protocol::CommandState(u8 data)
{
	u8 ret = 0xff;

	switch (currentCommandByte)
	{
	case 2: // Const values
		ret = 0x5a;
		break;
	case 3:
		ret = 0x5d;
		break;
	case 4:
		ret = 0x5c;
		break;
	case 5:
		ret = 0x5d;
		break;
	case 6:
		ret = 0x04;
		break;
	case 7:
		ret = 0x00;
		break;
	case 8:
		ret = 0x00;
		break;
	case 9:
		ret = 0x80;
		Reset();
		break;
	default:
		break;
	}

	return ret;
}

u8 MemcardPS1Protocol::CommandWrite(u8 data)
{
	u8 ret = 0xff;

	switch (currentCommandByte)
	{
	case 2: // Memcard ID 1, const value 
		ret = 0x5a;
		break;
	case 3: // Memcard ID 2, const value
		ret = 0x5d;
		break;
	case 4: // MSB, no response
		SetMSB(data);
		ret = 0x00;
		break;
	case 5: // LSB, no response
		SetLSB(data);
		ret = 0x00;
		break;
	case 133: // Write sector data
		sectorBuffer.at(currentCommandByte - 6) = data;
		ret = 0x00;
		activeMemcard->Write(activeMemcard->GetMemcardDataPointer(), address, 128);
		break;
	case 134: // Checksum 
		ret = CalculateChecksum();
		break;
	case 135: // Acknowledge 1, const value 
		ret = 0x5c;
		break;
	case 136: // Acknowledge 2, const value 
		ret = 0x5d;
		break;
	case 137: // End byte, const value
		ret = (address <= 0x3ff ? 0x47 : 0xff);
		// Flag bit 3 when set indicates directory sector is not read;
		// it is cleared on writes, no$psx thinks its weird to do on
		// writes not reads, so do I.
		activeMemcard->SetFlag(activeMemcard->GetFlag() & ~Flag::DirectoryRead);
		Reset();
		break;
	default: // 6-132: Increment counter with no other action
		sectorBuffer.at(currentCommandByte - 6) = data;
		ret = 0x00;
		break;
	}

	return ret;
}

MemcardPS1Protocol::MemcardPS1Protocol() noexcept
{
	memset(sectorBuffer.data(), 0xff, sectorBuffer.size());

	for (size_t i = 0; i < MAX_PORTS; i++)
	{
		for (size_t j = 0; j < MAX_SLOTS; j++)
		{
			memcards.at(i).at(j) = std::make_unique<MemcardPS1>();
		}
	}
}

MemcardPS1Protocol::~MemcardPS1Protocol() = default;

void MemcardPS1Protocol::Reset()
{
	mode = MemcardPS1Mode::NOT_SET;
	currentCommandByte = 1;
	address = 0;
	sectorBuffer = {};
}

MemcardPS1* MemcardPS1Protocol::GetMemcard(size_t port, size_t slot)
{
	port = std::clamp<size_t>(port, 0, MAX_PORTS);
	slot = std::clamp<size_t>(slot, 0, MAX_SLOTS);
	return memcards.at(port).at(slot).get();
}

void MemcardPS1Protocol::SetActiveMemcard(MemcardPS1* memcardPS1)
{
	activeMemcard = memcardPS1;
}

MemcardPS1Mode MemcardPS1Protocol::GetMemcardMode()
{
	return mode;
}

u8 MemcardPS1Protocol::SendToMemcard(u8 data)
{
	u8 ret = 0xff;

	switch (mode)
	{
	case MemcardPS1Mode::NOT_SET:
		mode = static_cast<MemcardPS1Mode>(data);
		ret = activeMemcard->GetFlag();
		break;
	case MemcardPS1Mode::READ:
		ret = CommandRead(data);
		break;
	case MemcardPS1Mode::STATE:
		ret = CommandState(data);
		break;
	case MemcardPS1Mode::WRITE:
		ret = CommandWrite(data);
		break;
		/*
	case Mode::PS_STATE:
		CommandPSState(data);
		break;
		*/
	default:
		DevCon.Warning("%s(%02X) - Unexpected first command byte", __FUNCTION__, data);
		ret = 0xff;
		Reset();
		break;
	}

	currentCommandByte++;
	return ret;
}
