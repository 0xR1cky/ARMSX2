/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2021  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "PrecompiledHeader.h"
#include "SPU2/Global.h"
#include "SPU2/Debug.h"
#include "SPU2/spu2.h"
#include "SPU2/Dma.h"
#include "R3000A.h"

namespace SPU2
{
	static int GetConsoleSampleRate();
	static void InitSndBuffer();
	static void UpdateSampleRate();
	static void InternalReset(bool psxmode);
} // namespace SPU2

static double s_device_sample_rate_multiplier = 1.0;
static bool s_psxmode = false;

int SampleRate = 48000;

u32 lClocks = 0;

int SPU2::GetConsoleSampleRate()
{
	return s_psxmode ? 44100 : 48000;
}

// --------------------------------------------------------------------------------------
//  DMA 4/7 Callbacks from Core Emulator
// --------------------------------------------------------------------------------------


void SPU2readDMA4Mem(u16* pMem, u32 size) // size now in 16bit units
{
	TimeUpdate(psxRegs.cycle);

	SPU2::FileLog("[%10d] SPU2 readDMA4Mem size %x\n", Cycles, size << 1);
	Cores[0].DoDMAread(pMem, size);
}

void SPU2writeDMA4Mem(u16* pMem, u32 size) // size now in 16bit units
{
	TimeUpdate(psxRegs.cycle);

	SPU2::FileLog("[%10d] SPU2 writeDMA4Mem size %x at address %x\n", Cycles, size << 1, Cores[0].TSA);

	Cores[0].DoDMAwrite(pMem, size);
}

void SPU2interruptDMA4()
{
	SPU2::FileLog("[%10d] SPU2 interruptDMA4\n", Cycles);
	if (Cores[0].DmaMode)
		Cores[0].Regs.STATX |= 0x80;
	Cores[0].Regs.STATX &= ~0x400;
	Cores[0].TSA = Cores[0].ActiveTSA;
}

void SPU2interruptDMA7()
{
	SPU2::FileLog("[%10d] SPU2 interruptDMA7\n", Cycles);
	if (Cores[1].DmaMode)
		Cores[1].Regs.STATX |= 0x80;
	Cores[1].Regs.STATX &= ~0x400;
	Cores[1].TSA = Cores[1].ActiveTSA;
}

void SPU2readDMA7Mem(u16* pMem, u32 size)
{
	TimeUpdate(psxRegs.cycle);

	SPU2::FileLog("[%10d] SPU2 readDMA7Mem size %x\n", Cycles, size << 1);
	Cores[1].DoDMAread(pMem, size);
}

void SPU2writeDMA7Mem(u16* pMem, u32 size)
{
	TimeUpdate(psxRegs.cycle);

	SPU2::FileLog("[%10d] SPU2 writeDMA7Mem size %x at address %x\n", Cycles, size << 1, Cores[1].TSA);

	Cores[1].DoDMAwrite(pMem, size);
}

void SPU2::InitSndBuffer()
{
	Console.WriteLn("Initializing SndBuffer at sample rate of %u...", SampleRate);
	if (SndBuffer::Init(EmuConfig.SPU2.OutputModule.c_str()))
		return;

	if (SampleRate != GetConsoleSampleRate())
	{
		// It'll get stretched instead..
		const int original_sample_rate = SampleRate;
		Console.Error("Failed to init SPU2 at adjusted sample rate %u, trying console rate.", SampleRate);
		SampleRate = GetConsoleSampleRate();
		if (SndBuffer::Init(EmuConfig.SPU2.OutputModule.c_str()))
			return;

		SampleRate = original_sample_rate;
	}

	// just use nullout
	if (!SndBuffer::Init("nullout"))
		pxFailRel("Failed to initialize nullout.");
}

void SPU2::UpdateSampleRate()
{
	const int new_sample_rate = static_cast<int>(std::round(static_cast<double>(GetConsoleSampleRate()) * s_device_sample_rate_multiplier));
	if (SampleRate == new_sample_rate)
		return;

	SndBuffer::Cleanup();
	SampleRate = new_sample_rate;
	InitSndBuffer();
}

