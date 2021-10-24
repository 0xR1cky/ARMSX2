
#pragma once

#include "PadPS2Types.h"

class PadPS2Protocol
{
private:
	PadPS2Mode mode = PadPS2Mode::NOT_SET;
public:
	PadPS2Protocol();
	~PadPS2Protocol();

	PadPS2Mode GetPadMode();
	u8 SendToPad(u8 data);
};

extern PadPS2Protocol g_padPS2Protocol;
