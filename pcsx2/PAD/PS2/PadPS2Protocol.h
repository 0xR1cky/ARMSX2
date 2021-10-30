
#pragma once

#include "PadPS2.h"
#include "SioTypes.h"
#include <array>

using PadPS2Array = std::array<std::array<std::unique_ptr<PadPS2>, MAX_SLOTS>, MAX_PORTS>;

class PadPS2Protocol
{
private:
	bool reset = false;
	PadPS2Array pads;
	PadPS2* activePad = nullptr;
	PadPS2Mode mode = PadPS2Mode::NOT_SET;
	size_t activePort = 0;
	size_t currentCommandByte = 1;

	void Reset();
	size_t GetResponseSize(PadPS2Type padPS2Type);
	
	u8 Mystery(u8 data);
	u8 ButtonQuery(u8 data);
	u8 Poll(u8 data);
	u8 Config(u8 data);
	u8 ModeSwitch(u8 data);
	u8 StatusInfo(u8 data);
	u8 Constant1(u8 data);
	u8 Constant2(u8 data);
	u8 Constant3(u8 data);
	u8 VibrationMap(u8 data);
	u8 ResponseBytes(u8 data);
public:
	PadPS2Protocol();
	~PadPS2Protocol();

	bool IsReset();
	PadPS2Mode GetPadMode();
	PadPS2* GetPad(size_t port, size_t slot);
	void SetActivePad(PadPS2* pad);

	u8 SendToPad(u8 data);
};

extern PadPS2Protocol g_padPS2Protocol;