void SPU2::InternalReset(bool psxmode)
{
	s_psxmode = psxmode;
	if (!s_psxmode)
	{
		memset(spu2regs, 0, 0x010000);
		memset(_spu2mem, 0, 0x200000);
		memset(_spu2mem + 0x2800, 7, 0x10); // from BIOS reversal. Locks the voices so they don't run free.
		memset(_spu2mem + 0xe870, 7, 0x10); // Loop which gets left over by the BIOS, Megaman X7 relies on it being there.

		Spdif.Info = 0; // Reset IRQ Status if it got set in a previously run game

		Cores[0].Init(0);
		Cores[1].Init(1);
	}
}

void SPU2::Reset(bool psxmode)
{
	InternalReset(psxmode);
	UpdateSampleRate();
}

void SPU2::OnTargetSpeedChanged()
{
	if (EmuConfig.SPU2.SynchMode != Pcsx2Config::SPU2Options::SynchronizationMode::TimeStretch)
		SndBuffer::ResetBuffers();
}

void SPU2::SetDeviceSampleRateMultiplier(double multiplier)
{
	if (s_device_sample_rate_multiplier == multiplier)
		return;

	s_device_sample_rate_multiplier = multiplier;
	UpdateSampleRate();
}

bool SPU2::Initialize()
{
	pxAssert(regtable[0x400] == nullptr);
	spu2regs = (s16*)malloc(0x010000);
	_spu2mem = (s16*)malloc(0x200000);

	// adpcm decoder cache:
	//  the cache data size is determined by taking the number of adpcm blocks
	//  (2MB / 16) and multiplying it by the decoded block size (28 samples).
	//  Thus: pcm_cache_data = 7,340,032 bytes (ouch!)
	//  Expanded: 16 bytes expands to 56 bytes [3.5:1 ratio]
	//    Resulting in 2MB * 3.5.

	pcm_cache_data = (PcmCacheEntry*)calloc(pcm_BlockCount, sizeof(PcmCacheEntry));

	if (!spu2regs || !_spu2mem || !pcm_cache_data)
	{
		Console.Error("SPU2: Error allocating Memory");
		return false;
	}

	// Patch up a copy of regtable that directly maps "nullptrs" to SPU2 memory.

	memcpy(regtable, regtable_original, sizeof(regtable));

	for (uint mem = 0; mem < 0x800; mem++)
	{
		u16* ptr = regtable[mem >> 1];
		if (!ptr)
		{
			regtable[mem >> 1] = &(spu2Ru16(mem));
		}
	}

	InitADSR();
	return true;
}


bool SPU2::Open()
{
#ifdef PCSX2_DEVBUILD
	if (SPU2::AccessLog())
		SPU2::OpenFileLog();
#endif

#ifdef PCSX2_DEVBUILD
	DMALogOpen();

	FileLog("[%10d] SPU2 Open\n", Cycles);
#endif

	lClocks = psxRegs.cycle;

	InternalReset(false);

	SampleRate = static_cast<int>(std::round(static_cast<double>(GetConsoleSampleRate()) * s_device_sample_rate_multiplier));
	InitSndBuffer();
#ifdef PCSX2_DEVBUILD
	WaveDump::Open();
#endif

	SetOutputVolume(EmuConfig.SPU2.FinalVolume);
	return true;
}

void SPU2::Close()
{
	FileLog("[%10d] SPU2 Close\n", Cycles);

	SndBuffer::Cleanup();

#ifdef PCSX2_DEVBUILD
	WaveDump::Close();
	DMALogClose();

	DoFullDump();
	CloseFileLog();
#endif
}

void SPU2::Shutdown()
{
	safe_free(spu2regs);
	safe_free(_spu2mem);
	safe_free(pcm_cache_data);
}

bool SPU2::IsRunningPSXMode()
{
	return s_psxmode;
}

void SPU2async(u32 cycles)
{
	TimeUpdate(psxRegs.cycle);
}

