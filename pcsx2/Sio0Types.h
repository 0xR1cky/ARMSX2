
#pragma once

enum class Sio0Mode
{
	NOT_SET = 0xff,
	PAD = 0x01,
	PAD_MULTITAP_2 = 0x02,
	PAD_MULTITAP_3 = 0x03,
	PAD_MULTITAP_4 = 0x04,
	MEMCARD = 0x81,
	MEMCARD_MULTITAP_2 = 0x82,
	MEMCARD_MULTITAP_3 = 0x83,
	MEMCARD_MULTITAP_4 = 0x84
};

namespace SioStat
{
	static constexpr u32 TX_READY = 0x00000001;
	static constexpr u32 RX_NOT_EMPTY = 0x00000002;
	static constexpr u32 TX_DONE = 0x00000004;
	static constexpr u32 IRQ = 0x00000200;
}

namespace SioCtrl
{
	static constexpr u16 TX_ENABLE = 0x0001;
	static constexpr u16 RX_ENABLE = 0x0004;
	static constexpr u16 ACKNOWLEDGE = 0x0010;
	static constexpr u16 RESET = 0x0040;
	static constexpr u16 PORT = 0x2000;
}
