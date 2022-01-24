
#include "PrecompiledHeader.h"
#include "PadPS2Protocol.h"

#include "Sio2.h"

#define fifoOut g_Sio2.GetFifoOut()

PadPS2Protocol g_PadPS2Protocol;

// Reset mode and byte counters to not set and 0,
// to prepare for the next command. Calling this
// function prematurely, or failing to call it 
// prior to returning the final byte will have
// adverse effects.
void PadPS2Protocol::SoftReset()
{
	std::queue<u8> emptyQueue;
	fifoOut.swap(emptyQueue);
}

void PadPS2Protocol::FullReset()
{
	SoftReset();
}

size_t PadPS2Protocol::GetResponseSize(PadPS2Type padPS2Type)
{
	const u8 type = static_cast<u8>(padPS2Type);
	return type != 0 ? (type & 0x0f) : 16;
}

PadPS2* PadPS2Protocol::GetPad(size_t port, size_t slot)
{
	port = std::clamp<size_t>(port, 0, MAX_PORTS);
	slot = std::clamp<size_t>(slot, 0, MAX_SLOTS);
	return pads.at(port).at(slot).get();
}

void PadPS2Protocol::SetActivePad(PadPS2* pad)
{
	activePad = pad;
}

void PadPS2Protocol::Mystery()
{
	if (!activePad->IsInConfigMode())
	{
		DevCon.Warning("%s(%02X) called outside of config mode", __FUNCTION__);
		return;
	}

	fifoOut.push(0x00);
	fifoOut.push(0x00);
	fifoOut.push(0x02);
	fifoOut.push(0x00);
	fifoOut.push(0x00);
	fifoOut.push(0x5a);
}

void PadPS2Protocol::ButtonQuery()
{
	if (!activePad->IsInConfigMode())
	{
		DevCon.Warning("%s() called outside of config mode", __FUNCTION__);
		return;
	}

	// TODO: Digital mode should respond all 0x00
	fifoOut.push(0xff);
	fifoOut.push(0xff);
	fifoOut.push(0x03);
	fifoOut.push(0x00);
	fifoOut.push(0x00);
	fifoOut.push(0x5a);
}

void PadPS2Protocol::Poll()
{
	activePad->Debug_Poll();
	fifoOut.push(activePad->GetDigitalByte1());
	fifoOut.push(activePad->GetDigitalByte2());

	if (activePad->GetPadType() == PadPS2Type::ANALOG || activePad->GetPadType() == PadPS2Type::DUALSHOCK2)
	{
		fifoOut.push(activePad->GetAnalog(PS2Analog::RIGHT_X));
		fifoOut.push(activePad->GetAnalog(PS2Analog::RIGHT_Y));
		fifoOut.push(activePad->GetAnalog(PS2Analog::LEFT_X));
		fifoOut.push(activePad->GetAnalog(PS2Analog::LEFT_Y));

		if (activePad->GetPadType() == PadPS2Type::DUALSHOCK2)
		{
			while (fifoOut.size() < Poll::DUALSHOCK2_RESPONSE_LENGTH)	
			{
				const size_t pressureIndex = fifoOut.size() - Poll::PRESSURE_OFFSET;
				fifoOut.push(activePad->GetButton(static_cast<PS2Button>(pressureIndex)));
			}
		}
	}
}

void PadPS2Protocol::Config(u8 enterConfig)
{
	if (enterConfig)
	{
		if (!activePad->IsInConfigMode())
		{
			activePad->SetInConfigMode(true);
		}
		else
		{
			DevCon.Warning("%s(%02X) Unexpected enter while already in config mode", __FUNCTION__, enterConfig);
		}
	}
	else
	{
		if (activePad->IsInConfigMode())
		{
			activePad->SetInConfigMode(false);
		}
		else
		{
			DevCon.Warning("%s(%02X) Unexpected exit while not in config mode", __FUNCTION__, enterConfig);
		}
	}

	Poll();
}