u16 SPU2read(u32 rmem)
{
	u16 ret = 0xDEAD;
	u32 core = 0, mem = rmem & 0xFFFF, omem = mem;

	if (mem & 0x400)
	{
		omem ^= 0x400;
		core = 1;
	}

	if (omem == 0x1f9001AC)
	{
		Cores[core].ActiveTSA = Cores[core].TSA;
		for (int i = 0; i < 2; i++)
		{
			if (Cores[i].IRQEnable && (Cores[i].IRQA == Cores[core].ActiveTSA))
			{
				SetIrqCall(i);
			}
		}
		ret = Cores[core].DmaRead();
	}
	else
	{
		TimeUpdate(psxRegs.cycle);

		if (rmem >> 16 == 0x1f80)
		{
			ret = Cores[0].ReadRegPS1(rmem);
		}
		else if (mem >= 0x800)
		{
			ret = spu2Ru16(mem);
			if (SPU2::MsgToConsole())
				SPU2::ConLog("* SPU2: Read from reg>=0x800: %x value %x\n", mem, ret);
		}
		else
		{
			ret = *(regtable[(mem >> 1)]);
#ifdef PCSX2_DEVBUILD
			//FileLog("[%10d] SPU2 read mem %x (core %d, register %x): %x\n",Cycles, mem, core, (omem & 0x7ff), ret);
			SPU2::WriteRegLog("read", rmem, ret);
#endif
		}
	}

	return ret;
}

void SPU2write(u32 rmem, u16 value)
{
	// Note: Reverb/Effects are very sensitive to having precise update timings.
	// If the SPU2 isn't in in sync with the IOP, samples can end up playing at rather
	// incorrect pitches and loop lengths.

	TimeUpdate(psxRegs.cycle);

	if (rmem >> 16 == 0x1f80)
		Cores[0].WriteRegPS1(rmem, value);
	else
	{
#ifdef PCSX2_DEVBUILD
		SPU2::WriteRegLog("write", rmem, value);
#endif
		SPU2_FastWrite(rmem, value);
	}
}

// returns a non zero value if successful
bool SPU2setupRecording(const std::string* filename)
{
	return RecordStart(filename);
}

void SPU2endRecording()
{
	if (WavRecordEnabled)
		RecordStop();
}

s32 SPU2freeze(FreezeAction mode, freezeData* data)
{
	pxAssume(data != nullptr);
	if (!data)
	{
		printf("SPU2 savestate null pointer!\n");
		return -1;
	}

	if (mode == FreezeAction::Size)
	{
		data->size = SPU2Savestate::SizeIt();
		return 0;
	}

	pxAssume(mode == FreezeAction::Load || mode == FreezeAction::Save);

	if (data->data == nullptr)
	{
		printf("SPU2 savestate null pointer!\n");
		return -1;
	}

	auto& spud = (SPU2Savestate::DataBlock&)*(data->data);

	switch (mode)
	{
		case FreezeAction::Load:
			return SPU2Savestate::ThawIt(spud);
		case FreezeAction::Save:
			return SPU2Savestate::FreezeIt(spud);

			jNO_DEFAULT;
	}

	// technically unreachable, but kills a warning:
	return 0;
}

void SPU2::CheckForConfigChanges(const Pcsx2Config& old_config)
{
	if (EmuConfig.SPU2 == old_config.SPU2)
		return;

	const Pcsx2Config::SPU2Options& opts = EmuConfig.SPU2;
	const Pcsx2Config::SPU2Options& oldopts = old_config.SPU2;

	// No need to reinit for volume change.
	if (opts.FinalVolume != oldopts.FinalVolume)
		SetOutputVolume(opts.FinalVolume);

	// Wipe buffer out when changing sync mode, so e.g. TS->none doesn't have a huge delay.
	if (opts.SynchMode != oldopts.SynchMode)
		SndBuffer::ResetBuffers();

	// Things which require re-initialzing the output.
	if (opts.Latency != oldopts.Latency ||
		opts.OutputLatency != oldopts.OutputLatency ||
		opts.OutputLatencyMinimal != oldopts.OutputLatencyMinimal ||
		opts.OutputModule != oldopts.OutputModule ||
		opts.BackendName != oldopts.BackendName ||
		opts.DeviceName != oldopts.DeviceName ||
		opts.SpeakerConfiguration != oldopts.SpeakerConfiguration ||
		opts.DplDecodingLevel != oldopts.DplDecodingLevel ||
		opts.SequenceLenMS != oldopts.SequenceLenMS ||
		opts.SeekWindowMS != oldopts.SeekWindowMS ||
		opts.OverlapMS != oldopts.OverlapMS)
	{
		SndBuffer::Cleanup();
		InitSndBuffer();
	}

#ifdef PCSX2_DEVBUILD
	// AccessLog controls file output.
	if (opts.AccessLog != oldopts.AccessLog)
	{
		if (AccessLog())
			OpenFileLog();
		else
			CloseFileLog();
	}
#endif
}
