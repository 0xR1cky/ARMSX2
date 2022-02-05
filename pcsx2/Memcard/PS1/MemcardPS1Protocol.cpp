
#include "PrecompiledHeader.h"
#include "MemcardPS1Protocol.h"

MemcardPS1Protocol g_memcardPS1Protocol;

u8 MemcardPS1Protocol::GetMSB()
{
	return address & 0x00ff;
}

u8 MemcardPS1Protocol::GetLSB()
{
	return ((address & 0xff00) >> 8);
}

void MemcardPS1Protocol::SetMSB(u8 data)
{
	u16 mask = data;
	address &= mask;
}

void MemcardPS1Protocol::SetLSB(u8 data)
{
	u16 mask = (data << 8);
	address &= mask;
	activeMemcard->SetSector(address);
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
		checksum ^= data;
		ret = 0x00;
		break;
	case 5: // LSB, no response
		SetLSB(data);
		checksum ^= data;
		ret = 0x00;
		break;
	case 6: // Acknowledge 1, const value 
		ret = 0x5c;
		break;
	case 7: // Acknowledge 2, const value 
		ret = 0x5d;
		break;
	// TODO: Case 8 and 9 should respond 0xff if sector is out of bounds
	case 8: // Echo back MSB 
		ret = GetMSB();
		break;
	case 9: // Echo back LSB
		ret = GetLSB();
		break;
	case 10: // Read sector data
		sectorBuffer = activeMemcard->Read(static_cast<u8>(SectorSize::PS1));
		ret = sectorBuffer.front();
		checksum ^= ret;
		sectorBuffer.pop();
		break;
	case 138: // Checksum 
		ret = checksum;
		break;
	case 139: // End byte, const value
		ret = 0x47;
		SoftReset();
		break;
	default: // 11-137: Continue to reply from the read buffer
		ret = sectorBuffer.front();
		checksum ^= ret;
		sectorBuffer.pop();
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
		SoftReset();
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
		checksum ^= data;
		ret = 0x00;
		break;
	case 5: // LSB, no response
		SetLSB(data);
		checksum ^= data;
		ret = 0x00;
		break;
	case 133: // Write sector data
		sectorBuffer.push(data);
		checksum ^= data;
		ret = 0x00;
		activeMemcard->Write(sectorBuffer);
		break;
	case 134: // Checksum 
		ret = checksum;
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
		SoftReset();
		break;
	default: // 6-132: Increment counter with no other action
		sectorBuffer.push(data);
		checksum ^= data;
		ret = 0x00;
		break;
	}

	return ret;
}

MemcardPS1Protocol::MemcardPS1Protocol() = default;
MemcardPS1Protocol::~MemcardPS1Protocol() = default;

void MemcardPS1Protocol::SoftReset()
{
	mode = MemcardPS1Mode::NOT_SET;
	currentCommandByte = 1;
	checksum = 0x00;
	address = 0;
	
	while (!sectorBuffer.empty())
	{
		sectorBuffer.pop();
	}
}

void MemcardPS1Protocol::FullReset()
{
	SoftReset();
}

void MemcardPS1Protocol::SetActiveMemcard(Memcard* memcard)
{
	activeMemcard = memcard;
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
		SoftReset();
		break;
	}

	currentCommandByte++;
	return ret;
}
