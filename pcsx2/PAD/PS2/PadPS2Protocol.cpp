
#include "PrecompiledHeader.h"
#include "PadPS2Protocol.h"

#include "Sio2.h"

PadPS2Protocol g_PadPS2Protocol;

void PadPS2Protocol::SoftReset()
{
	
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

	g_Sio2.GetFifoOut().push(0x00);
	g_Sio2.GetFifoOut().push(0x00);
	g_Sio2.GetFifoOut().push(0x02);
	g_Sio2.GetFifoOut().push(0x00);
	g_Sio2.GetFifoOut().push(0x00);
	g_Sio2.GetFifoOut().push(0x5a);
}

void PadPS2Protocol::ButtonQuery()
{
	if (!activePad->IsInConfigMode())
	{
		DevCon.Warning("%s() called outside of config mode", __FUNCTION__);
		return;
	}

	// TODO: Digital mode should respond all 0x00
	g_Sio2.GetFifoOut().push(0xff);
	g_Sio2.GetFifoOut().push(0xff);
	g_Sio2.GetFifoOut().push(0x03);
	g_Sio2.GetFifoOut().push(0x00);
	g_Sio2.GetFifoOut().push(0x00);
	g_Sio2.GetFifoOut().push(0x5a);
}

void PadPS2Protocol::Poll()
{
	activePad->Debug_Poll();
	g_Sio2.GetFifoOut().push(activePad->GetDigitalByte1());
	g_Sio2.GetFifoOut().push(activePad->GetDigitalByte2());
	g_Sio2.GetFifoIn().pop();
	g_Sio2.GetFifoIn().pop();

	// Some games will configure the controller to send analog values... and then continue
	// to only send digital requests. Check fifo size to catch these scenarios.
	if (g_Sio2.GetFifoIn().size() >= 4 && (activePad->GetPadType() == PadPS2Type::ANALOG || activePad->GetPadType() == PadPS2Type::DUALSHOCK2))
	{
		g_Sio2.GetFifoOut().push(activePad->GetAnalog(PS2Analog::RIGHT_X));
		g_Sio2.GetFifoOut().push(activePad->GetAnalog(PS2Analog::RIGHT_Y));
		g_Sio2.GetFifoOut().push(activePad->GetAnalog(PS2Analog::LEFT_X));
		g_Sio2.GetFifoOut().push(activePad->GetAnalog(PS2Analog::LEFT_Y));
		g_Sio2.GetFifoIn().pop();
		g_Sio2.GetFifoIn().pop();
		g_Sio2.GetFifoIn().pop();
		g_Sio2.GetFifoIn().pop();

		// Any remaining fifo in bytes signal pressures are requested. As above, some developers sniffed
		// glue, so we check BOTH configured mode and fifo size remaining.
		if (g_Sio2.GetFifoIn().size() > 0 && activePad->GetPadType() == PadPS2Type::DUALSHOCK2)
		{
			while (g_Sio2.GetFifoOut().size() < Poll::DUALSHOCK2_RESPONSE_LENGTH)	
			{
				const size_t pressureIndex = g_Sio2.GetFifoOut().size() - Poll::PRESSURE_OFFSET;
				g_Sio2.GetFifoOut().push(activePad->GetButton(static_cast<PS2Button>(pressureIndex)));
			}
		}
	}
}

void PadPS2Protocol::Config()
{
	const u8 enterConfig = g_Sio2.GetFifoIn().front();
	
	while (!g_Sio2.GetFifoIn().empty())
	{
		g_Sio2.GetFifoIn().pop();
		g_Sio2.GetFifoOut().push(0x00);
	}

	if (enterConfig)
	{
		if (!activePad->IsInConfigMode())
		{
			activePad->SetInConfigMode(true);
		}
		else
		{
			DevCon.Warning("%s() Unexpected enter while already in config mode", __FUNCTION__);
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
			DevCon.Warning("%s() Unexpected exit while not in config mode", __FUNCTION__);
		}
	}
}

