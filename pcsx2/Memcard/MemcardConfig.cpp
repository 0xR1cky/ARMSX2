
#include "PrecompiledHeader.h"
#include "MemcardConfig.h"

MemcardConfig g_MemcardConfig;

MemcardConfig::MemcardConfig()
{
	for (size_t port = 0; port < MAX_PORTS; port++)
	{
		for (size_t slot = 0; slot < MAX_SLOTS; slot++)
		{
			slots.at(port).at(slot) = std::make_unique<MemcardConfigSlot>(port, slot);
		}
	}
}

MemcardConfig::~MemcardConfig() = default;

MemcardConfigSlot* MemcardConfig::GetMemcardConfigSlot(int port, int slot)
{
	port = std::clamp<size_t>(port, 0, MAX_PORTS);
	slot = std::clamp<size_t>(slot, 0, MAX_SLOTS);
	return slots.at(port).at(slot).get();
}

ghc::filesystem::path MemcardConfig::GetMemcardsFolder()
{
	return memcardsFolder;
}

void MemcardConfig::SetMemcardsFolder(ghc::filesystem::path newPath)
{
	memcardsFolder = newPath;
}
