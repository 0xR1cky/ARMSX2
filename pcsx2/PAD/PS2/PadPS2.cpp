
#include "PrecompiledHeader.h"
#include "PadPS2.h"

#include "./PAD/PadUtils.h"

PadPS2::PadPS2() = default;
PadPS2::~PadPS2() = default;

bool PadPS2::IsInConfigMode()
{
	return config;
}

bool PadPS2::IsAnalogLightOn()
{
	return analogLight;
}

bool PadPS2::IsAnalogLocked()
{
	return analogLocked;
}

// Some of the "Constant" functions of the protocol are
// called twice back to back but expecting different responses
// each time; this bool tracks whether we are on an even or odd
// call.
bool PadPS2::GetConstantStage()
{
	return constantStage;
}

bool PadPS2::IsConfigResponse()
{
	return configResponse;
}

PadPS2Type PadPS2::GetPadType()
{
	return type;
}

PadPS2Physical PadPS2::GetPadPhysicalType()
{
	return physical;
}

u8 PadPS2::GetDigitalByte1()
{
	return digitalByte1;
}

u8 PadPS2::GetDigitalByte2()
{
	return digitalByte2;
}

u8 PadPS2::GetAnalogLeftX()
{
	return analogLeftX;
}

u8 PadPS2::GetAnalogLeftY()
{
	return analogLeftY;
}

u8 PadPS2::GetAnalogRightX()
{
	return analogRightX;
}

u8 PadPS2::GetAnalogRightY()
{
	return analogRightY;
}

void PadPS2::SetInConfigMode(bool b)
{
	config = b;
}

void PadPS2::SetAnalogLight(bool b)
{
	analogLight = b;
}

void PadPS2::SetAnalogLocked(bool b)
{
	analogLocked = b;
}

// Set the constantStage bool. The third byte sent in the command
// indicates whether this is the first (0) or second (1) time
// this command has been sent.
void PadPS2::SetConstantStage(bool b)
{
	constantStage = b;
}

void PadPS2::SetConfigResponse(bool b)
{
	configResponse = b;
}

void PadPS2::SetPadType(PadPS2Type type)
{
	this->type = type;
}

void PadPS2::SetPadPhysicalType(PadPS2Physical physical)
{
	this->physical = physical;
}

void PadPS2::SetDigitalByte1(u8 data)
{
	digitalByte1 = data;
}

void PadPS2::SetDigitalByte2(u8 data)
{
	digitalByte2 = data;
}

void PadPS2::SetAnalogLeftX(u8 data)
{
	analogLeftX = data;
}

void PadPS2::SetAnalogLeftY(u8 data)
{
	analogLeftY = data;
}

void PadPS2::SetAnalogRightX(u8 data)
{
	analogRightX = data;
}

void PadPS2::SetAnalogRightY(u8 data)
{
	analogRightY = data;
}

void PadPS2::Debug_Poll()
{
	DWORD res = XInputGetState(0, &state);

	if (res != ERROR_SUCCESS)
	{
		DevCon.Warning("%s Xinput error %d", __FUNCTION__, res);
		return;
	}

	WORD xinputButtons = state.Gamepad.wButtons;
	u8 digitalByte1 = 0xff;
	u8 digitalByte2 = 0xff;

	if (xinputButtons & XINPUT_GAMEPAD_BACK)
	{
		digitalByte1 &= DigitalByte1::SELECT;
	}

	if (xinputButtons & XINPUT_GAMEPAD_LEFT_THUMB)
	{
		digitalByte1 &= DigitalByte1::L3;
	}

	if (xinputButtons & XINPUT_GAMEPAD_RIGHT_THUMB)
	{
		digitalByte1 &= DigitalByte1::R3;
	}

	if (xinputButtons & XINPUT_GAMEPAD_START)
	{
		digitalByte1 &= DigitalByte1::START;
	}

	if (xinputButtons & XINPUT_GAMEPAD_DPAD_UP)
	{
		digitalByte1 &= DigitalByte1::UP;
	}

	if (xinputButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
	{
		digitalByte1 &= DigitalByte1::RIGHT;
	}

	if (xinputButtons & XINPUT_GAMEPAD_DPAD_DOWN)
	{
		digitalByte1 &= DigitalByte1::DOWN;
	}

	if (xinputButtons & XINPUT_GAMEPAD_DPAD_LEFT)
	{
		digitalByte1 &= DigitalByte1::LEFT;
	}

	if (state.Gamepad.bLeftTrigger)
	{
		digitalByte2 &= DigitalByte2::L2;
	}

	if (state.Gamepad.bRightTrigger)
	{
		digitalByte2 &= DigitalByte2::R2;
	}

	if (xinputButtons & XINPUT_GAMEPAD_LEFT_SHOULDER)
	{
		digitalByte2 &= DigitalByte2::L1;
	}

	if (xinputButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER)
	{
		digitalByte2 &= DigitalByte2::R1;
	}

	if (xinputButtons & XINPUT_GAMEPAD_Y)
	{
		digitalByte2 &= DigitalByte2::TRIANGLE;
	}

	if (xinputButtons & XINPUT_GAMEPAD_B)
	{
		digitalByte2 &= DigitalByte2::CIRCLE;
	}

	if (xinputButtons & XINPUT_GAMEPAD_A)
	{
		digitalByte2 &= DigitalByte2::CROSS;
	}

	if (xinputButtons & XINPUT_GAMEPAD_X)
	{
		digitalByte2 &= DigitalByte2::SQUARE;
	}

	SetDigitalByte1(digitalByte1);
	SetDigitalByte2(digitalByte2);

	SetAnalogLeftX(Normalize<SHORT>(state.Gamepad.sThumbLX));
	SetAnalogLeftY(0xff - Normalize<SHORT>(state.Gamepad.sThumbLY));
	SetAnalogRightX(Normalize<SHORT>(state.Gamepad.sThumbRX));
	SetAnalogRightY(0xff - Normalize<SHORT>(state.Gamepad.sThumbRY));
}
