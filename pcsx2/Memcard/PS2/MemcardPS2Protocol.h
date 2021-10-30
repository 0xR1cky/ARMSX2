
#pragma once

#include "MemcardPS2.h"

class MemcardPS2Protocol
{
private:
	MemcardPS2Mode mode = MemcardPS2Mode::NOT_SET;
public:
	MemcardPS2Protocol();
	~MemcardPS2Protocol();

	MemcardPS2Mode GetMemcardMode();
	u8 SendToMemcard(u8 data);
};

extern MemcardPS2Protocol g_memcardPS2Protocol;
