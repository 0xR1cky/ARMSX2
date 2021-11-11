
#pragma once

#include "MemcardPS2Types.h"

class MemcardPS2
{
private:
	u8 terminator = 0x55;
public:
	MemcardPS2();
	~MemcardPS2();

	u8 GetTerminator();

	void SetTerminator(u8 data);
};
