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

#include "MemoryTypes.h"
#include "SingleRegisterTypes.h"

#include "common/PageFaultSource.h"

static const uptr VTLB_AllocUpperBounds = _1gb * 2;

// Specialized function pointers for each read type
typedef  mem8_t vtlbMemR8FP(u32 addr);
typedef  mem16_t vtlbMemR16FP(u32 addr);
typedef  mem32_t vtlbMemR32FP(u32 addr);
typedef  mem64_t vtlbMemR64FP(u32 addr);
typedef  RETURNS_R128 vtlbMemR128FP(u32 addr);

// Specialized function pointers for each write type
typedef  void vtlbMemW8FP(u32 addr,mem8_t data);
typedef  void vtlbMemW16FP(u32 addr,mem16_t data);
typedef  void vtlbMemW32FP(u32 addr,mem32_t data);
typedef  void vtlbMemW64FP(u32 addr,mem64_t data);
typedef  void TAKES_R128 vtlbMemW128FP(u32 addr,r128 data);

template <size_t Width, bool Write> struct vtlbMemFP;

template<> struct vtlbMemFP<  8, false> { typedef vtlbMemR8FP   fn; static const uptr Index = 0; };
template<> struct vtlbMemFP< 16, false> { typedef vtlbMemR16FP  fn; static const uptr Index = 1; };
template<> struct vtlbMemFP< 32, false> { typedef vtlbMemR32FP  fn; static const uptr Index = 2; };
template<> struct vtlbMemFP< 64, false> { typedef vtlbMemR64FP  fn; static const uptr Index = 3; };
template<> struct vtlbMemFP<128, false> { typedef vtlbMemR128FP fn; static const uptr Index = 4; };
template<> struct vtlbMemFP<  8,  true> { typedef vtlbMemW8FP   fn; static const uptr Index = 0; };
template<> struct vtlbMemFP< 16,  true> { typedef vtlbMemW16FP  fn; static const uptr Index = 1; };
template<> struct vtlbMemFP< 32,  true> { typedef vtlbMemW32FP  fn; static const uptr Index = 2; };
template<> struct vtlbMemFP< 64,  true> { typedef vtlbMemW64FP  fn; static const uptr Index = 3; };
template<> struct vtlbMemFP<128,  true> { typedef vtlbMemW128FP fn; static const uptr Index = 4; };

typedef u32 vtlbHandler;

extern void vtlb_Core_Alloc();
extern void vtlb_Core_Free();
extern void vtlb_Alloc_Ppmap();
extern void vtlb_Init();
extern void vtlb_Shutdown();
extern void vtlb_Reset();
extern void vtlb_ResetFastmem();

extern vtlbHandler vtlb_NewHandler();

extern vtlbHandler vtlb_RegisterHandler(
	vtlbMemR8FP* r8,vtlbMemR16FP* r16,vtlbMemR32FP* r32,vtlbMemR64FP* r64,vtlbMemR128FP* r128,
	vtlbMemW8FP* w8,vtlbMemW16FP* w16,vtlbMemW32FP* w32,vtlbMemW64FP* w64,vtlbMemW128FP* w128
);

extern void vtlb_ReassignHandler( vtlbHandler rv,
	vtlbMemR8FP* r8,vtlbMemR16FP* r16,vtlbMemR32FP* r32,vtlbMemR64FP* r64,vtlbMemR128FP* r128,
	vtlbMemW8FP* w8,vtlbMemW16FP* w16,vtlbMemW32FP* w32,vtlbMemW64FP* w64,vtlbMemW128FP* w128
);


extern void vtlb_MapHandler(vtlbHandler handler,u32 start,u32 size);
extern void vtlb_MapBlock(void* base,u32 start,u32 size,u32 blocksize=0);
extern void* vtlb_GetPhyPtr(u32 paddr);
//extern void vtlb_Mirror(u32 new_region,u32 start,u32 size); // -> not working yet :(
extern u32  vtlb_V2P(u32 vaddr);
extern void vtlb_DynV2P();

//virtual mappings
extern void vtlb_VMap(u32 vaddr,u32 paddr,u32 sz);
extern void vtlb_VMapBuffer(u32 vaddr,void* buffer,u32 sz);
extern void vtlb_VMapUnmap(u32 vaddr,u32 sz);
extern bool vtlb_ResolveFastmemMapping(uptr* addr);
extern bool vtlb_GetGuestAddress(uptr host_addr, u32* guest_addr);
extern void vtlb_UpdateFastmemProtection(u32 paddr, u32 size, const PageProtectionMode& prot);
extern bool vtlb_BackpatchLoadStore(uptr code_address, uptr fault_address);

