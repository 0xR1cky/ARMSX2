
#include "PrecompiledHeader.h"
#include "MemcardFolderIO.h"

#include <array>

MemcardFolderIO g_MemcardFolderIO;

MemcardFolderIO::MemcardFolderIO() = default;
MemcardFolderIO::~MemcardFolderIO() = default;

void MemcardFolderIO::Initialize(Memcard* memcard)
{
	if (!ghc::filesystem::create_directories(memcard->GetFullPath()))
	{
		Console.Warning("%s() Failed to create root of folder memcard (port %d slot %d) on file system!", __FUNCTION__, memcard->GetPort(), memcard->GetSlot());
		return;
	}

	std::ofstream writer;
	writer.open(memcard->GetFullPath() / FOLDER_MEMCARD_SUPERBLOCK_NAME);

	if (writer.good())
	{
		const std::array<char, FOLDER_MEMCARD_SUPERBLOCK_SIZE> buf{0};
		writer.write(buf.data(), buf.size());
	}
	else
	{
		Console.Warning("%s() Failed to generate empty blob for memcard folder's superblock (port %d slot %d) on file system!", __FUNCTION__, port, slot);
	}

	writer.close();
}

void MemcardFolderIO::Load(Memcard* memcard)
{
	// TODO: Construct a 8 MB card.
	// Copy the superblock into the front.
	// Build an IFAT and FAT
	// Span data across the writeable sectors
	memcard->GetStreamRef().open(memcard->GetFullPath() / FOLDER_MEMCARD_SUPERBLOCK_NAME, std::ios_base::in | std::ios_base::out | std::ios_base::binary);

	if (!memcard->GetStreamRef().good())
	{
		Console.Warning("%s() Failed to open memcard file (port %d slot %d), ejecting it!", __FUNCTION__, memcard->GetPort(), memcard->GetSlot());
		memcard->SetMemcardType(MemcardType::EJECTED);
		return;
	}

	// First load the superblock
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
}

void MemcardFolderIO::Write(Memcard* memcard, u32 address, size_t location)
{

}
