
#include "PrecompiledHeader.h"
#include "PadPS2Protocol.h"

PadPS2Protocol g_padPS2Protocol;

// Reset mode and byte counters to not set and 0,
// to prepare for the next command. Calling this
// function prematurely, or failing to call it 
// prior to returning the final byte will have
// adverse effects.
void PadPS2Protocol::Reset()
{
	mode = PadPS2Mode::NOT_SET;
	currentCommandByte = 1;
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

u8 PadPS2Protocol::Mystery(u8 data)
{
	if (!activePad->IsInConfigMode())
	{
		DevCon.Warning("%s(%02X) called outside of config mode", __FUNCTION__, data);
		return 0xff;
	}

	switch (currentCommandByte)
	{
	case 5:
		return 0x02;
	case 8:
		return 0x5a;
	default:
		return 0x00;
	}
}

u8 PadPS2Protocol::ButtonQuery(u8 data)
{
	if (!activePad->IsInConfigMode())
	{
		DevCon.Warning("%s(%02X) called outside of config mode", __FUNCTION__, data);
		return 0xff;
	}

	switch (currentCommandByte)
	{
	case 3:
	case 4:
		return 0xff;
	case 5:
		return 0x03;
	case 8:
		return 0x5a;
	default:
		return 0x00;
	}
}

u8 PadPS2Protocol::Poll(u8 data)
{
	u8 ret = 0xff;

	switch (currentCommandByte)
	{
	case 3:
		activePad->Debug_Poll();
		ret = activePad->GetDigitalByte1();
		break;
	case 4:
		ret = activePad->GetDigitalByte2();
		break;
	case 5:
		ret = activePad->GetAnalogRightX();
		break;
	case 6:
		ret = activePad->GetAnalogRightY();
		break;
	case 7:
		ret = activePad->GetAnalogLeftX();
		break;
	case 8:
		ret = activePad->GetAnalogLeftY();
		break;
	default:
		ret = 0x00;
		break;
	}

	return ret;
}

u8 PadPS2Protocol::Config(u8 data)
{
	// Returns are a bit weird for this one. According to documentation, the initial "enter config pls"
	// command will get a response identical to a normal poll. Makes enough sense. Then after config
	// mode is entered, further config mode commands will not return button states. Also makes sense as
	// the only other config commands should be to exit, or illegal. What doesn't make sense is why
	// returning button states should matter at all, because a game will not be entering config mode
	// recreationally and should only ever do so at startup or after device plug/unplug events, for all
	// of which, WHO CARES IF THIS ONE POLL NEVER HAPPENS! So, we're just going to arbitrarily return
	// inactive pad states. Go team.
	u8 ret = 0xff;

	switch (currentCommandByte)
	{
	case 3:
		if (data == 0x00)
		{
			if (activePad->IsInConfigMode())
			{
				activePad->SetInConfigMode(false);
				activePad->SetConfigResponse(true);
			}
			else
			{
				DevCon.Warning("%s(%02X) Unexpected exit while not in config mode", __FUNCTION__, data);
			}
		}
		else if (data == 0x01)
		{
			if (!activePad->IsInConfigMode())
			{
				activePad->SetInConfigMode(true);
			}
			else
			{
				DevCon.Warning("%s(%02X) Unexpected enter while already in config mode", __FUNCTION__, data);
			}
		}
		else
		{
			DevCon.Warning("%s(%02X) Unexpected enter/exit byte (%d > 1)", __FUNCTION__, data, data);
		}
	default:
		ret = 0x00;
		break;
	}

	return ret;
}

u8 PadPS2Protocol::ModeSwitch(u8 data)
{
	if (!activePad->IsInConfigMode())
	{
		DevCon.Warning("%s(%02X) called outside of config mode", __FUNCTION__, data);
		return 0xff;
	}

	switch (currentCommandByte)
	{
	case 3:
		if (data == 0x01)
		{
			activePad->SetAnalogLight(true);
			activePad->SetPadType(PadPS2Type::ANALOG);
		}
		else if (data == 0x00)
		{
			activePad->SetAnalogLight(false);
			activePad->SetPadType(PadPS2Type::DIGITAL);
		}
		else
		{
			DevCon.Warning("%s(%02X) Unexpected 4th byte (%d > 1)", __FUNCTION__, data, data);
		}
		break;
	case 4:
		activePad->SetAnalogLocked(data == 0x03);
		break;
	case 8:
		return 0x00;
	default:
		return 0x00;
	}
}

u8 PadPS2Protocol::StatusInfo(u8 data)
{
	if (!activePad->IsInConfigMode())
	{
		DevCon.Warning("%s(%02X) called outside of config mode", __FUNCTION__, data);
		return 0xff;
	}

	switch (currentCommandByte)
	{
	case 3:
		return static_cast<u8>(activePad->GetPadPhysicalType());
	case 4:
		return 0x02; // magic!
	case 5:
		return activePad->IsAnalogLightOn();
	case 6:
		return 0x02;
	case 7:
		return 0x01;
	case 8:
		return 0x00;
	}
}

u8 PadPS2Protocol::Constant1(u8 data)
{
	if (!activePad->IsInConfigMode())
	{
		DevCon.Warning("%s(%02X) called outside of config mode", __FUNCTION__, data);
		return 0xff;
	}

	switch (currentCommandByte)
	{
	case 3:
		activePad->SetConstantStage(data);
	case 4:
		return 0x00;
	case 5:
		if (activePad->GetPadPhysicalType() == PadPS2Physical::STANDARD)
		{
			return 0x00;
		}
		else
		{
			return 0x01;
		}
	case 6:
		if (!activePad->GetConstantStage())
		{
			return 0x02;
		}
		else
		{
			if (activePad->GetPadPhysicalType() == PadPS2Physical::STANDARD)
			{
				return 0x00;
			}
			else
			{
				return 0x01;
			}
		}
	case 7:
		if (!activePad->GetConstantStage())
		{
			return 0x00;
		}
		else
		{
			if (activePad->GetPadPhysicalType() == PadPS2Physical::STANDARD)
			{
				return 0x00;
			}
			else
			{
				return 0x01;
			}
		}
	case 8:
		if (!activePad->GetConstantStage())
		{
			return 0x0A;
		}
		else
		{
			return 0x14;
		}
	default:
		DevCon.Warning("%s(%02X) Overran expected length (%d > 9)", __FUNCTION__, data, currentCommandByte);
		return 0x00;
	}
}

u8 PadPS2Protocol::Constant2(u8 data)
{
	if (!activePad->IsInConfigMode())
	{
		DevCon.Warning("%s(%02X) called outside of config mode", __FUNCTION__, data);
		return 0xff;
	}

	switch (currentCommandByte)
	{
	case 3:
	case 4:
		return 0x00;
	case 5:
		return 0x02;
	case 6:
		return 0x00;
	case 7:
		if (activePad->GetPadPhysicalType() == PadPS2Physical::STANDARD)
		{
			return 0x00;
		}
		else if (activePad->GetPadPhysicalType() == PadPS2Physical::GUITAR)
		{
			return 0x01;
		}
		else
		{
			DevCon.Warning("%s(%02X) Unrecognized physical type (%02X)", __FUNCTION__, data, activePad->GetPadPhysicalType());
			return 0x00;
		}
	case 8:
		return 0x00;
	default:
		DevCon.Warning("%s(%02X) Overran expected length (%d > 9)", __FUNCTION__, data, currentCommandByte);
		return 0x00;
	}
}

u8 PadPS2Protocol::Constant3(u8 data)
{
	if (!activePad->IsInConfigMode())
	{
		DevCon.Warning("%s(%02X) called outside of config mode", __FUNCTION__, data);
		return 0xff;
	}

	switch (currentCommandByte)
	{
	case 3:
		activePad->SetConstantStage(data);
	case 4:
	case 5:
		return 0x00;
	case 6:
		// Since documentation doesn't bother explaining this one...
		// (thanks padtest_ps2.elf for actually sheding some light on this!)
		// This byte, on each run of the command, specifies one of the controller's operating modes.
		// So far we know that (of the ones that actually matter) 0x04 = digital, 0x07 = analog.
		// This seems to correspond with assertions which are made about the "pad modes" being
		// 0x41 = digital, 0x73 = analog, 0x79 = dualshock 2. It does leave the question of if these
		// "constant" commands should also have a value somewhere for the second nibble of the pad mode...
		if (!activePad->GetConstantStage())
		{
			return 0x04;
		}
		else
		{
			return 0x07;
		}
	case 7:
		return 0x00;
	case 8:
		return 0x00;
	default:
		DevCon.Warning("%s(%02X) Overran expected length (%d > 9)", __FUNCTION__, data, currentCommandByte);
		return 0x00;
	}
}

u8 PadPS2Protocol::VibrationMap(u8 data)
{
	switch (currentCommandByte)
	{
	case 3:
		return 0x00;
	case 4:
		return 0x01;
	default:
		return 0xff;
	}
}

u8 PadPS2Protocol::ResponseBytes(u8 data)
{
	switch (currentCommandByte)
	{
	case 3:
		if (data == 0x03)
		{
			activePad->SetAnalogLight(false);
			activePad->SetPadType(PadPS2Type::DIGITAL);
		}
		else if (data == 0x3f)
		{
			activePad->SetAnalogLight(true);
			activePad->SetPadType(PadPS2Type::ANALOG);
		}
		break;
	case 4:
		break;
	case 5:
		if (data == 0x03)
		{
			activePad->SetAnalogLight(true);
			activePad->SetPadType(PadPS2Type::DUALSHOCK2);
		}
		break;
	case 8:
		return 0x5a;
	default:
		break;
	}

	return 0x00;
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

u8 PadPS2Protocol::SendToPad(u8 data)
{
	u8 ret = 0xff;

	if (currentCommandByte == 1)
	{
		mode = static_cast<PadPS2Mode>(data);
		ret = static_cast<u8>(activePad->IsInConfigMode() ? PadPS2Type::CONFIG : activePad->GetPadType());
	}
	else if (currentCommandByte == 2)
	{
		ret = 0x5a;
	}
	else
	{
		switch (mode)
		{
		case PadPS2Mode::MYSTERY:
			ret = Mystery(data);
			break;
		case PadPS2Mode::BUTTON_QUERY:
			ret = ButtonQuery(data);
			break;
		case PadPS2Mode::POLL:
			ret = Poll(data);
			break;
		case PadPS2Mode::CONFIG:
			ret = Config(data);
			break;
		case PadPS2Mode::MODE_SWITCH:
			ret = ModeSwitch(data);
			break;
		case PadPS2Mode::STATUS_INFO:
			ret = StatusInfo(data);
			break;
		case PadPS2Mode::CONST_1:
			ret = Constant1(data);
			break;
		case PadPS2Mode::CONST_2:
			ret = Constant2(data);
			break;
		case PadPS2Mode::CONST_3:
			ret = Constant3(data);
			break;
		case PadPS2Mode::VIBRATION_MAP:
			ret = VibrationMap(data);
			break;
		case PadPS2Mode::RESPONSE_BYTES:
			ret = ResponseBytes(data);
			break;
		default:
			DevCon.Warning("%s(%02X) Unhandled PadPS2Mode (%02X) (currentCommandByte = %d)", __FUNCTION__, data, static_cast<u8>(mode), currentCommandByte);
			break;
		}
	}

	currentCommandByte++;
	return ret;
}

