
#pragma once

#include "Sio2Types.h"

#include <array>

// A note about fifoIn and fifoOut:
// On hardware, these are 32 bit registers. However, they are only accessed by 8 bit reads or writes
// of the LSB.
//
// Comparing to the PS1 we get some moderately interesting behavior. The PS1 (SIO0) uses a single register
// for both the send and receive byte; the send byte is written to the register, processed by the device,
// and then the device's response is immediately written over top of the send byte in the same register.
// SIO2 on the other hand has separate send and receive registers. Rather than SIO2 handling alternating
// between send and receive bytes, all bytes are sent in sequence, then all bytes are received in sequence.
// Presumably, devices are still being sent all the bytes in real time as we'd expect, but waiting to respond
// until the console indicates it is ready with an interrupt. This interrupt seems to be when the
// Sio2Ctrl::START_TRANSFER mask returns true, after a CTRL write. After this, the game does repeated reads
// until it has all the data expected.
//
// We are representing fifoOut as a vector here to comply with the above. As for fifoIn, there really isn't
// any good reason to store it anywhere, it is essentially useless outside of Sio2Write (the data param of
// a Sio2Write invocation = what the PS2 tried to write to fifoIn).

class Sio2
{
private:
	Sio2Mode mode = Sio2Mode::NOT_SET;

	std::array<u32, 16> send3 = {};
	std::array<u32, 4> send1 = {};
	std::array<u32, 4> send2 = {};
	size_t fifoPosition = 0;
	std::vector<u8> fifoOut;
	u32 ctrl;
	u32 recv1;
	u32 recv2;
	u32 recv3;
	u32 unknown1;
	u32 unknown2;
	u32 iStat;
	
	u8 activePort;
	bool send3Read = false;
	size_t send3Position = 0;
	size_t commandLength = 0;
	size_t processedLength = 0;
public:
	Sio2();
	~Sio2();

	void Reset();

	void SetInterrupt();

	void Sio2Write(u8 data);
	u8 Sio2Read();

	u32 GetSend1(u8 index);
	u32 GetSend2(u8 index);
	u32 GetSend3(u8 index);
	u32 GetCtrl();
	u32 GetRecv1();
	u32 GetRecv2();
	u32 GetRecv3();
	u32 GetUnknown1();
	u32 GetUnknown2();
	u32 GetIStat();

	void SetSend1(u8 index, u32 data);
	void SetSend2(u8 index, u32 data);
	void SetSend3(u8 index, u32 data);
	void SetCtrl(u32 data);
	void SetRecv1(u32 data);
	void SetRecv2(u32 data);
	void SetRecv3(u32 data);
	void SetUnknown1(u32 data);
	void SetUnknown2(u32 data);
	void SetIStat(u32 data);
};

extern Sio2 g_Sio2;