void PadPS2Protocol::ModeSwitch()
{
	if (!activePad->IsInConfigMode())
	{
		DevCon.Warning("%s() called outside of config mode", __FUNCTION__);
		return;
	}

	const u8 newAnalogStatus = g_Sio2.GetFifoIn().front();
	g_Sio2.GetFifoIn().pop();
	activePad->SetAnalogLight(newAnalogStatus);

	if (newAnalogStatus)
	{
		activePad->SetPadType(PadPS2Type::ANALOG);
	}
	else
	{
		activePad->SetPadType(PadPS2Type::DIGITAL);
	}

	const u8 newLockStatus = g_Sio2.GetFifoIn().front();
	g_Sio2.GetFifoIn().pop();
	activePad->SetAnalogLocked(newLockStatus == ModeSwitch::ANALOG_LOCK);
	g_Sio2.GetFifoOut().push(0x00);
	g_Sio2.GetFifoOut().push(0x00);
	g_Sio2.GetFifoOut().push(0x00);
	g_Sio2.GetFifoOut().push(0x00);
	g_Sio2.GetFifoOut().push(0x00);
	g_Sio2.GetFifoOut().push(0x00);
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
	g_Sio2.GetFifoOut().push(static_cast<u8>(activePad->GetPadPhysicalType()));
	// "numModes", presumably the number of modes the controller has.
	// These modes are actually returned later in Constant3.
	g_Sio2.GetFifoOut().push(0x02);
	// Is the analog light on or not.
	g_Sio2.GetFifoOut().push(activePad->IsAnalogLightOn());
	// Number of actuators. Presumably vibration motors.
	g_Sio2.GetFifoOut().push(0x02);
	// "numActComb". There's references to command 0x47 as "comb"
	// in old Lilypad code and PS2SDK, presumably this is the controller
	// telling the PS2 how many times to invoke the 0x47 command (once,
	// in contrast to the two runs of 0x46 and 0x4c)
	g_Sio2.GetFifoOut().push(0x01);
	g_Sio2.GetFifoOut().push(0x00);
}

void PadPS2Protocol::Constant1()
{
	if (!activePad->IsInConfigMode())
	{
		DevCon.Warning("%s() called outside of config mode", __FUNCTION__);
		return;
	}

	const u8 stage = g_Sio2.GetFifoIn().front();
	g_Sio2.GetFifoIn().pop();

	g_Sio2.GetFifoOut().push(0x00);
	g_Sio2.GetFifoOut().push(0x00);
	g_Sio2.GetFifoOut().push(activePad->GetPadPhysicalType() == PadPS2Physical::STANDARD ? 0x00 : 0x01);
	
	if (stage)
	{
		g_Sio2.GetFifoOut().push(activePad->GetPadPhysicalType() == PadPS2Physical::STANDARD ? 0x00 : 0x01);
	}
	else 
	{
		g_Sio2.GetFifoOut().push(0x02);
	}

	if (stage)
	{
		g_Sio2.GetFifoOut().push(activePad->GetPadPhysicalType() == PadPS2Physical::STANDARD ? 0x00 : 0x01);
	}
	else
	{
		g_Sio2.GetFifoOut().push(0x00);
	}

	g_Sio2.GetFifoOut().push(stage ? 0x14 : 0x0a);
}

void PadPS2Protocol::Constant2()
{
	if (!activePad->IsInConfigMode())
	{
		DevCon.Warning("%s() called outside of config mode", __FUNCTION__);
		return;
	}

	g_Sio2.GetFifoOut().push(0x00);
	g_Sio2.GetFifoOut().push(0x00);
	g_Sio2.GetFifoOut().push(0x02);
	g_Sio2.GetFifoOut().push(0x00);
	g_Sio2.GetFifoOut().push(activePad->GetPadPhysicalType() == PadPS2Physical::STANDARD ? 0x00 : 0x01);
	g_Sio2.GetFifoOut().push(0x00);
}

