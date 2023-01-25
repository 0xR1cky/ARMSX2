/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
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

#pragma once

#include "SysForwardDefs.h"

#include "common/Exceptions.h"
#include "common/SafeArray.h"
#include "common/Threading.h"		// to use threading stuff, include the Threading namespace in your file.

#include "vtlb.h"

#include "Config.h"

typedef SafeArray<u8> VmStateBuffer;

class BaseVUmicroCPU;

// This is a table of default virtual map addresses for ps2vm components.  These locations
// are provided and used to assist in debugging and possibly hacking; as it makes it possible
// for a programmer to know exactly where to look (consistently!) for the base address of
// the various virtual machine components.  These addresses can be keyed directly into the
// debugger's disasm window to get disassembly of recompiled code, and they can be used to help
// identify recompiled code addresses in the callstack.

// All of these areas should be reserved as soon as possible during program startup, and its
// important that none of the areas overlap.  In all but superVU's case, failure due to overlap
// or other conflict will result in the operating system picking a preferred address for the mapping.

namespace HostMemoryMap
{
	//////////////////////////////////////////////////////////////////////////
	// Main
	//////////////////////////////////////////////////////////////////////////
	static const u32 MainSize = 0x14000000;

	// PS2 main memory, SPR, and ROMs (approximately 40.5MB, but we round up to 64MB for simplicity).
	static const u32 EEmemOffset   = 0x00000000;

	// IOP main memory and ROMs
	static const u32 IOPmemOffset  = 0x04000000;

	// VU0 and VU1 memory.
	static const u32 VUmemOffset   = 0x08000000;

	// Bump allocator for any other small allocations
	// size: Difference between it and HostMemoryMap::Size, so nothing should allocate higher than it!
	static const u32 bumpAllocatorOffset = 0x10000000;

	//////////////////////////////////////////////////////////////////////////
	// Code
	//////////////////////////////////////////////////////////////////////////
	static const u32 CodeSize = 0x13100000; // 305 mb

	// EE recompiler code cache area (64mb)
	static const u32 EErecOffset   = 0x00000000;

	// IOP recompiler code cache area (32mb)
	static const u32 IOPrecOffset  = 0x04000000;

	// newVif0 recompiler code cache area (8mb)
	static const u32 VIF0recOffset = 0x06000000;

	// newVif1 recompiler code cache area (8mb)
	static const u32 VIF1recOffset = 0x06800000;

	// microVU1 recompiler code cache area (64mb)
	static const u32 mVU0recOffset = 0x07000000;

	// microVU0 recompiler code cache area (64mb)
	static const u32 mVU1recOffset = 0x0B000000;

	// SSE-optimized VIF unpack functions (1mb)
	static const u32 VIFUnpackRecOffset = 0x0F000000;

	// Software Renderer JIT buffer (64mb)
	static const u32 SWrecOffset = 0x0F100000;
	static const u32 SWrecSize = 0x04000000;
}

// --------------------------------------------------------------------------------------
//  RecompiledCodeReserve
// --------------------------------------------------------------------------------------
// A recompiled code reserve is a simple sequential-growth block of memory which is auto-
// cleared to INT 3 (0xcc) as needed.
//
class RecompiledCodeReserve : public VirtualMemoryReserve
{
	typedef VirtualMemoryReserve _parent;

protected:
	std::string m_profiler_name;

public:
	RecompiledCodeReserve(std::string name);
	~RecompiledCodeReserve();

	void Assign(VirtualMemoryManagerPtr allocator, size_t offset, size_t size);
	void Reset();

	RecompiledCodeReserve& SetProfilerName(std::string name);

	void ForbidModification();
	void AllowModification();

	operator u8*() { return m_baseptr; }
	operator const u8*() const { return m_baseptr; }

protected:
	void _registerProfiler();
};

// --------------------------------------------------------------------------------------
//  GSCodeReserve
// --------------------------------------------------------------------------------------
// Stores code buffers for the GS software JIT.
class GSCodeReserve : public RecompiledCodeReserve
{
public:
	GSCodeReserve();
	~GSCodeReserve();

	size_t GetMemoryUsed() const { return m_memory_used; }

	void Assign(VirtualMemoryManagerPtr allocator);
	void Reset();

	u8* Reserve(size_t size);
	void Commit(size_t size);

private:
	size_t m_memory_used = 0;
};


// --------------------------------------------------------------------------------------
//  SysMainMemory
// --------------------------------------------------------------------------------------
// This class provides the main memory for the virtual machines.
class SysMainMemory final
{
protected:
	const VirtualMemoryManagerPtr m_mainMemory;
	const VirtualMemoryManagerPtr m_codeMemory;

	VirtualMemoryBumpAllocator m_bumpAllocator;

	eeMemoryReserve m_ee;
	iopMemoryReserve m_iop;
	vuMemoryReserve m_vu;

	GSCodeReserve m_gs_code;

public:
	SysMainMemory();
	~SysMainMemory();

	const VirtualMemoryManagerPtr& MainMemory() { return m_mainMemory; }
	const VirtualMemoryManagerPtr& CodeMemory() { return m_codeMemory; }

	VirtualMemoryBumpAllocator& BumpAllocator() { return m_bumpAllocator; }

	const eeMemoryReserve& EEMemory() const { return m_ee; }
	const iopMemoryReserve& IOPMemory() const { return m_iop; }
	const vuMemoryReserve& VUMemory() const { return m_vu; }

	GSCodeReserve& GSCode() { return m_gs_code; }

	bool Allocate();
	void Reset();
	void Release();
};

// --------------------------------------------------------------------------------------
//  SysCpuProviderPack
// --------------------------------------------------------------------------------------
class SysCpuProviderPack
{
public:
	SysCpuProviderPack();
	~SysCpuProviderPack();

	void ApplyConfig() const;
};

// GetCpuProviders - this function is not implemented by PCSX2 core -- it must be
// implemented by the provisioning interface.
extern SysCpuProviderPack& GetCpuProviders();

extern void SysLogMachineCaps();		// Detects cpu type and fills cpuInfo structs.
extern void SysClearExecutionCache();	// clears recompiled execution caches!

extern std::string SysGetBiosDiscID();
extern std::string SysGetDiscID();

extern SysMainMemory& GetVmMemory();

extern void SetCPUState(SSE_MXCSR sseMXCSR, SSE_MXCSR sseVU0MXCSR, SSE_MXCSR sseVU1MXCSR);
extern SSE_MXCSR g_sseVU0MXCSR, g_sseVU1MXCSR, g_sseMXCSR;
