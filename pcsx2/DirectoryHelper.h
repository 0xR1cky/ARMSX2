
#pragma once

#include "ghc/filesystem.h"
#include <cstdlib>

// OS-agnostic getter for the user folder.
ghc::filesystem::path GetHomeDirectory()
{
	char* ret;
#ifdef _WIN32
	ret = std::getenv("userprofile");
#else
	ret = std::getenv("HOME");
#endif
	return ghc::filesystem::path(ret);
}
