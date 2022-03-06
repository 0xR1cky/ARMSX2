
#pragma once

#include "SioTypes.h"
#include "ghc/filesystem.h"

#include <string>

class MemcardConfig
{
private:
	std::string memcardsFolder = "/Documents/PCSX2/memcards_v2/";
	std::string fileName_port_0_slot_0 = "Memcard_1-A.ps2";
	std::string fileName_port_0_slot_1 = "Memcard_1-B.ps2";
	std::string fileName_port_0_slot_2 = "Memcard_1-C.ps2";
	std::string fileName_port_0_slot_3 = "Memcard_1-D.ps2";
	std::string fileName_port_1_slot_0 = "Memcard_2-A.ps2";
	std::string fileName_port_1_slot_1 = "Memcard_2-B.ps2";
	std::string fileName_port_1_slot_2 = "Memcard_2-C.ps2";
	std::string fileName_port_1_slot_3 = "Memcard_2-D.ps2";

public:
	MemcardConfig();
	~MemcardConfig();

	std::string GetMemcardsFolder();
	std::string GetMemcardName(size_t port, size_t slot);

	void SetMemcardsFolder(const std::string& newPath);
};

extern MemcardConfig g_MemcardConfig;
