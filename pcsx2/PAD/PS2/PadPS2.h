
#pragma once

#include "PadPS2Types.h"
#include "Xinput.h"

class PadPS2
{
private:
	bool config = false;
	bool analogLight = false;
	bool analogLocked = false;
	bool constantStage = false;
	bool configResponse = false;
	PadPS2Type type = PadPS2Type::DIGITAL;
	PadPS2Physical physical = PadPS2Physical::STANDARD;
	ButtonStates buttonStates;

	XINPUT_STATE state;
public:
	PadPS2();
	~PadPS2();

	bool IsInConfigMode();
	bool IsAnalogLightOn();
	bool IsAnalogLocked();
	bool GetConstantStage();
	bool IsConfigResponse();
	PadPS2Type GetPadType();
	PadPS2Physical GetPadPhysicalType();

	u8 GetDigitalByte1();
	u8 GetDigitalByte2();
	u8 GetButton(PS2Button button);
	u8 GetAnalog(PS2Analog analog);

	void SetInConfigMode(bool b);
	void SetAnalogLight(bool b);
	void SetAnalogLocked(bool b);
	void SetConstantStage(bool b);
	void SetConfigResponse(bool b);
	void SetPadType(PadPS2Type type);
	void SetPadPhysicalType(PadPS2Physical physical);

	void SetButton(PS2Button button, u8 data);
	void SetAnalog(PS2Analog analog, u8 data);

	void Debug_Poll();
};
