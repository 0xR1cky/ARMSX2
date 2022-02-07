
#include "PrecompiledHeader.h"
#include "MemcardFileIO.h"

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
	std::ofstream writer;
	writer.open(memcard->GetFullPath());

	if (writer.good())
	{
		std::vector<u8>& memcardDataRef = memcard->GetMemcardDataRef();
		const char* buf = reinterpret_cast<char*>(memcardDataRef.data());
		writer.write(buf, memcardDataRef.size());
	}
	else
	{
		Console.Warning("%s() Failed to initialize memcard file (port %d slot %d) on file system!", __FUNCTION__, memcard->GetPort(), memcard->GetSlot());
	}

	writer.close();
}

void MemcardFileIO::Load(Memcard* memcard)
{
	memcard->GetStreamRef().open(memcard->GetFullPath(), std::ios_base::in | std::ios_base::out | std::ios_base::binary);

	if (!memcard->GetStreamRef().good())
	{
		Console.Warning("%s() Failed to open memcard file (port %d slot %d), ejecting it!", __FUNCTION__, memcard->GetPort(), memcard->GetSlot());
		memcard->SetMemcardType(MemcardType::EJECTED);
		return;
	}

	memcard->GetMemcardDataRef().clear();
	std::vector<char> buf;
	buf.resize(STREAM_BATCH_SIZE);

	while (memcard->GetStreamRef().good())
	{
		memcard->GetStreamRef().read(buf.data(), STREAM_BATCH_SIZE);

		if (!memcard->GetStreamRef().eof())
		{
			for (size_t pos = 0; pos < buf.size(); pos++)
			{
				memcard->GetMemcardDataRef().push_back(buf.at(pos));
			}
		}
	}

	memcard->GetStreamRef().flush();
	memcard->GetStreamRef().clear();
	memcard->GetStreamRef().seekg(0, memcard->GetStreamRef().beg);

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
}
