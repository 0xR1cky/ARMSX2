
#include "PrecompiledHeader.h"
#include "PadPS1.h"

PadPS1::PadPS1() = default;
PadPS1::~PadPS1() = default;

void PadPS1::Init()
{

}

PadPS1ControllerType PadPS1::GetControllerType()
{
	return controllerType;
}

bool PadPS1::IsMultitapBurstQueued()
{
	return multitapBurstQueued;
}

void PadPS1::SetMultitapBurstQueued(bool value)
{
	multitapBurstQueued = value;
}

PadPS1Controls PadPS1::GetControls()
{
	return controlStates;
}
