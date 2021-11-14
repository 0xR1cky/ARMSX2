
#pragma once

enum class Sio2Mode
{
	NOT_SET = 0xff,
	DUD = 0x00,
	PAD = 0x01,
	MULTITAP = 0x21,
	INFRARED = 0x61,
	MEMCARD = 0x81
};

namespace Send3
{
	static constexpr u32 PORT = 0x01;
}

namespace Sio2Ctrl
{
	static constexpr u32 START_TRANSFER = 0x1;
	static constexpr u32 RESET = 0xc;
	static constexpr u32 PORT = 0x2000;
}

namespace Recv1
{
	static constexpr u32 DISCONNECTED = 0x1d100;
	static constexpr u32 CONNECTED = 0x1100;
}

namespace Recv2
{
	static constexpr u32 DEFAULT = 0xf;
}

namespace Recv3
{
	static constexpr u32 DEFAULT = 0x0;
}
