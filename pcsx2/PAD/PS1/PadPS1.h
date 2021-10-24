
#pragma once

#include "PadPS1Types.h"

class PadPS1
{
private:
	bool analogLED = false;
	PadPS1Controls controlStates;
	PadPS1ControllerType controllerType = PadPS1ControllerType::DISCONNECTED;
	bool multitapBurstQueued = false;
public:
	PadPS1();
	~PadPS1();

	void Init();
	PadPS1ControllerType GetControllerType();
	bool IsMultitapBurstQueued();
	void SetMultitapBurstQueued(bool value);
	PadPS1Controls GetControls();
};
