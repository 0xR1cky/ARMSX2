
#pragma once

#include <string>
#include <cstdlib>

// OS-agnostic getter for the user folder.
std::string GetHomeDirectory()
{
	char* ret;
#ifdef _WIN32
	ret = std::getenv("userprofile");
#else
	ret = std::getenv("HOME");
#endif
	return std::string(ret);
}
