
#pragma once

class MultitapConfig 
{
private:
	bool enableMultitap0 = true;
	bool enableMultitap1 = true;

public:
	MultitapConfig();
	~MultitapConfig();

	bool IsMultitapEnabled(size_t port);

	void SetMultitapEnabled(size_t port, bool data);
};

extern MultitapConfig g_MultitapConfig;
