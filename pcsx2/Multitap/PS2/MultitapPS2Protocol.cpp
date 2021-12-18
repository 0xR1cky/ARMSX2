
#include "PrecompiledHeader.h"
#include "MultitapPS2Protocol.h"

#include "SioTypes.h"

MultitapPS2Protocol g_MultitapPS2Protocol;

u8 MultitapPS2Protocol::PadSupportCheck(u8 data)
{
	switch (currentCommandByte)
	{
		case 2:
			return 0x5a;
		case 3:
			return 0x04;
		case 4:
			return 0x00;
		case 5: 
			return 0x5a;
		default:
			return 0x00;
	}
}

u8 MultitapPS2Protocol::MemcardSupportCheck(u8 data)
{
	switch (currentCommandByte)
	{
		case 2:
			return 0x5a;
		case 3:
			return 0x04;
		case 4:
			return 0x00;
		case 5:
			return 0x5a;
		default:
			return 0x00;
	}
}

u8 MultitapPS2Protocol::SelectPad(u8 data)
{
	switch (currentCommandByte)
	{
		case 2:
			if (data >= 0 && data < MAX_SLOTS)
			{
				activeSlot = data;
			}
			else 
			{
				activeSlot = 0xff;
			}
			
			return 0x5a;
		case 3:
			return 0x00;
		case 4:
			return 0x00;
		case 5:
			return activeSlot;
		case 6:
			return (activeSlot != 0xff ? 0x5a : 0x66);
		default:
			return 0x00;
	}
}

u8 MultitapPS2Protocol::SelectMemcard(u8 data)
{
	switch (currentCommandByte)
	{
		case 2:
			if (data >= 0 && data < MAX_SLOTS)
			{
				activeSlot = data;
			}
			else
			{
				activeSlot = 0xff;
			}

			return 0x5a;
		case 3:
			return 0x00;
		case 4:
			return 0x00;
		case 5:
			return activeSlot;
		case 6:
			return (activeSlot != 0xff ? 0x5a : 0x66);
		default:
			return 0x00;
	}
}

MultitapPS2Protocol::MultitapPS2Protocol() = default;
MultitapPS2Protocol::~MultitapPS2Protocol() = default;

void MultitapPS2Protocol::SoftReset()
{
	mode = MultitapPS2Mode::NOT_SET;
	currentCommandByte = 1;
}

void MultitapPS2Protocol::FullReset()
{
	activeSlot = 0;
}

u8 MultitapPS2Protocol::GetActiveSlot()
{
	return activeSlot;
}

u8 MultitapPS2Protocol::SendToMultitap(u8 data)
{
	u8 ret = 0xff;

	if (currentCommandByte == 1)
	{
		mode = static_cast<MultitapPS2Mode>(data);
		ret = 0x80;
	}
	else
	{
		switch (mode)
		{
			case MultitapPS2Mode::PAD_SUPPORT_CHECK:
				ret = PadSupportCheck(data);
				break;
			case MultitapPS2Mode::MEMCARD_SUPPORT_CHECK:
				ret = MemcardSupportCheck(data);
				break;
			case MultitapPS2Mode::SELECT_PAD:
				ret = SelectPad(data);
				break;
			case MultitapPS2Mode::SELECT_MEMCARD:
				ret = SelectMemcard(data);
				break;
		}
	}
	
	currentCommandByte++;
	return ret;
}
