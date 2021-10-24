
#include "PrecompiledHeader.h"
#include "PadPS1Protocol.h"

PadPS1Protocol g_padPS1Protocol;

size_t PadPS1Protocol::GetResponseSize(PadPS1ControllerType controllerType)
{
	const u8 type = static_cast<u8>(controllerType);
	return type != 0 ? (type & 0x0f) : 16;
}

u8 PadPS1Protocol::CommandPoll(u8 data)
{
	u8 ret = 0xff;

	if (currentCommandByte == 2)
	{
		// PS1 pads can engage multitaps either by incrementing the initial PAD command (0x01)
		// to the multitap slot (0x02, etc) for a single slot access, or sending 0x01 on this byte
		// to make the next pad read send a burst of all four slots at once.
		if (data == 0x01)
		{
			activePad->SetMultitapBurstQueued(true);
		}

		ret = 0x5a;
	}
	else
	{
		// If the previous pad poll queued a multitap burst
		if (activePad->IsMultitapBurstQueued())
		{
			for (size_t i = 0; i < MAX_SLOTS; i++)
			{
				const size_t port = GetActivePort();
				activePad = GetPad(port, i);

				// -3 to remove header bytes. Divide result by i to get offset to each slot.
				switch ((currentCommandByte - 3) / i)
				{
				case 0:
					ret = static_cast<u8>(activePad->GetControllerType());
					break;
				case 1:
					ret = 0x5a;
					break;
				case 2:
					ret = activePad->GetControls().digitalButtons1;
					break;
				case 3:
					ret = activePad->GetControls().digitalButtons2;
					break;
				case 4:
					ret = activePad->GetControls().rightAnalogX;
					break;
				case 5:
					ret = activePad->GetControls().rightAnalogY;
					break;
				case 6:
					ret = activePad->GetControls().leftAnalogX;
					break;
				case 7:
					ret = activePad->GetControls().leftAnalogY;
					break;
				default:
					DevCon.Warning("%s(%02X) Unexpected byte on multitap (%02X)", __FUNCTION__, data, (currentCommandByte - 3) / i);
					break;
				}
			}
		}
		// Else, just send the active pad like usual
		else
		{
			switch (currentCommandByte)
			{
			case 3:
				ret = activePad->GetControls().digitalButtons1;
				break;
			case 4:
				ret = activePad->GetControls().digitalButtons2;
				break;
			case 5:
				ret = activePad->GetControls().rightAnalogX;
				break;
			case 6:
				ret = activePad->GetControls().rightAnalogY;
				break;
			case 7:
				ret = activePad->GetControls().leftAnalogX;
				break;
			case 8:
				ret = activePad->GetControls().leftAnalogY;
				break;
			default:
				break;
			}
		}
	}

	// Reset after the command is finished. Lower nibble of the controller's type value
	// is the number of half-words in the reply, indicating command length. -3 to remove
	// header bytes.
	if (GetResponseSize(activePad->GetControllerType()) == (currentCommandByte - 3))
	{
		Reset();
	}

	return ret;
}

PadPS1Protocol::PadPS1Protocol()
{
	for (size_t i = 0; i < MAX_PORTS; i++)
	{
		for (size_t j = 0; j < MAX_SLOTS; j++)
		{
			pads.at(i).at(j) = std::make_unique<PadPS1>();
		}
	}
}

PadPS1Protocol::~PadPS1Protocol() = default;

void PadPS1Protocol::Reset()
{
	mode = PadPS1Mode::NOT_SET;
	currentCommandByte = 1;
}

PadPS1* PadPS1Protocol::GetPad(size_t port, size_t slot)
{
	port = std::clamp<size_t>(port, 0, MAX_PORTS);
	slot = std::clamp<size_t>(slot, 0, MAX_SLOTS);
	return pads.at(port).at(slot).get();
}

void PadPS1Protocol::SetActivePad(PadPS1* padPS1)
{
	activePad = padPS1;
}

PadPS1Mode PadPS1Protocol::GetPadMode()
{
	return mode;
}

size_t PadPS1Protocol::GetActivePort()
{
	return activePort;
}

void PadPS1Protocol::SetActivePort(size_t port)
{
	activePort = port;
}

void PadPS1Protocol::SetVibration(PadPS1MotorType motorType, u8 strength)
{

}

u8 PadPS1Protocol::SendToPad(u8 data)
{
	u8 ret = 0xff;

	switch (mode)
	{
	case PadPS1Mode::NOT_SET:
		mode = static_cast<PadPS1Mode>(data);

		// If the previous command queued a multitap burst, we need to identify
		// as a multitap on this command.
		if (activePad->IsMultitapBurstQueued())
		{
			ret = static_cast<u8>(PadPS1ControllerType::MULTITAP);
		}
		// Else, send our normal identity
		else
		{
			ret = static_cast<u8>(activePad->GetControllerType());
		}
		
		break;
	case PadPS1Mode::POLL:
		ret = CommandPoll(data);
		break;
		/*
	case PadPS1Mode::CONFIG:
		ret = CommandConfig(data);
		break;
	case PadPS1Mode::SET_LED:
		ret = CommandSetLED(data);
		break;
	case PadPS1Mode::GET_LED:
		ret = CommandGetLED(data);
		break;
	case PadPS1Mode::STATUS_1:
		ret = CommandStatus1(data);
		break;
	case PadPS1Mode::STATUS_2:
		ret = CommandStatus2(data);
		break;
	case PadPS1Mode::STATUS_3:
		ret = CommandStatus3(data);
		break;
	case PadPS1Mode::RUMBLE:
		ret = CommandRumble(data);
		break;
	case PadPS1Mode::MYSTERY:
		ret = CommandMystery(data);
		break;
		*/
	default:
		DevCon.Warning("%s(%02X) - Unexpected first command byte", __FUNCTION__, data);
		ret = 0xff;
		Reset();
		break;
	}

	currentCommandByte++;
	return ret;
}
