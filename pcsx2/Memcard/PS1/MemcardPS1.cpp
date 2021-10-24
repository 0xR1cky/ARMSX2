
#include "PrecompiledHeader.h"
#include "MemcardPS1.h"

#include "IopHw.h"
#include "IopMem.h"

#define MEMCARD_PATH "./test_ps1.mcr"

bool MemcardPS1::FetchFromDisk()
{
	std::ifstream ifstream;
	ifstream.open(MEMCARD_PATH);

	if (!ifstream.good())
	{
		//DevCon.Warning("%s - ifstream in fail state, aborting", __FUNCTION__);
		return false;
	}

	ifstream.read(reinterpret_cast<char*>(memcardData.data()), memcardData.size());
	return true;
}

bool MemcardPS1::CommitToDisk()
{
	std::ofstream ofstream;
	ofstream.open(MEMCARD_PATH);

	if (!ofstream.good())
	{
		//DevCon.Warning("%s - ofstream in fail state, aborting", __FUNCTION__);
		return false;
	}

	ofstream.write(reinterpret_cast<char*>(memcardData.data()), memcardData.size());
	return true;
}

MemcardPS1::MemcardPS1()
{
	Init(); // TODO: Move out of constructor, make Sio0 invoke init on memcards based on config
}

MemcardPS1::~MemcardPS1() = default;

void MemcardPS1::Init()
{
	memset(memcardData.data(), 0xff, memcardData.size());

	if (!FetchFromDisk())
	{
		CommitToDisk();
	}
}

u8 MemcardPS1::GetFlag()
{
	return flag;
}

void MemcardPS1::SetFlag(u8 data)
{
	flag = data;
}

u8* MemcardPS1::GetMemcardDataPointer()
{
	return memcardData.data();
}

void MemcardPS1::Read(u8* dest, u16 offset, u8 length)
{
	u16 endPos = offset + length;

	if (offset >= MEMCARD_SIZE || endPos >= MEMCARD_SIZE)
	{
		DevCon.Warning("%s(%016X, %d, %d) - Exceeded bounds of memcard data (%d >= %d)", __FUNCTION__, dest, offset, length, endPos, MEMCARD_SIZE);
		return;
	}

	memcpy(dest, GetMemcardDataPointer() + offset, length);
}

// Length of src is unchecked; calling functions should ensure
// safety of passed in length and src arguments
void MemcardPS1::Write(u8* src, u16 offset, u8 length)
{
	u16 endPos = offset + length;

	if (offset >= MEMCARD_SIZE || endPos >= MEMCARD_SIZE)
	{
		DevCon.Warning("%s(%016X, %d, %d) - Exceeded bounds of memcard data (%d >= %d)", __FUNCTION__, src, offset, length, endPos, MEMCARD_SIZE);
		return;
	}

	memcpy(GetMemcardDataPointer() + offset, src, length);
}

#undef ResponseCapacityCheck()
#undef MEMCARD_PATH
