
#pragma once

enum class MultitapPS2Mode 
{
	NOT_SET = 0xff,
	PAD_SUPPORT_CHECK = 0x12,
	MEMCARD_SUPPORT_CHECK = 0x13,
	SELECT_PAD = 0x21,
	SELECT_MEMCARD = 0x22,
};
