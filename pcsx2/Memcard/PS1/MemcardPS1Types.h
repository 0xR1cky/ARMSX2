
#pragma once

static constexpr size_t SECTOR_SIZE = 128;
static constexpr size_t MEMCARD_SIZE = SECTOR_SIZE * 1024;

enum class MemcardPS1Mode
{
	NOT_SET = 0x00,
	INIT = 0x81,
	READ = 0x52,
	STATE = 0x53,
	WRITE = 0x57,
	PS_STATE = 0x58,
	DONE = 0x7f,
	INVALID = 0xff
};

namespace Flag
{
	static constexpr u8 WriteError = 0x04;
	static constexpr u8 DirectoryRead = 0x08;
}
