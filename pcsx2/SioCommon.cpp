
#include "PrecompiledHeader.h"
#include "SioCommon.h"

#include "Sio0.h"
#include "Sio2.h"
#include "Memcard/MemcardConfig.h"

SioCommon g_SioCommon;

SioCommon::SioCommon()
{
	for (size_t port = 0; port < MAX_PORTS; port++)
	{
		for (size_t slot = 0; slot < MAX_SLOTS; slot++)
		{
			memcards.at(port).at(slot) = std::make_unique<Memcard>(port, slot);
			/*
			MemcardConfigSlot* mcs = g_MemcardConfig.GetMemcardConfigSlot(port, slot);

			if (mcs != nullptr)
			{
				
				Memcard* memcard = memcards.at(port).at(slot).get();
				memcard->SetMemcardType(mcs->GetMemcardType());
			}
			*/
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
	
	for (size_t port = 0; port < MAX_PORTS; port++)
	{
		for (size_t slot = 0; slot < MAX_SLOTS; slot++)
		{
			MemcardConfigSlot* mcs = g_MemcardConfig.GetMemcardConfigSlot(port, slot);

			if (mcs != nullptr)
			{	
				Memcard* memcard = memcards.at(port).at(slot).get();
				memcard->SetMemcardType(mcs->GetMemcardType());
			}
		}
	}

	g_sio0.Reset();
	g_Sio2.Reset();
}

MemcardType SioCommon::GetMemcardType(size_t port, size_t slot)
{
	port = std::clamp<size_t>(port, 0, MAX_PORTS);
	slot = std::clamp<size_t>(slot, 0, MAX_SLOTS);
	Memcard* memcard = memcards.at(port).at(slot).get();
	return memcard->GetMemcardType();
}

MemcardPS1* SioCommon::GetMemcardPS1(size_t port, size_t slot)
{
	port = std::clamp<size_t>(port, 0, MAX_PORTS);
	slot = std::clamp<size_t>(slot, 0, MAX_SLOTS);
	Memcard* memcard = memcards.at(port).at(slot).get();
	return memcard->GetMemcardPS1();
}

MemcardPS2* SioCommon::GetMemcardPS2(size_t port, size_t slot)
{
	port = std::clamp<size_t>(port, 0, MAX_PORTS);
	slot = std::clamp<size_t>(slot, 0, MAX_SLOTS);
	Memcard* memcard = memcards.at(port).at(slot).get();
	return memcard->GetMemcardPS2();
}
