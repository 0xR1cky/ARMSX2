
#pragma once

#include "Sio0Types.h"
#include <array>
#include "Memcard/PS1/MemcardPS1Protocol.h"
#include "PAD/PS1/PadPS1Protocol.h"

class Sio0
{
private:
	Sio0Mode mode = Sio0Mode::NOT_SET;
	
	u8 sioData;
	u32 sioStat;
	u16 sioMode;
	u16 sioCtrl;
	u16 sioBaud;
public:
	Sio0();
	~Sio0();

	u8 GetSioData();
	u32 GetSioStat();
	u16 GetSioMode();
	u16 GetSioCtrl();
	u16 GetSioBaud();

	void SetData(u8 data);
	void SetStat(u32 data);
	void SetMode(u16 data);
	void SetCtrl(u16 data);
	void SetBaud(u16 data);

	void Reset();
	void SetInterrupt();
	void ClearInterrupt();
};

extern Sio0 g_Sio0;
