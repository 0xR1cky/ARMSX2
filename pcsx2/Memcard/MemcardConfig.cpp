
#include "PrecompiledHeader.h"
#include "MemcardConfig.h"

MemcardConfig g_MemcardConfig;

MemcardConfig::MemcardConfig() = default;
MemcardConfig::~MemcardConfig() = default;

std::string MemcardConfig::GetMemcardsFolder()
{
	return memcardsFolder;
}

void MemcardConfig::SetMemcardsFolder(const std::string& newPath)
{
	memcardsFolder = newPath;
}

std::string MemcardConfig::GetMemcardName(size_t port, size_t slot)
{
	port = std::clamp<size_t>(port, 0, MAX_PORTS);
	slot = std::clamp<size_t>(slot, 0, MAX_SLOTS);

	switch (port)
	{
		case 0:
			switch (slot)
			{
				case 0:
					return fileName_port_0_slot_0;
				case 1:
					return fileName_port_0_slot_1;
				case 2:
					return fileName_port_0_slot_2;
				case 3:
					return fileName_port_0_slot_3;
				default:
					DevCon.Warning("%s(%d, %d) Sanity check! Please report to PCSX2 team!", __FUNCTION__, port, slot);
					return "";
			}

			DevCon.Warning("%s(%d, %d) Sanity check! Please report to PCSX2 team!", __FUNCTION__, port, slot);
			return "";
		case 1:
			switch (slot)
			{
				case 0:
					return fileName_port_1_slot_0;
				case 1:
					return fileName_port_1_slot_1;
				case 2:
					return fileName_port_1_slot_2;
				case 3:
					return fileName_port_1_slot_3;
				default:
					DevCon.Warning("%s(%d, %d) Sanity check! Please report to PCSX2 team!", __FUNCTION__, port, slot);
					return "";
			}

			DevCon.Warning("%s(%d, %d) Sanity check! Please report to PCSX2 team!", __FUNCTION__, port, slot);
			return "";
		default:
			DevCon.Warning("%s(%d, %d) Sanity check! Please report to PCSX2 team!", __FUNCTION__, port, slot);
			return "";
	}
}