void PadPS2Protocol::ModeSwitch(std::queue<u8> &data)
{
	if (!activePad->IsInConfigMode())
	{
		DevCon.Warning("%s(queue) called outside of config mode", __FUNCTION__);
		return;
	}

	const u8 newAnalogStatus = data.front();
	data.pop();
	activePad->SetAnalogLight(newAnalogStatus);

	if (newAnalogStatus)
	{
		activePad->SetPadType(PadPS2Type::ANALOG);
	}
	else
	{
		activePad->SetPadType(PadPS2Type::DIGITAL);
	}

	const u8 newLockStatus = data.front();
	data.pop();
	activePad->SetAnalogLocked(newLockStatus == ModeSwitch::ANALOG_LOCK);
	fifoOut.push(0x00);
	fifoOut.push(0x00);
	fifoOut.push(0x00);
	fifoOut.push(0x00);
	fifoOut.push(0x00);
	fifoOut.push(0x00);
}

void PadPS2Protocol::StatusInfo()
{
	if (!activePad->IsInConfigMode())
	{
		DevCon.Warning("%s() called outside of config mode", __FUNCTION__);
		return;
	}

	// Thanks PS2SDK!
	// Controller model, 3 = DS2, 1 = PS1/Guitar/Others
	fifoOut.push(static_cast<u8>(activePad->GetPadPhysicalType()));
	// "numModes", presumably the number of modes the controller has.
	// These modes are actually returned later in Constant3.
	fifoOut.push(0x02);
	// Is the analog light on or not.
	fifoOut.push(activePad->IsAnalogLightOn());
	// Number of actuators. Presumably vibration motors.
	fifoOut.push(0x02);
	// "numActComb". There's references to command 0x47 as "comb"
	// in old Lilypad code and PS2SDK, presumably this is the controller
	// telling the PS2 how many times to invoke the 0x47 command (once,
	// in contrast to the two runs of 0x46 and 0x4c)
	fifoOut.push(0x01);
	fifoOut.push(0x00);
}

void PadPS2Protocol::Constant1(u8 stage)
{
	if (!activePad->IsInConfigMode())
	{
		DevCon.Warning("%s(%02X) called outside of config mode", __FUNCTION__, stage);
		return;
	}

	fifoOut.push(0x00);
	fifoOut.push(0x00);
	fifoOut.push(activePad->GetPadPhysicalType() == PadPS2Physical::STANDARD ? 0x00 : 0x01);
	
	if (stage)
	{
		fifoOut.push(activePad->GetPadPhysicalType() == PadPS2Physical::STANDARD ? 0x00 : 0x01);
	}
	else 
	{
		fifoOut.push(0x02);
	}

	if (stage)
	{
		fifoOut.push(activePad->GetPadPhysicalType() == PadPS2Physical::STANDARD ? 0x00 : 0x01);
	}
	else
	{
		fifoOut.push(0x00);
	}

	fifoOut.push(stage ? 0x14 : 0x0a);
}

void PadPS2Protocol::Constant2()
{
	if (!activePad->IsInConfigMode())
	{
		DevCon.Warning("%s() called outside of config mode", __FUNCTION__);
		return;
	}

	fifoOut.push(0x00);
	fifoOut.push(0x00);
	fifoOut.push(0x02);
	fifoOut.push(0x00);
	fifoOut.push(activePad->GetPadPhysicalType() == PadPS2Physical::STANDARD ? 0x00 : 0x01);
	fifoOut.push(0x00);
}

void PadPS2Protocol::Constant3(u8 stage)
{
	if (!activePad->IsInConfigMode())
	{
		DevCon.Warning("%s(%02X) called outside of config mode", __FUNCTION__, stage);
		return;
	}

	fifoOut.push(0x00);
	fifoOut.push(0x00);
	fifoOut.push(0x00);
	fifoOut.push(stage ? 0x07 : 0x04);
	fifoOut.push(0x00);
	fifoOut.push(0x00);
}

