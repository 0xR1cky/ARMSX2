
#pragma once

enum class PadPS1Mode
{
	NOT_SET = 0x00,
	POLL = 0x42,
	CONFIG = 0x43,
	SET_LED = 0x44,
	GET_LED = 0x45,
	STATUS_1 = 0x46,
	STATUS_2 = 0x47,
	STATUS_3 = 0x4c,
	RUMBLE = 0x4d,
	MYSTERY = 0x48
};

enum class PadPS1ControllerType
{
	MOUSE = 0x12,
	NEGCON = 0x23,
	KONAMI_LIGHTGUN = 0x31,
	PAD_DIGITAL = 0x41,
	ANALOG_FLIGHT_STICK = 0x53,
	NAMCO_LIGHTGUN = 0x63,
	PAD_ANALOG = 0x73,
	MULTITAP = 0x80,
	JOGCON = 0xe3,
	CONFIG_MODE = 0xf3,
	DISCONNECTED = 0xff
};

enum class PadPS1MotorType
{
	LARGE_MOTOR = 0x00,
	SMALL_MOTOR = 0x01
};

struct PadPS1Controls
{
	u8 digitalButtons1 = 0xff;
	u8 digitalButtons2 = 0xff;
	u8 rightAnalogX = 0x80;
	u8 rightAnalogY = 0x80;
	u8 leftAnalogX = 0x80;
	u8 leftAnalogY = 0x80;
};