extern void vtlb_ClearLoadStoreInfo();
extern void vtlb_AddLoadStoreInfo(uptr code_address, u32 code_size, u32 guest_pc, u32 gpr_bitmask, u32 fpr_bitmask, u8 address_register, u8 data_register, u8 size_in_bits, bool is_signed, bool is_load, bool is_fpr);
extern void vtlb_DynBackpatchLoadStore(uptr code_address, u32 code_size, u32 guest_pc, u32 guest_addr, u32 gpr_bitmask, u32 fpr_bitmask, u8 address_register, u8 data_register, u8 size_in_bits, bool is_signed, bool is_load, bool is_fpr);
extern bool vtlb_IsFaultingPC(u32 guest_pc);

//Memory functions

template< typename DataType >
extern DataType vtlb_memRead(u32 mem);
extern RETURNS_R128 vtlb_memRead128(u32 mem);

template< typename DataType >
extern void vtlb_memWrite(u32 mem, DataType value);
extern void TAKES_R128 vtlb_memWrite128(u32 mem, r128 value);

// "Safe" variants of vtlb, designed for external tools.
// These routines only access the various RAM, and will not call handlers
// which has the potential to change hardware state.
template <typename DataType>
extern DataType vtlb_ramRead(u32 mem);
template <typename DataType>
extern bool vtlb_ramWrite(u32 mem, const DataType& value);

using vtlb_ReadRegAllocCallback = int(*)();
extern int vtlb_DynGenReadNonQuad(u32 bits, bool sign, bool xmm, int addr_reg, vtlb_ReadRegAllocCallback dest_reg_alloc = nullptr);
extern int vtlb_DynGenReadNonQuad_Const(u32 bits, bool sign, bool xmm, u32 addr_const, vtlb_ReadRegAllocCallback dest_reg_alloc = nullptr);
extern int vtlb_DynGenReadQuad(u32 bits, int addr_reg, vtlb_ReadRegAllocCallback dest_reg_alloc = nullptr);
extern int vtlb_DynGenReadQuad_Const(u32 bits, u32 addr_const, vtlb_ReadRegAllocCallback dest_reg_alloc = nullptr);

extern void vtlb_DynGenWrite(u32 sz, bool xmm, int addr_reg, int value_reg);
extern void vtlb_DynGenWrite_Const(u32 bits, bool xmm, u32 addr_const, int value_reg);

// --------------------------------------------------------------------------------------
//  VtlbMemoryReserve
// --------------------------------------------------------------------------------------
class VtlbMemoryReserve : public VirtualMemoryReserve
{
public:
	VtlbMemoryReserve(std::string name);

	void Assign(VirtualMemoryManagerPtr allocator, size_t offset, size_t size);

	virtual void Reset();
};

// --------------------------------------------------------------------------------------
//  eeMemoryReserve
// --------------------------------------------------------------------------------------
class eeMemoryReserve : public VtlbMemoryReserve
{
	typedef VtlbMemoryReserve _parent;

public:
	eeMemoryReserve();
	~eeMemoryReserve();

	void Assign(VirtualMemoryManagerPtr allocator);
	void Release();

	void Reset() override;
};

// --------------------------------------------------------------------------------------
//  iopMemoryReserve
// --------------------------------------------------------------------------------------
class iopMemoryReserve : public VtlbMemoryReserve
{
	typedef VtlbMemoryReserve _parent;

public:
	iopMemoryReserve();
	~iopMemoryReserve();

	void Assign(VirtualMemoryManagerPtr allocator);
	void Release();

	void Reset() override;
};

// --------------------------------------------------------------------------------------
//  vuMemoryReserve
// --------------------------------------------------------------------------------------
class vuMemoryReserve : public VtlbMemoryReserve
{
	typedef VtlbMemoryReserve _parent;

public:
	vuMemoryReserve();
	~vuMemoryReserve();

	void Assign(VirtualMemoryManagerPtr allocator);
	void Release();

	void Reset() override;
};

namespace vtlb_private
{
	static const uint VTLB_PAGE_BITS = 12;
	static const uint VTLB_PAGE_MASK = 4095;
	static const uint VTLB_PAGE_SIZE = 4096;

