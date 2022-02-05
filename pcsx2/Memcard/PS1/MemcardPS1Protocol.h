
#pragma once

#include "Memcard/Memcard.h"
#include "SioTypes.h"

#include <queue>

class MemcardPS1Protocol
{
private:
	Memcard* activeMemcard = nullptr;
	MemcardPS1Mode mode = MemcardPS1Mode::NOT_SET;
	// Begins at 1; the Sio0 shell will always respond to byte 0 without notifying
	// the memcard (byte 0 is just telling Sio0 which device to talk to, with a 0 reply)
	u8 currentCommandByte = 1;
	u8 checksum = 0x00;
	u16 address = 0;
	std::queue<u8> sectorBuffer;

	u8 GetMSB();
	u8 GetLSB();

	void SetMSB(u8 byte);
	void SetLSB(u8 byte);
	
	// Read from a memcard (0x52)
	u8 CommandRead(u8 data);
	// Request status info from the memcard (0x53)
	u8 CommandState(u8 data);
	// Write to a memcard (0x57)
	u8 CommandWrite(u8 data);
	// Some fangled pocketstation status command (0x58)
	//void CommandPSState(u8 data);
public:
	MemcardPS1Protocol();
	~MemcardPS1Protocol();

	void SoftReset();
	void FullReset();

	void SetActiveMemcard(Memcard* memcard);
	MemcardPS1Mode GetMemcardMode();
	// Handler for all command bytes, invokes command function based on mode
	u8 SendToMemcard(u8 data);
};

extern MemcardPS1Protocol g_memcardPS1Protocol;
