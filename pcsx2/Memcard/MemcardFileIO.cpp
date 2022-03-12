
#include "PrecompiledHeader.h"
#include "MemcardFileIO.h"

#include "common/FileSystem.h"

MemcardFileIO g_MemcardFileIO;

bool MemcardFileIO::IsPS2Size(size_t size)
{
	if (size % BASE_8MB_SIZE != 0)
	{
		return false;
	}

	for (size_t powerSize = BASE_8MB_SIZE; powerSize <= MAX_2GB_SIZE; powerSize * 2)
	{
		if (size == powerSize)
		{
			return true;
		}
	}

	return false;
}

bool MemcardFileIO::IsPS1Size(size_t size)
{
	return size == BASE_PS1_SIZE;
}

MemcardFileIO::MemcardFileIO() = default;
MemcardFileIO::~MemcardFileIO() = default;

void MemcardFileIO::Initialize(Memcard* memcard)
{
	std::vector<u8> emptyMemcard = std::vector<u8>(BASE_8MB_SIZE, 0xff);
	FileSystem::WriteBinaryFile(memcard->GetFullPath().c_str(), emptyMemcard.data(), emptyMemcard.size());
}

void MemcardFileIO::Load(Memcard* memcard)
{
	std::optional<std::vector<u8>> dataOpt = FileSystem::ReadBinaryFile(memcard->GetFullPath().c_str());

	if (dataOpt.has_value())
	{
		memcard->GetMemcardDataRef() = dataOpt.value();
	}
	else
	{
		Console.Warning("%s(memcard) Failed to read memcard! (Port = %d, Slot = %d)", __FUNCTION__, memcard->GetPort(), memcard->GetSlot());
		memcard->SetMemcardType(MemcardType::EJECTED);
		return;
	}

	// Update sector count to reflect size of the card
	if (IsPS2Size(memcard->GetMemcardDataRef().size()))
	{
		const size_t sectorSizeWithECC = (static_cast<u16>(memcard->GetSectorSize()) + ECC_BYTES);
		memcard->SetSectorCount(static_cast<SectorCount>(memcard->GetMemcardDataRef().size() / sectorSizeWithECC));
		memcard->SetMemcardType(MemcardType::PS2);
	}
	else if (IsPS1Size(memcard->GetMemcardDataRef().size()))
	{
		memcard->SetSectorCount(SectorCount::PS1);
		memcard->SetMemcardType(MemcardType::PS1);
	}
	else
	{
		Console.Warning("%s() Memcard file (port %d slot %d) size does not match any known formats!", __FUNCTION__, memcard->GetPort(), memcard->GetSlot());
		memcard->SetMemcardType(MemcardType::EJECTED);
		return;
	}

	DevCon.WriteLn("%s() SectorCount updated: %08X", __FUNCTION__, memcard->GetSectorCount());

	// Finally, open a stream to the memcard file; this write-locks it and prevents file sync services
	// (OneDrive, Dropbox, etc) from causing any concurrency issues, as well as allows us rapid access
	// for in-place writes.
	memcard->GetStreamRef().open(memcard->GetFullPath(), MEMCARD_OPEN_MODE);

	if (!memcard->GetStreamRef().good())
	{
		Console.Warning("%s(memcard) Failed to open stream on memcard! Ejecting it! (Port = %d, Slot = %d)", __FUNCTION__, memcard->GetPort(), memcard->GetSlot());
		memcard->SetMemcardType(MemcardType::EJECTED);
		return;
	}
}

void MemcardFileIO::Write(Memcard* memcard, u32 address, size_t length)
{
	if (!memcard->GetStreamRef().good())
	{
		Console.Warning("%s(memcard, %08x, %d) Failed to open memcard file! (Port = %d, Slot = %d)", __FUNCTION__, address, length, memcard->GetPort(), memcard->GetSlot());
		Console.Warning("This sector write will persist in memory, but will not be committed to disk!");
		// TODO: Should we eject the card? What's the proper thing to do here...
		return;
	}

	const char* data = reinterpret_cast<char*>(memcard->GetMemcardDataRef().data() + address);
	memcard->GetStreamRef().seekp(address);
	memcard->GetStreamRef().write(data, length);
	memcard->GetStreamRef().flush();
}
