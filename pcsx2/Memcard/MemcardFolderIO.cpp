
#include "PrecompiledHeader.h"
#include "MemcardFolderIO.h"

#include "common/FileSystem.h"

#include <array>
#include <sstream>
#include <optional>
#include <ctime>

MemcardFolderIO g_MemcardFolderIO;

std::array<u8, 8> MemcardFolderIO::UnixTimeToPS2(const u64 &unixTime)
{
	const time_t time = static_cast<time_t>(unixTime);
	const tm* timeStruct = gmtime(&time);
	std::array<u8, 8> ps2Time{0};
	ps2Time.at(1) = timeStruct->tm_sec;
	ps2Time.at(2) = timeStruct->tm_min;
	ps2Time.at(3) = timeStruct->tm_hour;
	ps2Time.at(4) = timeStruct->tm_mday;
	ps2Time.at(5) = timeStruct->tm_mon + 1;
	ps2Time.at(6) = (timeStruct->tm_year + 1900) & 0xff;
	ps2Time.at(7) = (timeStruct->tm_year + 1900) >> 8;
	return ps2Time;
}

u64 MemcardFolderIO::PS2TimeToUnix(const std::array<u8, 8> &ps2Time)
{
	tm* timeStruct;
	timeStruct->tm_sec = ps2Time.at(1);
	timeStruct->tm_min = ps2Time.at(2);
	timeStruct->tm_hour = ps2Time.at(3);
	timeStruct->tm_mday = ps2Time.at(4);
	timeStruct->tm_mon = ps2Time.at(5) - 1;
	timeStruct->tm_year = (ps2Time.at(6) & (ps2Time.at(7) << 8)) - 1900;
	const time_t time = mktime(timeStruct);
	const u64 unixTime = static_cast<u64>(time);
	return unixTime;
}

ryml::Tree MemcardFolderIO::TreeFromString(const std::string &s)
{
	return ryml::parse(c4::to_csubstr(s));
}

std::optional<ryml::Tree> MemcardFolderIO::ReadYamlFromFile(const char* yamlFileName)
{
	if (!FileSystem::FileExists(yamlFileName))
	{
		DevCon.Warning("%s(%s) File does not exist", __FUNCTION__, yamlFileName);
		return std::nullopt;
	}

	std::optional<std::string> yamlFileContents = FileSystem::ReadFileToString(yamlFileName);

	if (yamlFileContents.has_value())
	{
		return std::make_optional(TreeFromString(yamlFileContents.value()));	
	}
	else 
	{
		DevCon.Warning("%s(%s) Optional has no value; did the file read fail?", __FUNCTION__, yamlFileName);
		return std::nullopt;
	}
}

void MemcardFolderIO::WriteYamlToFile(const char* yamlFileName, const ryml::NodeRef& node)
{
	std::string yaml = ryml::emitrs<std::string>(node);
	FileSystem::WriteBinaryFile(yamlFileName, yaml.data(), yaml.length());
}

void MemcardFolderIO::RecurseDirectory(const char* directory, DirectoryEntry* currentEntry)
{
	FileSystem::FindResultsArray results;
	const bool hasChildren = FileSystem::FindFiles(directory, "*", FILESYSTEM_FIND_FOLDERS + FILESYSTEM_FIND_FILES, &results);

	if (!hasChildren)
	{
		DevCon.WriteLn("%s(%s) Empty directory", __FUNCTION__, directory);
	}

	for (FILESYSTEM_FIND_DATA result : results)
	{
		if (FileSystem::GetFileNameFromPath(result.FileName) == "_pcsx2_superblock")
		{
			continue;
		}

		DirectoryEntry* child = new DirectoryEntry();
		child->name = FileSystem::GetFileNameFromPath(result.FileName);

		if (result.Attributes & FILESYSTEM_FILE_ATTRIBUTE_DIRECTORY)
		{
			child->type = DirectoryType::DIRECTORY;
			child->flags = DEFAULT_DIRECTORY_MODE_FLAGS;
			InsertDotDirectories(child);
			currentEntry->children.push_back(child);
			RecurseDirectory(result.FileName.c_str(), child);
		}
		else if (child->name == FOLDER_MEMCARD_INDEX_NAME)
		{
			LoadIndexFile(result.FileName.c_str(), currentEntry);
		}
		else
		{
			child->type = DirectoryType::FILE;
			child->flags = DEFAULT_FILE_MODE_FLAGS;
			child->fileData = LoadFile(result.FileName.c_str());
			currentEntry->children.push_back(child);
		}
	}
}

