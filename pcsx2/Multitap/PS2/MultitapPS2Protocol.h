
#pragma once

#include "MultitapPS2Types.h"

class MultitapPS2Protocol
{
private:
	MultitapPS2Mode mode = MultitapPS2Mode::NOT_SET;
	size_t currentCommandByte = 1;
	u8 activeSlot = 0;

	u8 PadSupportCheck(u8 data);
	u8 MemcardSupportCheck(u8 data);
	u8 SelectPad(u8 data);
	u8 SelectMemcard(u8 data);

public:
	MultitapPS2Protocol();
	~MultitapPS2Protocol();

	void SoftReset();
	void FullReset();

	u8 GetActiveSlot();

	u8 SendToMultitap(u8 data);
};

extern MultitapPS2Protocol g_MultitapPS2Protocol;
