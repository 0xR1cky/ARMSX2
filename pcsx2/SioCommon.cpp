
#include "PrecompiledHeader.h"
#include "SioCommon.h"

#include "Sio0.h"
#include "Sio2.h"

SioCommon g_SioCommon;

SioCommon::SioCommon()
{
	for (size_t port = 0; port < MAX_PORTS; port++)
	{
		for (size_t slot = 0; slot < MAX_SLOTS; slot++)
		{
			memcards.at(port).at(slot) = std::make_unique<Memcard>(port, slot);
		}
	}
}

SioCommon::~SioCommon() = default;

void SioCommon::SoftReset()
{

}

void SioCommon::FullReset()
{
	SoftReset();
	
	g_Sio0.FullReset();
	g_Sio2.FullReset();
}

Memcard* SioCommon::GetMemcard(size_t port, size_t slot)
{
	port = std::clamp<size_t>(port, 0, MAX_PORTS);
	slot = std::clamp<size_t>(slot, 0, MAX_SLOTS);
	return memcards.at(port).at(slot).get();
}