void MemcardFolderIO::InsertDotDirectories(DirectoryEntry* dirEntry)
{
	DirectoryEntry* singleDot = new DirectoryEntry();
	singleDot->name = ".";
	singleDot->flags = SINGLE_DOT_MODE_FLAGS;
	dirEntry->children.insert(dirEntry->children.begin(), singleDot);
	DirectoryEntry* doubleDot = new DirectoryEntry();
	doubleDot->name = "..";
	doubleDot->flags = DOUBLE_DOT_MODE_FLAGS;

	// Special case: ".." entry in the root does not have read flag set,
	// and is hidden
	if (dirEntry->name == "")
	{
		doubleDot->flags &= ~(static_cast<u16>(DirectoryModeFlag::READ));
		doubleDot->flags |= static_cast<u16>(DirectoryModeFlag::HIDDEN);
	}

	dirEntry->children.insert(dirEntry->children.begin() + 1, doubleDot);
}

void MemcardFolderIO::LoadIndexFile(const char* indexFilePath, DirectoryEntry* currentEntry)
{
	std::optional<ryml::Tree> yaml = ReadYamlFromFile(indexFilePath);

	if (yaml.has_value() && !yaml.value().empty())
	{
		ryml::NodeRef root = yaml.value().rootref();
		DirectoryIndex index;
		index.directoryEntry = currentEntry;

		for (const ryml::NodeRef& node : root.children())
		{
			u64 unixTime = 0;
			DirectoryIndexEntry entry;
			std::string fileName = std::string(node.key().str, node.key().len);
			entry.name = fileName;

			if (node.has_val())
			{
				Console.Warning("Damaged index file:");
				Console.Warning("%s", indexFilePath);
				Console.Warning("Skipping this index, save data will not be corrupted, but may be inaccessible.");
				return;
			}
			else
			{
				if (node.has_child("order"))
				{
					node["order"] >> entry.order;
				}
				if (node.has_child("timeCreated"))
				{
					node["timeCreated"] >> unixTime;
					entry.created = UnixTimeToPS2(unixTime);
				}
				if (node.has_child("timeModified"))
				{
					node["timeModified"] >> unixTime;
					entry.modified = UnixTimeToPS2(unixTime);
				}
			}

			index.entries.push_back(entry);
		}

		indexes.push_back(index);
	}
}

std::vector<u8> MemcardFolderIO::LoadFile(const char* fileName)
{
	std::optional<std::vector<u8>> fileOpt = FileSystem::ReadBinaryFile(fileName);

	if (fileOpt.has_value())
	{
		return fileOpt.value();
	}
	else
	{
		DevCon.Warning("%s(%s) Empty optional, either file could not be read or is empty", __FUNCTION__, fileName);
		return std::vector<u8>();
	}
}

void MemcardFolderIO::ApplyIndexes()
{
	for (DirectoryIndex index : indexes)
	{
		DirectoryEntry* dirEntryPtr = index.directoryEntry;
		std::vector<DirectoryEntry*> newVector;

		// For each entry in the index...
		for (DirectoryIndexEntry indexEntry : index.entries)
		{
			// Find the DirectoryEntry which matches
			for (DirectoryEntry* child : dirEntryPtr->children)
			{
				if (child->name == indexEntry.name)
				{
					// Copy its created and modified times
					child->created = indexEntry.created;
					child->modified = indexEntry.modified;

					// Then copy it to our new vector
					newVector.push_back(child);
				}
			}
		}

		// Finally, replace the directory entries
		dirEntryPtr->children.swap(newVector);
		// And replace the dot directories which were left behind
		InsertDotDirectories(dirEntryPtr);
	}
}

