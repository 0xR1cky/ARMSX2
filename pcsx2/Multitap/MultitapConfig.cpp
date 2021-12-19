
#include "PrecompiledHeader.h"
#include "MultitapConfig.h"

#include "SioTypes.h"

MultitapConfig g_MultitapConfig;

MultitapConfig::MultitapConfig() = default;
MultitapConfig::~MultitapConfig() = default;

bool MultitapConfig::IsMultitapEnabled(size_t port)
{
	port = std::clamp<size_t>(port, 0, MAX_PORTS);

	if (port == 0)
	{
		return enableMultitap0;
	}
	else if (port == 1)
	{
		return enableMultitap1;
	}
	else 
	{
		DevCon.Warning("%s(%d) Sanity check!", __FUNCTION__, port);
		return false;
	}
}

void MultitapConfig::SetMultitapEnabled(size_t port, bool data)
{
	port = std::clamp<size_t>(port, 0, MAX_PORTS);

	if (port == 0)
	{
		enableMultitap0 = data;
	}
	else if (port == 1)
	{
		enableMultitap1 = data;
	}
	else
	{
		DevCon.Warning("%s(%d) Sanity check!", __FUNCTION__, port);
	}
}