	static const uint VTLB_PMAP_SZ		= _1mb * 512;
	static const uint VTLB_PMAP_ITEMS	= VTLB_PMAP_SZ / VTLB_PAGE_SIZE;
	static const uint VTLB_VMAP_ITEMS	= _4gb / VTLB_PAGE_SIZE;

	static const uint VTLB_HANDLER_ITEMS = 128;

	static const uptr POINTER_SIGN_BIT = 1ULL << (sizeof(uptr) * 8 - 1);

	struct VTLBPhysical
	{
	private:
		sptr value;
		explicit VTLBPhysical(sptr value): value(value) { }
	public:
		VTLBPhysical(): value(0) {}
		/// Create from a pointer to raw memory
		static VTLBPhysical fromPointer(void *ptr) { return fromPointer((sptr)ptr); }
		/// Create from an integer representing a pointer to raw memory
		static VTLBPhysical fromPointer(sptr ptr);
		/// Create from a handler and address
		static VTLBPhysical fromHandler(vtlbHandler handler);

		/// Get the raw value held by the entry
		uptr raw() const { return value; }
		/// Returns whether or not this entry is a handler
		bool isHandler() const { return value < 0; }
		/// Assumes the entry is a pointer, giving back its value
		uptr assumePtr() const { return value; }
		/// Assumes the entry is a handler, and gets the raw handler ID
		u8 assumeHandler() const { return value; }
	};

	struct VTLBVirtual
	{
	private:
		uptr value;
		explicit VTLBVirtual(uptr value): value(value) { }
	public:
		VTLBVirtual(): value(0) {}
		VTLBVirtual(VTLBPhysical phys, u32 paddr, u32 vaddr);
		static VTLBVirtual fromPointer(uptr ptr, u32 vaddr) {
			return VTLBVirtual(VTLBPhysical::fromPointer(ptr), 0, vaddr);
		}

		/// Get the raw value held by the entry
		uptr raw() const { return value; }
		/// Returns whether or not this entry is a handler
		bool isHandler(u32 vaddr) const { return (sptr)(value + vaddr) < 0; }
		/// Assumes the entry is a pointer, giving back its value
		uptr assumePtr(u32 vaddr) const { return value + vaddr; }
		/// Assumes the entry is a handler, and gets the raw handler ID
		u8 assumeHandlerGetID() const { return value; }
		/// Assumes the entry is a handler, and gets the physical address
		u32 assumeHandlerGetPAddr(u32 vaddr) const { return (value + vaddr - assumeHandlerGetID()) & ~POINTER_SIGN_BIT; }
		/// Assumes the entry is a handler, returning it as a void*
		void *assumeHandlerGetRaw(int index, bool write) const;
		/// Assumes the entry is a handler, returning it
		template <size_t Width, bool Write>
		typename vtlbMemFP<Width, Write>::fn *assumeHandler() const;
	};

	struct MapData
	{
		// first indexer -- 8/16/32/64/128 bit tables [values 0-4]
		// second indexer -- read/write  [0 or 1]
		// third indexer -- 128 possible handlers!
		void* RWFT[5][2][VTLB_HANDLER_ITEMS];

		VTLBPhysical pmap[VTLB_PMAP_ITEMS]; //512KB // PS2 physical to x86 physical

		VTLBVirtual* vmap;                //4MB (allocated by vtlb_init) // PS2 virtual to x86 physical

		u32* ppmap;               //4MB (allocated by vtlb_init) // PS2 virtual to PS2 physical

		uptr fastmem_base;

		MapData()
		{
			vmap = NULL;
			ppmap = NULL;
			fastmem_base = 0;
		}
	};

	alignas(64) extern MapData vtlbdata;

	inline void *VTLBVirtual::assumeHandlerGetRaw(int index, bool write) const
	{
		return vtlbdata.RWFT[index][write][assumeHandlerGetID()];
	}

	template <size_t Width, bool Write>
	typename vtlbMemFP<Width, Write>::fn *VTLBVirtual::assumeHandler() const
	{
		using FP = vtlbMemFP<Width, Write>;
		return (typename FP::fn *)assumeHandlerGetRaw(FP::Index, Write);
	}
}

// --------------------------------------------------------------------------------------
//  Goemon game fix
// --------------------------------------------------------------------------------------
struct GoemonTlb {
	u32 valid;
	u32 unk1; // could be physical address also
	u32 unk2;
	u32 low_add;
	u32 physical_add;
	u32 unk3; // likely the size
	u32 high_add;
	u32 key; // uniq number attached to an allocation
	u32 unk5;
};