// Reserves enough FAT entries to contain the directory data, and then writes the directory data
// to the data clusters that were reserved in the FAT.
//
// Returns the first reserved fat entry, to be used in the cluster attribute of a PS2Directory.
u32 MemcardFolderIO::CommitDirectory(Memcard* memcard, DirectoryEntry* dirEntry, u32 parentEntryPos)
{
	// Dot directories never have children, and thus no effort should be made to allocate space.
	// A dot entry's cluster attribute will always be 0.
	if (dirEntry->name == "." || dirEntry->name == "..")
	{
		return 0;
	}

	// One dir entry per page, two pages per cluster, so divide number of dir entries by 2 and use ceiling.
	// Result is number of data clusters (and number of fat entries) required to store this directory.
	const size_t clustersRequired = ceil(dirEntry->children.size() / 2.0f);
	std::vector<u32> fatEntries;

	// First, reserve fat entries
	while (fatEntries.size() < clustersRequired)
	{
		const u32 fatEntry = GetFirstFreeFatEntry(memcard);
		fatEntries.push_back(fatEntry);
		SetFatEntry(memcard, fatEntry, FAT::IN_USE_MASK);
	}

	// Now link them together
	assert(!fatEntries.empty());
	u32 fatEntry = 0;
	u32 lastEntry = fatEntries.at(0);

	for (size_t i = 1; i < fatEntries.size(); i++)
	{
		fatEntry = fatEntries.at(i);
		SetFatEntry(memcard, lastEntry, fatEntry | FAT::IN_USE_MASK);
		lastEntry = fatEntry;
	}

	SetFatEntry(memcard, fatEntry, FAT::LAST_CLUSTER);

	// Any subdirectories will need to have the "dirEntry" attribute of their "." directory entry
	// set to the position of their parent. Position is relative to the other directory entries,
	// it is not its position in the FAT.
	size_t entryCount = 0;

	// Now fill their data clusters
	size_t fatEntryCount = 0;
	size_t bytesWritten = 0;
	size_t sectorsWritten = 0;
	u32 address = GetDataClusterAddr(memcard, fatEntries.at(fatEntryCount));

	for (DirectoryEntry* entry : dirEntry->children)
	{
		PS2Directory ps2Dir;
		memcpy(ps2Dir.created, entry->created.data(), entry->created.size());
		memcpy(ps2Dir.modified, entry->modified.data(), entry->modified.size());
		memcpy(ps2Dir.name, entry->name.c_str(), entry->name.size());
		
		if (entry->type == DirectoryType::FILE)
		{
			ps2Dir.mode = entry->flags;
			ps2Dir.length = entry->fileData.size();
			ps2Dir.cluster = (ps2Dir.length > 0 ? CommitFile(memcard, entry) : EMPTY_FILE_CLUSTER_VALUE);
		}
		else if (entry->type == DirectoryType::DIRECTORY)
		{
			ps2Dir.mode = entry->flags;
			ps2Dir.length = entry->children.size();
			ps2Dir.cluster = CommitDirectory(memcard, entry, entryCount);

			if (entry->name == ".")
			{
				// If this is the "." directory for the root,
				// set length to the number of items in root,
				// and set modified to the current time
				if (dirEntry->name == "")
				{
					ps2Dir.length = dirEntry->children.size();
					memcpy(ps2Dir.modified, UnixTimeToPS2(time(nullptr)).data(), 8);
				}

				if (parentEntryPos != 0)
				{
					ps2Dir.dirEntry = parentEntryPos;
				}
			}
		}
		else
		{
			assert(false);
		}

		memcpy(memcard->GetMemcardDataRef().data() + address, &ps2Dir, sizeof(ps2Dir));

		// Offset address by the sector size, since one directory entry fills an entire sector,
		// plus 16 bytes of ECC which we did not write (this is a later step).
		address += BASE_SECTOR_SIZE_WITH_ECC;
		// Increment our sectorsWritten counter.
		sectorsWritten++;

		// When both sectors in a cluster are written
		if (sectorsWritten == static_cast<size_t>(ClusterSize::STANDARD))
		{
			// Reset the sectorsWritten counter, and find the next data cluster
			// to continue writing.
			sectorsWritten = 0;
			fatEntryCount++;

			// Prevent overflow on fatEntries; if this condition is not true,
			// then that means we are on the last DirectoryEntry and should be exiting anyways.
			if (fatEntryCount < fatEntries.size())
			{
				address = GetDataClusterAddr(memcard, fatEntries.at(fatEntryCount));
			}
		}

		entryCount++;
	}

	return fatEntries.at(0);
}