void PadPS2Protocol::Constant3()
{
	if (!activePad->IsInConfigMode())
	{
		DevCon.Warning("%s() called outside of config mode", __FUNCTION__);
		return;
	}

	const u8 stage = g_Sio2.GetFifoIn().front();
	g_Sio2.GetFifoIn().pop();

	g_Sio2.GetFifoOut().push(0x00);
	g_Sio2.GetFifoOut().push(0x00);
	g_Sio2.GetFifoOut().push(0x00);
	g_Sio2.GetFifoOut().push(stage ? 0x07 : 0x04);
	g_Sio2.GetFifoOut().push(0x00);
	g_Sio2.GetFifoOut().push(0x00);
}

void PadPS2Protocol::VibrationMap()
{
	if (!activePad->IsInConfigMode())
	{
		DevCon.Warning("%s() called outside of config mode", __FUNCTION__);
		return;
	}

	g_Sio2.GetFifoOut().push(0x00);
	g_Sio2.GetFifoOut().push(0x01);
	g_Sio2.GetFifoOut().push(0xff);
	g_Sio2.GetFifoOut().push(0xff);
	g_Sio2.GetFifoOut().push(0xff);
	g_Sio2.GetFifoOut().push(0xff);
}

void PadPS2Protocol::ResponseBytes()
{
	if (!activePad->IsInConfigMode())
	{
		DevCon.Warning("%s() called outside of config mode", __FUNCTION__);
		return;
	}

	const u8 responseBytesLSB = g_Sio2.GetFifoIn().front();
	g_Sio2.GetFifoIn().pop();
	const u8 responseBytes2nd = g_Sio2.GetFifoIn().front();
	g_Sio2.GetFifoIn().pop();
	const u8 responseBytesMSB = g_Sio2.GetFifoIn().front();
	g_Sio2.GetFifoIn().pop();

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

	g_Sio2.GetFifoOut().push(0x00);
	g_Sio2.GetFifoOut().push(0x00);
	g_Sio2.GetFifoOut().push(0x00);
	g_Sio2.GetFifoOut().push(0x00);
	g_Sio2.GetFifoOut().push(0x00);
	g_Sio2.GetFifoOut().push(0x5a);
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

void PadPS2Protocol::SendToPad()
{
	const u8 deviceTypeByte = g_Sio2.GetFifoIn().front();
	assert(static_cast<Sio2Mode>(deviceTypeByte) == Sio2Mode::PAD, "PadPS2Protocol was initiated, but this SIO2 command is targeting another device!");
	g_Sio2.GetFifoIn().pop();
	g_Sio2.GetFifoOut().push(0x00);

	const u8 commandByte = g_Sio2.GetFifoIn().front();
	g_Sio2.GetFifoIn().pop();
	g_Sio2.GetFifoOut().push(static_cast<u8>(activePad->IsInConfigMode() ? PadPS2Type::CONFIG : activePad->GetPadType()));
	g_Sio2.GetFifoIn().pop();
	g_Sio2.GetFifoOut().push(0x5a);

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
			Config();
			break;
		case PadPS2Mode::MODE_SWITCH:
			ModeSwitch();
			break;
		case PadPS2Mode::STATUS_INFO:
			StatusInfo();
			break;
		case PadPS2Mode::CONST_1:
			Constant1();
			break;
		case PadPS2Mode::CONST_2:
			Constant2();
			break;
		case PadPS2Mode::CONST_3:
			Constant3();
			break;
		case PadPS2Mode::VIBRATION_MAP:
			VibrationMap();
			break;
		case PadPS2Mode::RESPONSE_BYTES:
			ResponseBytes();
			break;
		default:
			DevCon.Warning("%s(queue) Unhandled PadPS2Mode (%02X)", __FUNCTION__, commandByte);
			break;
	}
}

