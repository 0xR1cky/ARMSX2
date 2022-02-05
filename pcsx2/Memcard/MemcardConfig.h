
#pragma once


#include "SioTypes.h"
#include "ghc/filesystem.h"

class MemcardConfig
{
private:
	ghc::filesystem::path memcardsFolder = "./Documents/PCSX2/memcards_v2/";
	ghc::filesystem::path fileName_port_0_slot_0 = "Memcard_1-A.ps2";
	ghc::filesystem::path fileName_port_0_slot_1 = "Memcard_1-B.ps2";
	ghc::filesystem::path fileName_port_0_slot_2 = "Memcard_1-C.ps2";
	ghc::filesystem::path fileName_port_0_slot_3 = "Memcard_1-D.ps2";
	ghc::filesystem::path fileName_port_1_slot_0 = "Memcard_2-A.ps2";
	ghc::filesystem::path fileName_port_1_slot_1 = "Memcard_2-B.ps2";
	ghc::filesystem::path fileName_port_1_slot_2 = "Memcard_2-C.ps2";
	ghc::filesystem::path fileName_port_1_slot_3 = "Memcard_2-D.ps2";

public:
	MemcardConfig();
	~MemcardConfig();

	ghc::filesystem::path GetMemcardsFolder();
	ghc::filesystem::path GetMemcardName(size_t port, size_t slot);

	void SetMemcardsFolder(ghc::filesystem::path newPath);
};

extern MemcardConfig g_MemcardConfig;
