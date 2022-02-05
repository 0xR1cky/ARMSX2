
#pragma once

#include "Memcard/Memcard.h"
#include "MemcardPS1.h"
#include "SioTypes.h"

using MemcardPS1Array = std::array<std::array<std::unique_ptr<Memcard>, MAX_SLOTS>, MAX_PORTS>;

class MemcardPS1Protocol
{
private:
	MemcardPS1Array memcards;
	Memcard* activeMemcard = nullptr;
	MemcardPS1Mode mode = MemcardPS1Mode::NOT_SET;
	// Begins at 1; the Sio0 shell will always respond to byte 0 without notifying
	// the memcard (byte 0 is just telling Sio0 which device to talk to, with a 0 reply)
	u8 currentCommandByte = 1;
	u16 address = 0;
	std::array<u8, SECTOR_SIZE> sectorBuffer = {};

	u8 GetMSB();
	void SetMSB(u8 byte);
	u8 GetLSB();
	void SetLSB(u8 byte);
	u8 CalculateChecksum();
	
	// Read from a memcard (0x52)
	u8 CommandRead(u8 data);
	// Request status info from the memcard (0x53)
	u8 CommandState(u8 data);
	// Write to a memcard (0x57)
	u8 CommandWrite(u8 data);
	// Some fangled pocketstation status command (0x58)
	//void CommandPSState(u8 data);
public:
	MemcardPS1Protocol() noexcept;
	~MemcardPS1Protocol();

	void Reset();
	MemcardPS1* GetMemcard(size_t port, size_t slot);
	void SetActiveMemcard(Memcard* memcard);
	MemcardPS1Mode GetMemcardMode();
	// Handler for all command bytes, invokes command function based on mode
	u8 SendToMemcard(u8 data);
};

extern MemcardPS1Protocol g_memcardPS1Protocol;
