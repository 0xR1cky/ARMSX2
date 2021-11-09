
#pragma once

#include "MultitapPS2Types.h"

class MultitapPS2Protocol
{
private:
public:
	MultitapPS2Protocol();
	~MultitapPS2Protocol();

	void Reset();

	u8 SendToMultitap(u8 data);
};

extern MultitapPS2Protocol g_MultitapPS2Protocol;
