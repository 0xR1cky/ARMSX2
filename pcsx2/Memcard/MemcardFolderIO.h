
#pragma once

#include "ryml_std.hpp"
#include "ryml.hpp"

#include "Memcard.h"

#include <array>
#include <vector>
#include <optional>

// Structure representing an individual entry in an index file. An individual entry is considered
// the top level node (whose only parent is the root itself), and any of its children.
struct DirectoryIndexEntry
{
	std::string name;
	std::array<u8, 8> created;
	std::array<u8, 8> modified;
	size_t position;
};

// Structure representing a first level directory (the directory's only parent level is the root itself)
// which contains an index. Tracks the name of the directory, and each entry in the index file.
struct DirectoryIndex
{
	std::string name;
	std::vector<DirectoryIndexEntry> entries;
};

// Using a class instead of a struct; this is easier to manage on the heap with pointers than it is
// keeping it on the stack and tossing references everywhere.
class DirectoryEntry
{
public:
	u16 flags = 0;
	std::array<u8, 8> created = {0};
	std::array<u8, 8> modified = {0};
	std::string name;
	
	std::vector<DirectoryEntry*> children;
	std::vector<u8> fileData;
};

class MemcardFolderIO
{
private:
	std::vector<DirectoryIndex> indexes;
	DirectoryEntry* root;

	std::array<u8, 8> UnixTimeToPS2(const u64& unixTime);
	u64 PS2TimeToUnix(const std::array<u8, 8>& ps2Time);

	ryml::Tree TreeFromString(const std::string& s);
	std::optional<ryml::Tree> ReadYamlFromFile(const char* yamlFileName);
	void WriteYamlToFile(const char* yamlFileName, const ryml::NodeRef& node);
	void GenerateIndex();
	void RecurseDirectory(const char* directory, DirectoryEntry* currentEntry);
	void LoadIndexFile(const char* indexFilePath);
	std::vector<u8> LoadFile(const char* fileName);
	void PrintDirectoryTree(DirectoryEntry* entry, u32 level);
	
	DirectoryEntry* GetRootDirPtr();
	void InsertDotDirectories(DirectoryEntry* dirEntry);
	void CommitDirectoryTree(Memcard* memcard);
	void CommitDirectoryEntry(Memcard* memcard, DirectoryEntry dirEntry);
	size_t GetFreeFatCluster(Memcard* memcard);
	void SetFatCluster(Memcard* memcard, size_t position, u32 newValue);
	std::array<u8, ECC_BYTES> ComputeECC(std::vector<u8> sectorData);

public:
	MemcardFolderIO();
	~MemcardFolderIO();

	void Initialize(Memcard* memcard);
	void Load(Memcard* memcard);
	void Write(Memcard* memcard, u32 address, size_t length);
};

extern MemcardFolderIO g_MemcardFolderIO;