// Reserves enough FAT entries to contain the file data, and then writes the file data
// to the data clusters that were reserved in the FAT.
//
// Returns the first reserved fat entry, to be used in the cluster attribute of a PS2Directory.
u32 MemcardFolderIO::CommitFile(Memcard* memcard, DirectoryEntry* fileEntry)
{
	// Reserve FAT entries for the file
	const size_t clustersRequired = ceil(fileEntry->fileData.size() / (static_cast<float>(SectorSize::STANDARD) * static_cast<u16>(ClusterSize::STANDARD)));
	std::vector<u32> fatEntries;

	while (fatEntries.size() < clustersRequired)
	{
		const u32 fatEntry = GetFirstFreeFatEntry(memcard);
		fatEntries.push_back(fatEntry);
		SetFatEntry(memcard, fatEntry, FAT::IN_USE_MASK);
	}

	// Now link them together
	assert(!fatEntries.empty());
	u32 fatEntry = 0;
	u32 lastEntry = fatEntries.at(0);

	for (size_t i = 0; i < fatEntries.size(); i++)
	{
		fatEntry = fatEntries.at(i);
		SetFatEntry(memcard, lastEntry, fatEntry | FAT::IN_USE_MASK);
		lastEntry = fatEntry;
	}

	SetFatEntry(memcard, fatEntry, FAT::LAST_CLUSTER);

	// Finally, write the file data
	size_t fatEntryCount = 0;
	size_t bytesWritten = 0;
	size_t sectorsWritten = 0;
	u32 address = GetDataClusterAddr(memcard, fatEntries.at(fatEntryCount));

	for (u8 fileByte : fileEntry->fileData)
	{
		memcard->GetMemcardDataRef().at(address + bytesWritten++) = fileByte;
		
		// When we've written 512 data bytes and are now on the ECC section of
		// a sector
		if (bytesWritten == static_cast<size_t>(SectorSize::STANDARD))
		{
			// Offset address by the amount of data written (512 bytes)
			// plus 16 bytes of ECC which we did not write (this is a later step).
			// Then, reset bytesWritten and increment our sectorsWritten counter.
			address += BASE_SECTOR_SIZE_WITH_ECC;
			bytesWritten = 0;
			sectorsWritten++;
		}

		// When both sectors in a cluster are written
		if (sectorsWritten == static_cast<size_t>(ClusterSize::STANDARD))
		{
			// Reset the sectorsWritten counter, and find the next data cluster
			// to continue writing.
			sectorsWritten = 0;
			fatEntryCount++;

			// Prevent overflow on fatEntries; if this condition is not true,
			// then that means we are on the last bytes and should be exiting anyways.
			if (fatEntryCount < fatEntries.size())
			{
				address = GetDataClusterAddr(memcard, fatEntries.at(fatEntryCount));
			}
		}
	}

	return fatEntries.at(0);
}

size_t MemcardFolderIO::GetDataClusterAddr(Memcard* memcard, size_t fatEntry)
{
	return STANDARD_CLUSTER_SIZE * (STANDARD_DATA_OFFSET_CLUSTERS + fatEntry);
}

size_t MemcardFolderIO::GetFirstFreeFatEntry(Memcard* memcard)
{
	u32 sectorStart = STANDARD_FAT_OFFSET;
	u32 address = sectorStart;
	u32 nextAddress = address;
	u32 fatValue;

	do
	{
		fatValue = 0;

		// If we're in to ECC, jump over and start the next sector
		if (nextAddress >= sectorStart + static_cast<u32>(SectorSize::STANDARD))
		{
			sectorStart += BASE_SECTOR_SIZE_WITH_ECC;
			address = sectorStart;
			nextAddress = address;
		}

		for (size_t i = 0; i < 4; i++)
		{
			fatValue |= (memcard->GetMemcardDataRef().at(nextAddress + i) << (i * 8));
		}

		address = nextAddress;
		nextAddress += 4;

		if (address >= STANDARD_DATA_OFFSET)
		{
			Console.Warning("%s(memcard) Exceeded FAT boundary! This memory card is OVER CAPACITY!", __FUNCTION__);
			Console.Warning("Some data will still work, but the last files added to the memcard may be missing or corrupt.");
			Console.Warning("Data loss may occur if care is not taken; PLEASE consider enabling 'Memory Card Filtering' and restarting your game.");
			Console.Warning("If 'Memory Card Filtering' is enabled and you are seeing this message, please report this to the PCSX2 team.");
		}
	} while (fatValue & FAT::IN_USE_MASK);

	// First, figure out from the start of the FAT, how many bytes leading up to address are actually for ECC.
	// These will need to be subtracted later in order to guarantee correct values for FAT entries.
	const u32 eccBytesToSubtract = ((address - STANDARD_FAT_OFFSET) / BASE_SECTOR_SIZE_WITH_ECC) * ECC_BYTES;
	// Return the position of the open FAT entry, minus the starting offset to get its relative
	// position, divide by 4 to convert relative position from bytes to u32's. This gives us the
	// data cluster position and can be used later in SetFatEntry to write a value to the FAT.
	u32 ret = ((address - STANDARD_FAT_OFFSET) - eccBytesToSubtract) / 4;
	return ret;
}

