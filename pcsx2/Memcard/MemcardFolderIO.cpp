
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
	if (FileSystem::FileExists(yamlFileName))
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

void MemcardFolderIO::GenerateIndex()
{
	
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
		DirectoryEntry* child = new DirectoryEntry();
		child->name = FileSystem::GetFileNameFromPath(result.FileName);

		if (result.Attributes & FILESYSTEM_FILE_ATTRIBUTE_DIRECTORY)
		{
			InsertDotDirectories(child);
			currentEntry->children.push_back(child);
			RecurseDirectory(result.FileName.c_str(), child);
		}
		else if (result.FileName == FOLDER_MEMCARD_INDEX_NAME)
		{
			LoadIndexFile(result.FileName.c_str());
		}
		else
		{
			child->fileData = LoadFile(result.FileName.c_str());
			currentEntry->children.push_back(child);
		}
	}
}

void MemcardFolderIO::LoadIndexFile(const char* indexFilePath)
{
	std::optional<ryml::Tree> yaml = ReadYamlFromFile(indexFilePath);

	if (yaml.has_value() && !yaml.value().empty())
	{
		ryml::NodeRef root = yaml.value().rootref();
		DirectoryIndex index;
		index.name = FileSystem::GetFileNameFromPath(indexFilePath);

		for (const ryml::NodeRef& node : root.children())
		{
			u64 unixTime;
			DirectoryIndexEntry entry;
			std::string fileName = std::string(node.key().str, node.key().len);
			entry.name = fileName;

			if (node.has_child("order"))
			{
				node["order"] >> entry.position;
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

void MemcardFolderIO::PrintDirectoryTree(DirectoryEntry* entry, u32 level)
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
			PrintDirectoryTree(child, newLevel);	
		}
	}
}

DirectoryEntry* MemcardFolderIO::GetRootDirPtr()
{
	return root;
}

void MemcardFolderIO::InsertDotDirectories(DirectoryEntry* dirEntry)
{
	DirectoryEntry* singleDot = new DirectoryEntry();
	singleDot->name = ".";
	singleDot->flags = SINGLE_DOT_MODE_FLAGS;
	dirEntry->children.push_back(singleDot);
	DirectoryEntry* doubleDot = new DirectoryEntry();
	doubleDot->name = "..";
	doubleDot->flags = DOUBLE_DOT_MODE_FLAGS;
	dirEntry->children.push_back(doubleDot);
}

void MemcardFolderIO::CommitDirectoryTree(Memcard* memcard)
{
/*
	CommitDirectoryEntry(memcard, root);
	// One dir entry per page, two pages per cluster, so divide number of dir entries by 2 and use ceiling.
	size_t fatClustersRequired = ceil(currentDir.children.size() / 2.0f);
	size_t currentClusterOffset = 0;
	u32 dataAddress = STANDARD_DATA_OFFSET + (currentClusterOffset * STANDARD_CLUSTER_SIZE);

	for (DirectoryEntry child : currentDir.children)
	{
		
	}
	
	for (size_t i = 0; i < fatClustersRequired; i++)
	{
		
	}
*/
}

void MemcardFolderIO::CommitDirectoryEntry(Memcard* memcard, DirectoryEntry dirEntry)
{
/*
	std::array<char, static_cast<size_t>(SectorSize::STANDARD)> directoryBytes = {0};
	directoryBytes.at(0) = (DEFAULT_DIRECTORY_MODE_FLAGS & 0xff);
	directoryBytes.at(1) = (DEFAULT_DIRECTORY_MODE_FLAGS >> 8);
	strcpy(directoryBytes.data() + 0x40, dirEntry.name.c_str());

	for (DirectoryEntry child : dirEntry.children)
	{
		CommitDirectoryEntry(memcard, child);
	}
*/
}

size_t MemcardFolderIO::GetFreeFatCluster(Memcard* memcard)
{
	u32 offset = STANDARD_FAT_OFFSET;
	u32 fatValue = 0;

	do
	{
		// We offset immediately rather than at the end of the loop to preserve offset for the
		// return. The FAT cluster at STANDARD_FAT_OFFSET is always guaranteed to be the root
		// directory, so there is no point in even attempting to check if it is in use.
		offset += 4;

		for (size_t i = 0; i < 4; i++)
		{
			fatValue &= (memcard->GetMemcardDataRef().at(offset + i) << (i * 8));
		}
	} while (fatValue & FAT_IN_USE_MASK);

	// Return the position of the open FAT entry, minus the starting offset to get its relative
	// position, divide by 4 to convert relative position from bytes to u32's. This gives us the
	// data cluster position and can be used later in SetFatCluster to write a value to the FAT.
	return (offset - STANDARD_FAT_OFFSET) / 4;
}

void MemcardFolderIO::SetFatCluster(Memcard* memcard, size_t position, u32 newValue)
{
	const u32 bytePosition = (position * 4) + STANDARD_FAT_OFFSET;

	for (size_t i = 0; i < 4; i++)
	{
		memcard->GetMemcardDataRef().at(bytePosition + i) = (newValue << (i * 8));
	}
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
	const std::string superblockFileName = memcard->GetFullPath() + FOLDER_MEMCARD_SUPERBLOCK_NAME;
	const std::optional<std::vector<u8>> dataOpt = FileSystem::ReadBinaryFile(superblockFileName.c_str());

	if (dataOpt.has_value())
	{
		const std::vector<u8> data = dataOpt.value();

		for (size_t pos = 0; pos < data.size(); pos++)
		{
			memcard->GetMemcardDataRef().push_back(data.at(pos));
		}
	}

	// Next, fill the indirect FAT
	for (const u32 iFatEntry : STANDARD_INDIRECT_FAT)
	{
		// Though this seems like a loss of precision, it is not. Indirect FAT
		// elements will always be a u8 value. More details on the comment
		// of STANDARD_INDIRECT_FAT.
		memcard->GetMemcardDataRef().push_back(static_cast<u8>(iFatEntry));
	}

	// Next, generate a FAT for the folder memcard contents, using the indirect
	// FAT described above, and populate game data.
	RecurseDirectory(memcard->GetFullPath().c_str(), root);

#define MEMCARD_DEBUG
#ifdef MEMCARD_DEBUG
	while (memcard->GetMemcardDataRef().size() < BASE_8MB_SIZE)
	{
		memcard->GetMemcardDataRef().push_back(0xff);
	}
#endif

	Console.WriteLn("Root contains %d members", root->children.size());

	PrintDirectoryTree(root, 0);

	Console.WriteLn("%s(memcard) Function complete", __FUNCTION__);
}

void MemcardFolderIO::Write(Memcard* memcard, u32 address, size_t location)
{

}
