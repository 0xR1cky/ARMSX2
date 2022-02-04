
#pragma once

#include "MultitapPS2Types.h"

class MultitapPS2Protocol
{
private:
	u8 activeSlot = 0;

	void SupportCheck();
	void Select();

public:
	MultitapPS2Protocol();
	~MultitapPS2Protocol();

	void SoftReset();
	void FullReset();

	u8 GetActiveSlot();

	void SendToMultitap();
};

extern MultitapPS2Protocol g_MultitapPS2Protocol;