void MemcardFolderIO::SetFatEntry(Memcard* memcard, size_t position, u32 newValue)
{
	const u32 bytePosition = (position * 4) + STANDARD_FAT_OFFSET;
	// The inverse of the above, to go from position to byte address, we need to account for ECC presence
	// and add that length to our target address.
	const u32 eccBytesToAdd = ((bytePosition - STANDARD_FAT_OFFSET) / static_cast<u32>(SectorSize::STANDARD)) * ECC_BYTES;
	
	for (size_t i = 0; i < 4; i++)
	{
		const u32 shifted = (newValue >> (i * 8));
		const u8 trunc = shifted & 0xff;
		memcard->GetMemcardDataRef().at(bytePosition + eccBytesToAdd + i) = (newValue >> (i * 8));
	}
}

// Computes ECC for all sectors of the memcard. Every 128 bytes of a sector = 3 ECC bytes.
void MemcardFolderIO::ComputeAllECC(Memcard* memcard)
{
	for (size_t sectorPosition = 0; sectorPosition < static_cast<u32>(SectorCount::STANDARD); sectorPosition++)
	{
		std::array<u8, static_cast<u16>(SectorSize::STANDARD)> sectorData = {0};
		const size_t offset = (sectorPosition * BASE_SECTOR_SIZE_WITH_ECC);
		memcpy(sectorData.data(), memcard->GetMemcardDataRef().data() + offset, static_cast<u32>(SectorSize::STANDARD));
		
		// Grab 128 byte chunks and do ECC
		for (size_t chunk = 0; chunk < 4; chunk++)
		{
			std::array<u8, 128> input = {0};
			memcpy(input.data(), sectorData.data() + (chunk * 128), 128);
			std::array<u8, 3> ecc = ComputeECC(input);
			const size_t eccOffset = offset + static_cast<u32>(SectorSize::STANDARD) + (chunk * 3);
			memcpy(memcard->GetMemcardDataRef().data() + eccOffset, ecc.data(), ecc.size());
		}

		const size_t eccBlankOffset = offset + static_cast<u32>(SectorSize::STANDARD) + 12;
		memset(memcard->GetMemcardDataRef().data() + eccBlankOffset, 0xff, 4);
	}
	
}

std::array<u8, 3> MemcardFolderIO::ComputeECC(std::array<u8, 128> input)
{
	// Original code from http://www.oocities.org/siliconvalley/station/8269/sma02/sma02.html#ECC
	std::array<u8, 3> ecc = {0};
	int i, c;

	for (i = 0; i < 0x80; i++)
	{
		c = ECC_TABLE[input[i]];

		ecc[0] ^= c;

		if (c & 0x80)
		{
			ecc[1] ^= ~i;
			ecc[2] ^= i;
		}
	}

	ecc[0] = ~ecc[0];
	ecc[0] &= 0x77;

	ecc[1] = ~ecc[1];
	ecc[1] &= 0x7f;

	ecc[2] = ~ecc[2];
	ecc[2] &= 0x7f;

	return ecc;
}

void MemcardFolderIO::CleanupDirectoryEntries()
{

}

void MemcardFolderIO::Debug_PrintDirectoryTree(DirectoryEntry* entry, u32 level)
{
	if (entry != nullptr)
	{
		std::string indent;

		for (u32 i = 0; i < level; i++)
		{
			indent += "\t";
		}

		DevCon.WriteLn("%s%s", indent.c_str(), entry->name.c_str());
		const u32 newLevel = level + 1;

		for (DirectoryEntry* child : entry->children)
		{
			Debug_PrintDirectoryTree(child, newLevel);
		}
	}
}

void MemcardFolderIO::Debug_DumpFat(Memcard* memcard)
{
	FileSystem::WriteBinaryFile("C:/Users/Brian/Documents/PCSX2/memcards_v2/fat.bin", memcard->GetMemcardDataRef().data() + STANDARD_FAT_OFFSET, STANDARD_DATA_OFFSET - STANDARD_FAT_OFFSET);
}