void PadPS2Protocol::VibrationMap()
{
	if (!activePad->IsInConfigMode())
	{
		DevCon.Warning("%s() called outside of config mode", __FUNCTION__);
		return;
	}

	fifoOut.push(0x00);
	fifoOut.push(0x01);
	fifoOut.push(0xff);
	fifoOut.push(0xff);
	fifoOut.push(0xff);
	fifoOut.push(0xff);
}

void PadPS2Protocol::ResponseBytes(std::queue<u8> &data)
{
	if (!activePad->IsInConfigMode())
	{
		DevCon.Warning("%s(queue) called outside of config mode", __FUNCTION__);
		return;
	}

	const u8 responseBytesLSB = data.front();
	data.pop();
	const u8 responseBytes2nd = data.front();
	data.pop();
	const u8 responseBytesMSB = data.front();
	data.pop();

	const u32 responseBytes = responseBytesLSB | (responseBytes2nd << 8) | (responseBytesMSB << 16);

	switch (responseBytes)
	{
		case ResponseBytes::DIGITAL:
			activePad->SetAnalogLight(false);
			activePad->SetPadType(PadPS2Type::DIGITAL);
			break;
		case ResponseBytes::ANALOG:
			activePad->SetAnalogLight(true);
			activePad->SetPadType(PadPS2Type::ANALOG);
			break;
		case ResponseBytes::DUALSHOCK2:
			activePad->SetAnalogLight(true);
			activePad->SetPadType(PadPS2Type::DUALSHOCK2);
			break;
	}

	fifoOut.push(0x00);
	fifoOut.push(0x00);
	fifoOut.push(0x00);
	fifoOut.push(0x00);
	fifoOut.push(0x00);
	fifoOut.push(0x5a);
}

PadPS2Protocol::PadPS2Protocol()
{
	for (size_t i = 0; i < MAX_PORTS; i++)
	{
		for (size_t j = 0; j < MAX_SLOTS; j++)
		{
			pads.at(i).at(j) = std::make_unique<PadPS2>();
		}
	}
}

PadPS2Protocol::~PadPS2Protocol() = default;

PadPS2Mode PadPS2Protocol::GetPadMode()
{
	return mode;
}

std::queue<u8> PadPS2Protocol::SendToPad(std::queue<u8> &data)
{
	const u8 deviceTypeByte = data.front();
	assert(static_cast<Sio2Mode>(deviceTypeByte) == Sio2Mode::PAD, "PadPS2Protocol was initiated, but this SIO2 command is targeting another device!");
	data.pop();
	fifoOut.push(0x00);

	const u8 commandByte = data.front();
	data.pop();
	fifoOut.push(static_cast<u8>(activePad->IsInConfigMode() ? PadPS2Type::CONFIG : activePad->GetPadType()));
	fifoOut.push(0x5a);

	const u8 frontByte = data.front();
	// Do not pop; let the switch cases do this, if and only if they actually utilize this
	// value as a param for their function.

	switch (static_cast<PadPS2Mode>(commandByte))
	{
		case PadPS2Mode::MYSTERY:
			Mystery();
			break;
		case PadPS2Mode::BUTTON_QUERY:
			ButtonQuery();
			break;
		case PadPS2Mode::POLL:
			Poll();
			break;
		case PadPS2Mode::CONFIG:
			data.pop();
			Config(frontByte);
			break;
		case PadPS2Mode::MODE_SWITCH:
			ModeSwitch(data);
			break;
		case PadPS2Mode::STATUS_INFO:
			StatusInfo();
			break;
		case PadPS2Mode::CONST_1:
			data.pop();
			Constant1(frontByte);
			break;
		case PadPS2Mode::CONST_2:
			Constant2();
			break;
		case PadPS2Mode::CONST_3:
			data.pop();
			Constant3(frontByte);
			break;
		case PadPS2Mode::VIBRATION_MAP:
			VibrationMap();
			break;
		case PadPS2Mode::RESPONSE_BYTES:
			ResponseBytes(data);
			break;
		default:
			DevCon.Warning("%s(queue) Unhandled PadPS2Mode (%02X)", __FUNCTION__, commandByte);
			break;
	}

	return fifoOut;
}

