
#pragma once

#include "ryml_std.hpp"
#include "ryml.hpp"

#include "MemcardFolderIOTypes.h"
#include "Memcard.h"

#include <array>
#include <vector>
#include <optional>

// Constant size structure representing a directory entry sector on a PS2 memcard.
// Unlike the DirectoryEntry class below which is a high level, easier to use class
// for manipulation, this should only be used as a nice stencil to copy directory
// entries directly into memcard data, not any kind of data manipulation.
struct PS2Directory
{
	u16 mode = 0;
	u16 unused = 0;
	u32 length = 0;
	char created[8] = {0};
	u32 cluster = 0;
	u32 dirEntry = 0;
	char modified[8] = {0};
	u32 attr1 = 0;
	u32 attr2 = 0;
	u32 attr3 = 0;
	u32 attr4 = 0;
	u32 attr5 = 0;
	u32 attr6 = 0;
	u32 attr7 = 0;
	u32 attr8 = 0;
	char name[32] = {0};
};

enum class DirectoryType
{
	DIRECTORY,
	FILE
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
	DirectoryType type;
	
	std::vector<DirectoryEntry*> children;
	std::vector<u8> fileData;
};

// Representation of a single entry in a _pcsx2_index file
struct DirectoryIndexEntry
{
	std::string name;
	std::array<u8, 8> created;
	std::array<u8, 8> modified;
	size_t order;
};

// Contains the DirectoryEntry that an index was found inside,
// and pairs it with the collection of entries inside the index.
struct DirectoryIndex
{
	DirectoryEntry* directoryEntry;
	std::vector<DirectoryIndexEntry> entries;
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
	void RecurseDirectory(const char* directory, DirectoryEntry* currentEntry);
	void InsertDotDirectories(DirectoryEntry* dirEntry);
	void LoadIndexFile(const char* indexFilePath, DirectoryEntry* currentEntry);
	std::vector<u8> LoadFile(const char* fileName);
	void ApplyIndexes();
	u32 CommitDirectory(Memcard* memcard, DirectoryEntry* dirEntry, u32 parentEntryPos = 0);
	u32 CommitFile(Memcard* memcard, DirectoryEntry* fileEntry);
	size_t GetDataClusterAddr(Memcard* memcard, size_t fatEntry);
	size_t GetFirstFreeFatEntry(Memcard* memcard);
	void SetFatEntry(Memcard* memcard, size_t position, u32 newValue);
	void ComputeAllECC(Memcard* memcard);
	std::array<u8, 3> ComputeECC(std::array<u8, 128> input);
	void CleanupDirectoryEntries();

	void Debug_PrintDirectoryTree(DirectoryEntry* entry, u32 level);
	void Debug_DumpFat(Memcard* memcard);
	void Debug_DumpCard(Memcard* memcard);

public:
	MemcardFolderIO();
	~MemcardFolderIO();

	void Initialize(Memcard* memcard);
	void Load(Memcard* memcard);
	void Write(Memcard* memcard, u32 address, size_t length);
};

extern MemcardFolderIO g_MemcardFolderIO;