void MemcardFolderIO::Debug_DumpCard(Memcard* memcard)
{
	FileSystem::WriteBinaryFile("C:/Users/Brian/Documents/PCSX2/memcards_v2/card.bin", memcard->GetMemcardDataRef().data(), memcard->GetMemcardDataRef().size());
}

MemcardFolderIO::MemcardFolderIO() = default;
MemcardFolderIO::~MemcardFolderIO() = default;

void MemcardFolderIO::Initialize(Memcard* memcard)
{
	FileSystem::CreateDirectoryPath(memcard->GetFullPath().c_str(), true);
	const std::string superblockFileName = memcard->GetFullPath() + FOLDER_MEMCARD_SUPERBLOCK_NAME;
	const bool superblockFileExists = FileSystem::FileExists(superblockFileName.c_str());

	if (!superblockFileExists)
	{
		const std::array<char, FOLDER_MEMCARD_SUPERBLOCK_SIZE> buf{0};
		FileSystem::WriteBinaryFile(superblockFileName.c_str(), buf.data(), buf.size());
	}
}

void MemcardFolderIO::Load(Memcard* memcard)
{
	// Set up the root
	this->root = new DirectoryEntry();
	InsertDotDirectories(root);

	// Load the superblock
	const std::string superblockFileName = memcard->GetFullPath() + "/" + FOLDER_MEMCARD_SUPERBLOCK_NAME;
	const std::optional<std::vector<u8>> dataOpt = FileSystem::ReadBinaryFile(superblockFileName.c_str());

	if (dataOpt.has_value())
	{
		const std::vector<u8> data = dataOpt.value();

		for (size_t pos = 0; pos < data.size(); pos++)
		{
			memcard->GetMemcardDataRef().push_back(data.at(pos));
		}
	}

	// Old memcards store 8192 bytes of superblock data, including ECC bytes up to that point,
	// but fail to account for ECC's presence making the real size of 8192 data bytes an actual 8448 bytes.
	// Add that padding now; this is fine as junk data, since this section of the memcard is unused,
	// and ECC will all be recalculated later.
	while (memcard->GetMemcardDataRef().size() < STANDARD_IFAT_OFFSET)
	{
		memcard->GetMemcardDataRef().push_back(0xff);
	}

	// Next, fill the indirect FAT
	for (const u32 iFatEntry : STANDARD_INDIRECT_FAT)
	{
		memcard->GetMemcardDataRef().push_back(iFatEntry);
		memcard->GetMemcardDataRef().push_back(0);
		memcard->GetMemcardDataRef().push_back(0);
		memcard->GetMemcardDataRef().push_back(0);
	}

	// Fill the FAT area with available entries
	while (memcard->GetMemcardDataRef().size() < STANDARD_DATA_OFFSET)
	{
		// Remember little endian... MSB at the end...
		memcard->GetMemcardDataRef().push_back(0xff);
		memcard->GetMemcardDataRef().push_back(0xff);
		memcard->GetMemcardDataRef().push_back(0xff);
		memcard->GetMemcardDataRef().push_back(0x7f);
	}

	// Fill the rest of the memcard bytes which are not yet in use
	while (memcard->GetMemcardDataRef().size() < BASE_8MB_SIZE)
	{
		memcard->GetMemcardDataRef().push_back(0xff);
	}

	// Get directories and files off the host filesystem, and make a tree
	RecurseDirectory(memcard->GetFullPath().c_str(), root);
	Console.WriteLn("Root contains %d members", root->children.size());
	// For debugging, print out the directory tree
	Debug_PrintDirectoryTree(root, 0);
	
	// If a directory had an index, use that to reorder its contents correctly
	ApplyIndexes();

	// Now, use the directory tree to fill out the FAT, and write data clusters
	// as defined by that FAT.
	CommitDirectory(memcard, root);

	// Update ECC values for the entire card
	ComputeAllECC(memcard);
	// Finally, delete the directory entries from the heap
	CleanupDirectoryEntries();

	Debug_DumpFat(memcard);
	Debug_DumpCard(memcard);
	Console.WriteLn("%s(memcard) Function complete", __FUNCTION__);
}

void MemcardFolderIO::Write(Memcard* memcard, u32 address, size_t location)
{

}
