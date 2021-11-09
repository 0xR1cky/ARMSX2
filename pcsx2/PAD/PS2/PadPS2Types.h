
#pragma once

enum class PadPS2Mode
{
	NOT_SET = 0xff,
	MYSTERY = 0x40,
	BUTTON_QUERY = 0x41,
	POLL = 0x42,
	CONFIG = 0x43,
	MODE_SWITCH = 0x44,
	STATUS_INFO = 0x45,
	CONST_1 = 0x46,
	CONST_2 = 0x47,
	CONST_3 = 0x4c,
	VIBRATION_MAP = 0x4d,
	RESPONSE_BYTES = 0x4f
};

enum class PadPS2Type
{
	DIGITAL = 0x41,
	ANALOG = 0x73,
	DUALSHOCK2 = 0x79,
	CONFIG = 0xf3
};

enum class PadPS2Physical
{
	STANDARD = 0x03,
	GUITAR = 0x01
};

struct ButtonStates
{
	// Pressure capable buttons

	u8 right = 0x00;
	u8 left = 0x00;
	u8 up = 0x00;
	u8 down = 0x00;
	u8 triangle = 0x00;
	u8 circle = 0x00;
	u8 cross = 0x00;
	u8 square = 0x00;
	u8 l1 = 0x00;
	u8 r1 = 0x00;
	u8 l2 = 0x00;
	u8 r2 = 0x00;

	// Digital only buttons

	u8 select = 0x00;
	u8 l3 = 0x00;
	u8 r3 = 0x00;
	u8 start = 0x00;

	// Analog axes

	u8 leftX = 0x7f;
	u8 leftY = 0x7f;
	u8 rightX = 0x7f;
	u8 rightY = 0x7f;
};

// Order matches the order used for pressures in 0x42 poll commands.
// Last four items are not pressure capable.
enum class PS2Button
{
	RIGHT = 0x00,
	LEFT,
	UP,
	DOWN,
	TRIANGLE,
	CIRCLE,
	CROSS,
	SQUARE,
	L1,
	R1,
	L2,
	R2,
	SELECT,
	L3,
	R3,
	START
};

enum class PS2Analog
{
	LEFT_X = 0x00,
	LEFT_Y,
	RIGHT_X,
	RIGHT_Y
};

namespace DigitalByte1
{
	static constexpr u8 SELECT = 0xfe;
	static constexpr u8 L3 = 0xfd;
	static constexpr u8 R3 = 0xfb;
	static constexpr u8 START = 0xf7;
	static constexpr u8 UP = 0xef;
	static constexpr u8 RIGHT = 0xdf;
	static constexpr u8 DOWN = 0xbf;
	static constexpr u8 LEFT = 0x7f;
}

namespace DigitalByte2
{
	static constexpr u8 L2 = 0xfe;
	static constexpr u8 R2 = 0xfd;
	static constexpr u8 L1 = 0xfb;
	static constexpr u8 R1 = 0xf7;
	static constexpr u8 TRIANGLE = 0xef;
	static constexpr u8 CIRCLE = 0xdf;
	static constexpr u8 CROSS = 0xbf;
	static constexpr u8 SQUARE = 0x7f;
}


