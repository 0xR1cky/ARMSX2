
#pragma once

#include "PadPS1.h"
#include "SioTypes.h"
#include <array>

using PadPS1Array = std::array<std::array<std::unique_ptr<PadPS1>, MAX_SLOTS>, MAX_PORTS>;

class PadPS1Protocol
{
private:
	PadPS1Array pads;
	PadPS1* activePad = nullptr;
	PadPS1Mode mode = PadPS1Mode::NOT_SET;
	size_t activePort = 0;
	// Begins at 1; the Sio0 shell will always respond to byte 0 without notifying
	// the pad (byte 0 is just telling Sio0 which device to talk to, with a 0 reply)
	u8 currentCommandByte = 1;

	// Get the number of half-words returned by a PadPS1ControllerType.
	// This value is always the lower nibble of the type, except for a
	// lower nibble of 0 which is actually 16 half-words. At the time of
	// writing, only multitaps use a 0 lower nibble.
	size_t GetResponseSize(PadPS1ControllerType controllerType);

	u8 CommandPoll(u8 data);
	u8 CommandConfig(u8 data);
	u8 CommandSetLED(u8 data);
	u8 CommandGetLED(u8 data);
	u8 CommandStatus1(u8 data);
	u8 CommandStatus2(u8 data);
	u8 CommandStatus3(u8 data);
	u8 CommandRumble(u8 data);
	u8 CommandMystery(u8 data);
public:
	PadPS1Protocol();
	~PadPS1Protocol();

	void Reset();
	PadPS1* GetPad(size_t port, size_t slot);
	void SetActivePad(PadPS1* padPS1);
	PadPS1Mode GetPadMode();
	size_t GetActivePort();
	void SetActivePort(size_t port);
	void SetVibration(PadPS1MotorType motorType, u8 strength);
	u8 SendToPad(u8 data);
};

extern PadPS1Protocol g_padPS1Protocol;
