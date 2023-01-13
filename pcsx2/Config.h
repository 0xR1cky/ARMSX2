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

#pragma once

#include "common/emitter/tools.h"
#include "common/General.h"
#include <array>
#include <string>
#include <vector>

class SettingsInterface;
class SettingsWrapper;

enum class CDVD_SourceType : uint8_t;

/// Generic setting information which can be reused in multiple components.
struct SettingInfo
{
	using GetOptionsCallback = std::vector<std::pair<std::string, std::string>>(*)();

	enum class Type
	{
		Boolean,
		Integer,
		IntegerList,
		Float,
		String,
		StringList,
		Path,
	};

	Type type;
	const char* name;
	const char* display_name;
	const char* description;
	const char* default_value;
	const char* min_value;
	const char* max_value;
	const char* step_value;
	const char* format;
	const char* const* options; // For integer lists.
	GetOptionsCallback get_options; // For string lists.
	float multiplier;

	const char* StringDefaultValue() const;
	bool BooleanDefaultValue() const;
	s32 IntegerDefaultValue() const;
	s32 IntegerMinValue() const;
	s32 IntegerMaxValue() const;
	s32 IntegerStepValue() const;
	float FloatDefaultValue() const;
	float FloatMinValue() const;
	float FloatMaxValue() const;
	float FloatStepValue() const;
};

enum class GenericInputBinding : u8;

// TODO(Stenzek): Move to InputCommon.h or something?
struct InputBindingInfo
{
	enum class Type : u8
	{
		Unknown,
		Button,
		Axis,
		HalfAxis,
		Motor,
		Pointer, // Receive relative mouse movement events, bind_index is offset by the axis.
		Keyboard, // Receive host key events, bind_index is offset by the key code.
		Device, // Used for special-purpose device selection, e.g. force feedback.
		Macro,
	};

	const char* name;
	const char* display_name;
	Type bind_type;
	u16 bind_index;
	GenericInputBinding generic_mapping;
};

/// Generic input bindings. These roughly match a DualShock 4 or XBox One controller.
/// They are used for automatic binding to PS2 controller types, and for big picture mode navigation.
enum class GenericInputBinding : u8
{
	Unknown,

	DPadUp,
	DPadRight,
	DPadLeft,
	DPadDown,

	LeftStickUp,
	LeftStickRight,
	LeftStickDown,
	LeftStickLeft,
	L3,

	RightStickUp,
	RightStickRight,
	RightStickDown,
	RightStickLeft,
	R3,

	Triangle, // Y on XBox pads.
	Circle, // B on XBox pads.
	Cross, // A on XBox pads.
	Square, // X on XBox pads.

	Select, // Share on DS4, View on XBox pads.
	Start, // Options on DS4, Menu on XBox pads.
	System, // PS button on DS4, Guide button on XBox pads.

	L1, // LB on Xbox pads.
	L2, // Left trigger on XBox pads.
	R1, // RB on XBox pads.
	R2, // Right trigger on Xbox pads.

	SmallMotor, // High frequency vibration.
	LargeMotor, // Low frequency vibration.

	Count,
};

enum GamefixId
{
	GamefixId_FIRST = 0,

	Fix_FpuMultiply = GamefixId_FIRST,
	Fix_FpuNegDiv,
	Fix_GoemonTlbMiss,
	Fix_SoftwareRendererFMV,
	Fix_SkipMpeg,
	Fix_OPHFlag,
	Fix_EETiming,
	Fix_InstantDMA,
	Fix_DMABusy,
	Fix_GIFFIFO,
	Fix_VIFFIFO,
	Fix_VIF1Stall,
	Fix_VuAddSub,
	Fix_Ibit,
	Fix_VUSync,
	Fix_VUOverflow,
	Fix_XGKick,
	Fix_BlitInternalFPS,
	Fix_FullVU0Sync,

	GamefixId_COUNT
};

// TODO - config - not a fan of the excessive use of enums and macros to make them work
// a proper object would likely make more sense (if possible).

enum SpeedhackId
{
	SpeedhackId_FIRST = 0,

	Speedhack_mvuFlag = SpeedhackId_FIRST,
	Speedhack_InstantVU1,
	Speedhack_MTVU,

	SpeedhackId_COUNT
};

enum class VsyncMode
{
	Off,
	On,
	Adaptive,
};

enum class AspectRatioType : u8
{
	Stretch,
	RAuto4_3_3_2,
	R4_3,
	R16_9,
	MaxCount
};

enum class FMVAspectRatioSwitchType : u8
{
	Off,
	RAuto4_3_3_2,
	R4_3,
	R16_9,
	MaxCount
};

enum class MemoryCardType
{
	Empty,
	File,
	Folder,
	MaxCount
};

enum class MemoryCardFileType
{
	Unknown,
	PS2_8MB,
	PS2_16MB,
	PS2_32MB,
	PS2_64MB,
	PS1,
	MaxCount
};

enum class LimiterModeType : u8
{
	Nominal,
	Turbo,
	Slomo,
	Unlimited,
};

enum class GSRendererType : s8
{
	Auto = -1,
	DX11 = 3,
	Null = 11,
	OGL = 12,
	SW = 13,
	VK = 14,
	Metal = 17,
	DX12 = 15,
};

enum class GSInterlaceMode : u8
{
	Automatic,
	Off,
	WeaveTFF,
	WeaveBFF,
	BobTFF,
	BobBFF,
	BlendTFF,
	BlendBFF,
	AdaptiveTFF,
	AdaptiveBFF,
	Count
};

enum class GSPostBilinearMode : u8
{
	Off,
	BilinearSmooth,
	BilinearSharp,
};

// Ordering was done to keep compatibility with older ini file.
enum class BiFiltering : u8
{
	Nearest,
	Forced,
	PS2,
	Forced_But_Sprite,
};

enum class TriFiltering : s8
{
	Automatic = -1,
	Off,
	PS2,
	Forced,
};

enum class HWMipmapLevel : s8
{
	Automatic = -1,
	Off,
	Basic,
	Full
};

enum class CRCHackLevel : s8
{
	Automatic = -1,
	Off,
	Minimum,
	Partial,
	Full,
	Aggressive
};

enum class AccBlendLevel : u8
{
	Minimum,
	Basic,
	Medium,
	High,
	Full,
	Maximum,
};

enum class TexturePreloadingLevel : u8
{
	Off,
	Partial,
	Full,
};

enum class GSScreenshotSize : u8
{
	WindowResolution,
	InternalResolution,
	InternalResolutionUncorrected,
};

enum class GSScreenshotFormat : u8
{
	PNG,
	JPEG,
	Count,
};

enum class GSDumpCompressionMethod : u8
{
	Uncompressed,
	LZMA,
	Zstandard,
};

enum class GSHardwareDownloadMode : u8
{
	Enabled,
	NoReadbacks,
	Unsynchronized,
	Disabled
};

enum class GSCASMode : u8
{
	Disabled,
	SharpenOnly,
	SharpenAndResize,
};

enum class GSGPUTargetCLUTMode : u8
{
	Disabled,
	Enabled,
	InsideTarget,
};

// Template function for casting enumerations to their underlying type
template <typename Enumeration>
typename std::underlying_type<Enumeration>::type enum_cast(Enumeration E)
{
	return static_cast<typename std::underlying_type<Enumeration>::type>(E);
}

ImplementEnumOperators(GamefixId);
ImplementEnumOperators(SpeedhackId);

//------------ DEFAULT sseMXCSR VALUES ---------------
#define DEFAULT_sseMXCSR 0xffc0 //FPU rounding > DaZ, FtZ, "chop"
#define DEFAULT_sseVUMXCSR 0xffc0 //VU  rounding > DaZ, FtZ, "chop"
#define SYSTEM_sseMXCSR 0x1f80

// --------------------------------------------------------------------------------------
//  TraceFiltersEE
// --------------------------------------------------------------------------------------
struct TraceFiltersEE
{
	BITFIELD32()
	bool
		m_EnableAll : 1, // Master Enable switch (if false, no logs at all)
		m_EnableDisasm : 1,
		m_EnableRegisters : 1,
		m_EnableEvents : 1; // Enables logging of event-driven activity -- counters, DMAs, etc.
	BITFIELD_END

	TraceFiltersEE()
	{
		bitset = 0;
	}

	bool operator==(const TraceFiltersEE& right) const
	{
		return OpEqu(bitset);
	}

	bool operator!=(const TraceFiltersEE& right) const
	{
		return !this->operator==(right);
	}
};

// --------------------------------------------------------------------------------------
//  TraceFiltersIOP
// --------------------------------------------------------------------------------------
struct TraceFiltersIOP
{
	BITFIELD32()
	bool
		m_EnableAll : 1, // Master Enable switch (if false, no logs at all)
		m_EnableDisasm : 1,
		m_EnableRegisters : 1,
		m_EnableEvents : 1; // Enables logging of event-driven activity -- counters, DMAs, etc.
	BITFIELD_END

	TraceFiltersIOP()
	{
		bitset = 0;
	}

	bool operator==(const TraceFiltersIOP& right) const
	{
		return OpEqu(bitset);
	}

	bool operator!=(const TraceFiltersIOP& right) const
	{
		return !this->operator==(right);
	}
};

// --------------------------------------------------------------------------------------
//  TraceLogFilters
// --------------------------------------------------------------------------------------
struct TraceLogFilters
{
	// Enabled - global toggle for high volume logging.  This is effectively the equivalent to
	// (EE.Enabled() || IOP.Enabled() || SIF) -- it's cached so that we can use the macros
	// below to inline the conditional check.  This is desirable because these logs are
	// *very* high volume, and debug builds get noticably slower if they have to invoke
	// methods/accessors to test the log enable bits.  Debug builds are slow enough already,
	// so I prefer this to help keep them usable.
	bool Enabled;

	TraceFiltersEE EE;
	TraceFiltersIOP IOP;

	TraceLogFilters()
	{
		Enabled = false;
	}

	void LoadSave(SettingsWrapper& ini);

	bool operator==(const TraceLogFilters& right) const
	{
		return OpEqu(Enabled) && OpEqu(EE) && OpEqu(IOP);
	}

	bool operator!=(const TraceLogFilters& right) const
	{
		return !this->operator==(right);
	}
};

// --------------------------------------------------------------------------------------
//  Pcsx2Config class
// --------------------------------------------------------------------------------------
// This is intended to be a public class library between the core emulator and GUI only.
//
// When GUI code performs modifications of this class, it must be done with strict thread
// safety, since the emu runs on a separate thread.  Additionally many components of the
// class require special emu-side resets or state save/recovery to be applied.  Please
// use the provided functions to lock the emulation into a safe state and then apply
// chances on the necessary scope (see Core_Pause, Core_ApplySettings, and Core_Resume).
//
struct Pcsx2Config
{
	struct ProfilerOptions
	{
		BITFIELD32()
		bool
			Enabled : 1, // universal toggle for the profiler.
			RecBlocks_EE : 1, // Enables per-block profiling for the EE recompiler [unimplemented]
			RecBlocks_IOP : 1, // Enables per-block profiling for the IOP recompiler [unimplemented]
			RecBlocks_VU0 : 1, // Enables per-block profiling for the VU0 recompiler [unimplemented]
			RecBlocks_VU1 : 1; // Enables per-block profiling for the VU1 recompiler [unimplemented]
		BITFIELD_END

		// Default is Disabled, with all recs enabled underneath.
		ProfilerOptions()
			: bitset(0xfffffffe)
		{
		}
		void LoadSave(SettingsWrapper& wrap);

		bool operator==(const ProfilerOptions& right) const
		{
			return OpEqu(bitset);
		}

		bool operator!=(const ProfilerOptions& right) const
		{
			return !OpEqu(bitset);
		}
	};

	// ------------------------------------------------------------------------
	struct RecompilerOptions
	{
		BITFIELD32()
		bool
			EnableEE : 1,
			EnableIOP : 1,
			EnableVU0 : 1,
			EnableVU1 : 1;

		bool
			vu0Overflow : 1,
			vu0ExtraOverflow : 1,
			vu0SignOverflow : 1,
			vu0Underflow : 1;

		bool
			vu1Overflow : 1,
			vu1ExtraOverflow : 1,
			vu1SignOverflow : 1,
			vu1Underflow : 1;

		bool
			fpuOverflow : 1,
			fpuExtraOverflow : 1,
			fpuFullMode : 1;

		bool
			EnableEECache : 1;
		bool
			EnableFastmem : 1;
		BITFIELD_END

		RecompilerOptions();
		void ApplySanityCheck();

		void LoadSave(SettingsWrapper& wrap);

		bool operator==(const RecompilerOptions& right) const
		{
			return OpEqu(bitset);
		}

		bool operator!=(const RecompilerOptions& right) const
		{
			return !OpEqu(bitset);
		}

		u32 GetEEClampMode() const
		{
			return fpuFullMode ? 3 : (fpuExtraOverflow ? 2 : (fpuOverflow ? 1 : 0));
		}

		void SetEEClampMode(u32 value)
		{
			fpuOverflow = (value >= 1);
			fpuExtraOverflow = (value >= 2);
			fpuFullMode = (value >= 3);
		}

		u32 GetVUClampMode() const
		{
			return vu0SignOverflow ? 3 : (vu0ExtraOverflow ? 2 : (vu0Overflow ? 1 : 0));
		}
	};

	// ------------------------------------------------------------------------
	struct CpuOptions
	{
		RecompilerOptions Recompiler;

		SSE_MXCSR sseMXCSR;
		SSE_MXCSR sseVU0MXCSR;
		SSE_MXCSR sseVU1MXCSR;

		u32 AffinityControlMode;

		CpuOptions();
		void LoadSave(SettingsWrapper& wrap);
		void ApplySanityCheck();

		bool CpusChanged(const CpuOptions& right) const;

		bool operator==(const CpuOptions& right) const
		{
			return OpEqu(sseMXCSR) && OpEqu(sseVU0MXCSR) && OpEqu(sseVU1MXCSR) && OpEqu(AffinityControlMode) && OpEqu(Recompiler);
		}

		bool operator!=(const CpuOptions& right) const
		{
			return !this->operator==(right);
		}
	};

	// ------------------------------------------------------------------------
	struct GSOptions
	{
		static const char* AspectRatioNames[];
		static const char* FMVAspectRatioSwitchNames[];
		static const char* VideoCaptureContainers[];

		static const char* GetRendererName(GSRendererType type);

		static constexpr float DEFAULT_FRAME_RATE_NTSC = 59.94f;
		static constexpr float DEFAULT_FRAME_RATE_PAL = 50.00f;

		static constexpr u32 DEFAULT_VIDEO_CAPTURE_BITRATE = 6000;
		static const char* DEFAULT_VIDEO_CAPTURE_CONTAINER;

		union
		{
			u64 bitset;

			struct
			{
				bool
					PCRTCAntiBlur : 1,
					DisableInterlaceOffset : 1,
					PCRTCOffsets : 1,
					PCRTCOverscan : 1,
					IntegerScaling : 1,
					SyncToHostRefreshRate : 1,
					UseDebugDevice : 1,
					UseBlitSwapChain : 1,
					DisableShaderCache : 1,
					DisableDualSourceBlend : 1,
					DisableFramebufferFetch : 1,
					DisableThreadedPresentation : 1,
					SkipDuplicateFrames : 1,
					OsdShowMessages : 1,
					OsdShowSpeed : 1,
					OsdShowFPS : 1,
					OsdShowCPU : 1,
					OsdShowGPU : 1,
					OsdShowResolution : 1,
					OsdShowGSStats : 1,
					OsdShowIndicators : 1,
					OsdShowSettings : 1,
					OsdShowInputs : 1,
					OsdShowFrameTimes : 1;

				bool
					HWSpinGPUForReadbacks : 1,
					HWSpinCPUForReadbacks : 1,
					GPUPaletteConversion : 1,
					AutoFlushSW : 1,
					PreloadFrameWithGSData : 1,
					WrapGSMem : 1,
					Mipmap : 1,
					ManualUserHacks : 1,
					UserHacks_AlignSpriteX : 1,
					UserHacks_AutoFlush : 1,
					UserHacks_CPUFBConversion : 1,
					UserHacks_DisableDepthSupport : 1,
					UserHacks_DisablePartialInvalidation : 1,
					UserHacks_DisableSafeFeatures : 1,
					UserHacks_MergePPSprite : 1,
					UserHacks_WildHack : 1,
					UserHacks_TextureInsideRt : 1,
					FXAA : 1,
					ShadeBoost : 1,
					DumpGSData : 1,
					SaveRT : 1,
					SaveFrame : 1,
					SaveTexture : 1,
					SaveDepth : 1,
					DumpReplaceableTextures : 1,
					DumpReplaceableMipmaps : 1,
					DumpTexturesWithFMVActive : 1,
					DumpDirectTextures : 1,
					DumpPaletteTextures : 1,
					LoadTextureReplacements : 1,
					LoadTextureReplacementsAsync : 1,
					PrecacheTextureReplacements : 1;
			};
		};

		int VsyncQueueSize{2};

		// forces the MTGS to execute tags/tasks in fully blocking/synchronous
		// style. Useful for debugging potential bugs in the MTGS pipeline.
		bool SynchronousMTGS{false};
		bool FrameLimitEnable{true};

		VsyncMode VsyncEnable{VsyncMode::Off};

		float LimitScalar{1.0f};
		float FramerateNTSC{DEFAULT_FRAME_RATE_NTSC};
		float FrameratePAL{DEFAULT_FRAME_RATE_PAL};

		AspectRatioType AspectRatio{AspectRatioType::RAuto4_3_3_2};
		FMVAspectRatioSwitchType FMVAspectRatioSwitch{FMVAspectRatioSwitchType::Off};
		GSInterlaceMode InterlaceMode{GSInterlaceMode::Automatic};
		GSPostBilinearMode LinearPresent{ GSPostBilinearMode::BilinearSmooth };

		float StretchY{100.0f};
		int Crop[4]{};

		float OsdScale{100.0};

		GSRendererType Renderer{GSRendererType::Auto};
		float UpscaleMultiplier{1.0f};

		HWMipmapLevel HWMipmap{HWMipmapLevel::Automatic};
		AccBlendLevel AccurateBlendingUnit{AccBlendLevel::Basic};
		CRCHackLevel CRCHack{CRCHackLevel::Automatic};
		BiFiltering TextureFiltering{BiFiltering::PS2};
		TexturePreloadingLevel TexturePreloading{TexturePreloadingLevel::Full};
		GSDumpCompressionMethod GSDumpCompression{GSDumpCompressionMethod::Zstandard};
		GSHardwareDownloadMode HWDownloadMode{GSHardwareDownloadMode::Enabled};
		GSCASMode CASMode{GSCASMode::Disabled};
		int Dithering{2};
		int MaxAnisotropy{0};
		int SWExtraThreads{2};
		int SWExtraThreadsHeight{4};
		int TVShader{0};
		s16 GetSkipCountFunctionId{-1};
		s16 BeforeDrawFunctionId{-1};
		int SkipDrawStart{0};
		int SkipDrawEnd{0};

		int UserHacks_HalfBottomOverride{-1};
		int UserHacks_HalfPixelOffset{0};
		int UserHacks_RoundSprite{0};
		int UserHacks_TCOffsetX{0};
		int UserHacks_TCOffsetY{0};
		int UserHacks_CPUSpriteRenderBW{0};
		int UserHacks_CPUCLUTRender{ 0 };
		GSGPUTargetCLUTMode UserHacks_GPUTargetCLUTMode{GSGPUTargetCLUTMode::Disabled};
		TriFiltering TriFilter{TriFiltering::Automatic};
		int OverrideTextureBarriers{-1};
		int OverrideGeometryShaders{-1};

		int CAS_Sharpness{50};
		int ShadeBoost_Brightness{50};
		int ShadeBoost_Contrast{50};
		int ShadeBoost_Saturation{50};
		int PNGCompressionLevel{1};

		int SaveN{0};
		int SaveL{5000};

		GSScreenshotSize ScreenshotSize{GSScreenshotSize::WindowResolution};
		GSScreenshotFormat ScreenshotFormat{GSScreenshotFormat::PNG};
		int ScreenshotQuality{50};

		std::string VideoCaptureContainer{DEFAULT_VIDEO_CAPTURE_CONTAINER};
		std::string VideoCaptureCodec;
		int VideoCaptureBitrate{DEFAULT_VIDEO_CAPTURE_BITRATE};

		std::string Adapter;
		std::string HWDumpDirectory;
		std::string SWDumpDirectory;

		GSOptions();

		void LoadSave(SettingsWrapper& wrap);

		/// Sets user hack values to defaults when user hacks are not enabled.
		void MaskUserHacks();

		/// Sets user hack values to defaults when upscaling is not enabled.
		void MaskUpscalingHacks();

		/// Returns true if any of the hardware renderers are selected.
		bool UseHardwareRenderer() const;

		/// Returns false if the compared to the old settings, we need to reopen GS.
		/// (i.e. renderer change, swap chain mode change, etc.)
		bool RestartOptionsAreEqual(const GSOptions& right) const;

		/// Returns false if any options need to be applied to the MTGS.
		bool OptionsAreEqual(const GSOptions& right) const;

		bool operator==(const GSOptions& right) const;
		bool operator!=(const GSOptions& right) const;
	};

	struct SPU2Options
	{
		enum class SynchronizationMode
		{
			TimeStretch,
			ASync,
			NoSync,
		};

		static constexpr s32 MAX_VOLUME = 200;
		
		static constexpr s32 MIN_LATENCY = 3;
		static constexpr s32 MIN_LATENCY_TIMESTRETCH = 15;
		static constexpr s32 MAX_LATENCY = 750;

		static constexpr s32 MIN_SEQUENCE_LEN = 20;
		static constexpr s32 MAX_SEQUENCE_LEN = 100;
		static constexpr s32 MIN_SEEKWINDOW = 10;
		static constexpr s32 MAX_SEEKWINDOW = 30;
		static constexpr s32 MIN_OVERLAP = 5;
		static constexpr s32 MAX_OVERLAP = 15;

		BITFIELD32()
		bool OutputLatencyMinimal : 1;
		bool
			DebugEnabled : 1,
			MsgToConsole : 1,
			MsgKeyOnOff : 1,
			MsgVoiceOff : 1,
			MsgDMA : 1,
			MsgAutoDMA : 1,
			MsgOverruns : 1,
			MsgCache : 1,
			AccessLog : 1,
			DMALog : 1,
			WaveLog : 1,
			CoresDump : 1,
			MemDump : 1,
			RegDump : 1,
			VisualDebugEnabled : 1;
		BITFIELD_END

		SynchronizationMode SynchMode = SynchronizationMode::TimeStretch;

		s32 FinalVolume = 100;
		s32 Latency = 60;
		s32 OutputLatency = 20;
		s32 SpeakerConfiguration = 0;
		s32 DplDecodingLevel = 0;

		s32 SequenceLenMS = 30;
		s32 SeekWindowMS = 20;
		s32 OverlapMS = 10;

		std::string OutputModule;
		std::string BackendName;
		std::string DeviceName;

		SPU2Options();

		void LoadSave(SettingsWrapper& wrap);

		bool operator==(const SPU2Options& right) const
		{
			return OpEqu(bitset) &&

				OpEqu(SynchMode) &&

				OpEqu(FinalVolume) &&
				OpEqu(Latency) &&
				OpEqu(OutputLatency) &&
				OpEqu(SpeakerConfiguration) &&
				OpEqu(DplDecodingLevel) &&

				OpEqu(SequenceLenMS) &&
				OpEqu(SeekWindowMS) &&
				OpEqu(OverlapMS) &&

				OpEqu(OutputModule) &&
				OpEqu(BackendName) &&
				OpEqu(DeviceName);
		}

		bool operator!=(const SPU2Options& right) const
		{
			return !this->operator==(right);
		}
	};

	struct DEV9Options
	{
		enum struct NetApi : int
		{
			Unset = 0,
			PCAP_Bridged = 1,
			PCAP_Switched = 2,
			TAP = 3,
			Sockets = 4,
		};
		static const char* NetApiNames[];

		enum struct DnsMode : int
		{
			Manual = 0,
			Auto = 1,
			Internal = 2,
		};
		static const char* DnsModeNames[];

		struct HostEntry
		{
			std::string Url;
			std::string Desc;
			u8 Address[4]{};
			bool Enabled;

			bool operator==(const HostEntry& right) const
			{
				return OpEqu(Url) &&
					   OpEqu(Desc) &&
					   (*(int*)Address == *(int*)right.Address) &&
					   OpEqu(Enabled);
			}

			bool operator!=(const HostEntry& right) const
			{
				return !this->operator==(right);
			}
		};

		bool EthEnable{false};
		NetApi EthApi{NetApi::Unset};
		std::string EthDevice;
		bool EthLogDNS{false};

		bool InterceptDHCP{false};
		u8 PS2IP[4]{};
		u8 Mask[4]{};
		u8 Gateway[4]{};
		u8 DNS1[4]{};
		u8 DNS2[4]{};
		bool AutoMask{true};
		bool AutoGateway{true};
		DnsMode ModeDNS1{DnsMode::Auto};
		DnsMode ModeDNS2{DnsMode::Auto};

		std::vector<HostEntry> EthHosts;

		bool HddEnable{false};
		std::string HddFile;

		/* The PS2's HDD max size is 2TB
		 * which is 2^32 * 512 byte sectors
		 * Note that we don't yet support
		 * 48bit LBA, so our limit is lower */
		uint HddSizeSectors{40 * (1024 * 1024 * 1024 / 512)};

		DEV9Options();

		void LoadSave(SettingsWrapper& wrap);

		bool operator==(const DEV9Options& right) const
		{
			return OpEqu(EthEnable) &&
				   OpEqu(EthApi) &&
				   OpEqu(EthDevice) &&
				   OpEqu(EthLogDNS) &&

				   OpEqu(InterceptDHCP) &&
				   (*(int*)PS2IP == *(int*)right.PS2IP) &&
				   (*(int*)Gateway == *(int*)right.Gateway) &&
				   (*(int*)DNS1 == *(int*)right.DNS1) &&
				   (*(int*)DNS2 == *(int*)right.DNS2) &&

				   OpEqu(AutoMask) &&
				   OpEqu(AutoGateway) &&
				   OpEqu(ModeDNS1) &&
				   OpEqu(ModeDNS2) &&

				   OpEqu(EthHosts) &&

				   OpEqu(HddEnable) &&
				   OpEqu(HddFile) &&
				   OpEqu(HddSizeSectors);
		}

		bool operator!=(const DEV9Options& right) const
		{
			return !this->operator==(right);
		}

	protected:
		static void LoadIPHelper(u8* field, const std::string& setting);
		static std::string SaveIPHelper(u8* field);
	};

	// ------------------------------------------------------------------------
	// NOTE: The GUI's GameFixes panel is dependent on the order of bits in this structure.
	struct GamefixOptions
	{
		BITFIELD32()
		bool
			FpuMulHack : 1, // Tales of Destiny hangs.
			FpuNegDivHack : 1, // Gundam games messed up camera-view.
			GoemonTlbHack : 1, // Gomeon tlb miss hack. The game need to access unmapped virtual address. Instead to handle it as exception, tlb are preloaded at startup
			SoftwareRendererFMVHack : 1, // Switches to software renderer for FMVs
			SkipMPEGHack : 1, // Skips MPEG videos (Katamari and other games need this)
			OPHFlagHack : 1, // Bleach Blade Battlers
			EETimingHack : 1, // General purpose timing hack.
			InstantDMAHack : 1, // Instantly complete DMA's if possible, good for cache emulation problems.
			DMABusyHack : 1, // Denies writes to the DMAC when it's busy. This is correct behaviour but bad timing can cause problems.
			GIFFIFOHack : 1, // Enabled the GIF FIFO (more correct but slower)
			VIFFIFOHack : 1, // Pretends to fill the non-existant VIF FIFO Buffer.
			VIF1StallHack : 1, // Like above, processes FIFO data before the stall is allowed (to make sure data goes over).
			VuAddSubHack : 1, // Tri-ace games, they use an encryption algorithm that requires VU ADDI opcode to be bit-accurate.
			IbitHack : 1, // I bit hack. Needed to stop constant VU recompilation in some games
			VUSyncHack : 1, // Makes microVU run behind the EE to avoid VU register reading/writing sync issues. Useful for M-Bit games
			VUOverflowHack : 1, // Tries to simulate overflow flag checks (not really possible on x86 without soft floats)
			XgKickHack : 1, // Erementar Gerad, adds more delay to VU XGkick instructions. Corrects the color of some graphics, but breaks Tri-ace games and others.
			BlitInternalFPSHack : 1, // Disables privileged register write-based FPS detection.
			FullVU0SyncHack : 1; // Forces tight VU0 sync on every COP2 instruction.
		BITFIELD_END

		GamefixOptions();
		void LoadSave(SettingsWrapper& wrap);
		GamefixOptions& DisableAll();

		bool Get(GamefixId id) const;
		void Set(GamefixId id, bool enabled = true);
		void Clear(GamefixId id) { Set(id, false); }

		bool operator==(const GamefixOptions& right) const
		{
			return OpEqu(bitset);
		}

		bool operator!=(const GamefixOptions& right) const
		{
			return !OpEqu(bitset);
		}
	};

	// ------------------------------------------------------------------------
	struct SpeedhackOptions
	{
		BITFIELD32()
		bool
			fastCDVD : 1, // enables fast CDVD access
			IntcStat : 1, // tells Pcsx2 to fast-forward through intc_stat waits.
			WaitLoop : 1, // enables constant loop detection and fast-forwarding
			vuFlagHack : 1, // microVU specific flag hack
			vuThread : 1, // Enable Threaded VU1
			vu1Instant : 1; // Enable Instant VU1 (Without MTVU only)
		BITFIELD_END

		s8 EECycleRate; // EE cycle rate selector (1.0, 1.5, 2.0)
		u8 EECycleSkip; // EE Cycle skip factor (0, 1, 2, or 3)

		SpeedhackOptions();
		void LoadSave(SettingsWrapper& conf);
		SpeedhackOptions& DisableAll();

		void Set(SpeedhackId id, bool enabled = true);

		bool operator==(const SpeedhackOptions& right) const
		{
			return OpEqu(bitset) && OpEqu(EECycleRate) && OpEqu(EECycleSkip);
		}

		bool operator!=(const SpeedhackOptions& right) const
		{
			return !this->operator==(right);
		}
	};

	struct DebugOptions
	{
		BITFIELD32()
		bool
			ShowDebuggerOnStart : 1;
		bool
			AlignMemoryWindowStart : 1;
		BITFIELD_END

		u8 FontWidth;
		u8 FontHeight;
		u32 WindowWidth;
		u32 WindowHeight;
		u32 MemoryViewBytesPerRow;

		DebugOptions();
		void LoadSave(SettingsWrapper& wrap);

		bool operator==(const DebugOptions& right) const
		{
			return OpEqu(bitset) && OpEqu(FontWidth) && OpEqu(FontHeight) && OpEqu(WindowWidth) && OpEqu(WindowHeight) && OpEqu(MemoryViewBytesPerRow);
		}

		bool operator!=(const DebugOptions& right) const
		{
			return !this->operator==(right);
		}
	};

	// ------------------------------------------------------------------------
	struct FramerateOptions
	{
		float NominalScalar{1.0f};
		float TurboScalar{2.0f};
		float SlomoScalar{0.5f};

		void LoadSave(SettingsWrapper& wrap);
		void SanityCheck();

		bool operator==(const FramerateOptions& right) const
		{
			return OpEqu(NominalScalar) && OpEqu(TurboScalar) && OpEqu(SlomoScalar);
		}

		bool operator!=(const FramerateOptions& right) const
		{
			return !this->operator==(right);
		}
	};

	// ------------------------------------------------------------------------
	struct FilenameOptions
	{
		std::string Bios;

		FilenameOptions();
		void LoadSave(SettingsWrapper& wrap);

		bool operator==(const FilenameOptions& right) const
		{
			return OpEqu(Bios);
		}

		bool operator!=(const FilenameOptions& right) const
		{
			return !this->operator==(right);
		}
	};

	// ------------------------------------------------------------------------
	struct USBOptions
	{
		enum : u32
		{
			NUM_PORTS = 2
		};

		struct Port
		{
			s32 DeviceType;
			u32 DeviceSubtype;

			bool operator==(const USBOptions::Port& right) const;
			bool operator!=(const USBOptions::Port& right) const;
		};

		std::array<Port, NUM_PORTS> Ports;

		USBOptions();
		void LoadSave(SettingsWrapper& wrap);

		bool operator==(const USBOptions& right) const;
		bool operator!=(const USBOptions& right) const;
	};

	// ------------------------------------------------------------------------
	// Options struct for each memory card.
	//
	struct McdOptions
	{
		std::string Filename; // user-configured location of this memory card
		bool Enabled; // memory card enabled (if false, memcard will not show up in-game)
		MemoryCardType Type; // the memory card implementation that should be used
	};

	// ------------------------------------------------------------------------

#ifdef ENABLE_ACHIEVEMENTS
	struct AchievementsOptions
	{
		BITFIELD32()
		bool
			Enabled : 1,
			TestMode : 1,
			UnofficialTestMode : 1,
			RichPresence : 1,
			ChallengeMode : 1,
			Leaderboards : 1,
			Notifications : 1,
			SoundEffects : 1,
			PrimedIndicators : 1;
		BITFIELD_END

		AchievementsOptions();
		void LoadSave(SettingsWrapper& wrap);

		bool operator==(const AchievementsOptions& right) const
		{
			return OpEqu(bitset);
		}

		bool operator!=(const AchievementsOptions& right) const
		{
			return !this->operator==(right);
		}
	};
#endif

	// ------------------------------------------------------------------------

	BITFIELD32()
	bool
		CdvdVerboseReads : 1, // enables cdvd read activity verbosely dumped to the console
		CdvdDumpBlocks : 1, // enables cdvd block dumping
		CdvdShareWrite : 1, // allows the iso to be modified while it's loaded
		EnablePatches : 1, // enables patch detection and application
		EnableCheats : 1, // enables cheat detection and application
		EnablePINE : 1, // enables inter-process communication
		EnableWideScreenPatches : 1,
		EnableNoInterlacingPatches : 1,
		// TODO - Vaser - where are these settings exposed in the Qt UI?
		EnableRecordingTools : 1,
		EnableGameFixes : 1, // enables automatic game fixes
		SaveStateOnShutdown : 1, // default value for saving state on shutdown
		EnableDiscordPresence : 1, // enables discord rich presence integration
		InhibitScreensaver : 1,
		// when enabled uses BOOT2 injection, skipping sony bios splashes
		UseBOOT2Injection : 1,
		BackupSavestate : 1,
		SavestateZstdCompression : 1,
		// enables simulated ejection of memory cards when loading savestates
		McdEnableEjection : 1,
		McdFolderAutoManage : 1,

		MultitapPort0_Enabled : 1,
		MultitapPort1_Enabled : 1,

		ConsoleToStdio : 1,
		HostFs : 1,

		WarnAboutUnsafeSettings : 1;

	// uses automatic ntfs compression when creating new memory cards (Win32 only)
#ifdef _WIN32
	bool McdCompressNTFS;
#endif
	BITFIELD_END

	CpuOptions Cpu;
	GSOptions GS;
	SpeedhackOptions Speedhacks;
	GamefixOptions Gamefixes;
	ProfilerOptions Profiler;
	DebugOptions Debugger;
	FramerateOptions Framerate;
	SPU2Options SPU2;
	DEV9Options DEV9;
	USBOptions USB;

	TraceLogFilters Trace;

	FilenameOptions BaseFilenames;

#ifdef ENABLE_ACHIEVEMENTS
	AchievementsOptions Achievements;
#endif

	// Memorycard options - first 2 are default slots, last 6 are multitap 1 and 2
	// slots (3 each)
	McdOptions Mcd[8];
	std::string GzipIsoIndexTemplate; // for quick-access index with gzipped ISO

	// Set at runtime, not loaded from config.
	std::string CurrentBlockdump;
	std::string CurrentIRX;
	std::string CurrentGameArgs;
	AspectRatioType CurrentAspectRatio = AspectRatioType::RAuto4_3_3_2;
	LimiterModeType LimiterMode = LimiterModeType::Nominal;

	Pcsx2Config();
	void LoadSave(SettingsWrapper& wrap);
	void LoadSaveMemcards(SettingsWrapper& wrap);

	std::string FullpathToBios() const;
	std::string FullpathToMcd(uint slot) const;

	bool MultitapEnabled(uint port) const;

	bool operator==(const Pcsx2Config& right) const;
	bool operator!=(const Pcsx2Config& right) const
	{
		return !this->operator==(right);
	}

	/// Copies runtime configuration settings (e.g. frame limiter state).
	void CopyRuntimeConfig(Pcsx2Config& cfg);
};

extern Pcsx2Config EmuConfig;

namespace EmuFolders
{
	extern std::string AppRoot;
	extern std::string DataRoot;
	extern std::string Settings;
	extern std::string Bios;
	extern std::string Snapshots;
	extern std::string Savestates;
	extern std::string MemoryCards;
	extern std::string Langs;
	extern std::string Logs;
	extern std::string Cheats;
	extern std::string CheatsWS;
	extern std::string CheatsNI;
	extern std::string Resources;
	extern std::string Cache;
	extern std::string Covers;
	extern std::string GameSettings;
	extern std::string Textures;
	extern std::string InputProfiles;

	// Assumes that AppRoot and DataRoot have been initialized.
	void SetDefaults(SettingsInterface& si);
	void LoadConfig(SettingsInterface& si);
	bool EnsureFoldersExist();

	/// Opens the specified log file for writing.
	std::FILE* OpenLogFile(const std::string_view& name, const char* mode);
} // namespace EmuFolders

/////////////////////////////////////////////////////////////////////////////////////////
// Helper Macros for Reading Emu Configurations.
//

// ------------ CPU / Recompiler Options ---------------

#define THREAD_VU1 (EmuConfig.Cpu.Recompiler.EnableVU1 && EmuConfig.Speedhacks.vuThread)
#define INSTANT_VU1 (EmuConfig.Speedhacks.vu1Instant)
#define CHECK_EEREC (EmuConfig.Cpu.Recompiler.EnableEE)
#define CHECK_CACHE (EmuConfig.Cpu.Recompiler.EnableEECache)
#define CHECK_IOPREC (EmuConfig.Cpu.Recompiler.EnableIOP)
#define CHECK_FASTMEM (EmuConfig.Cpu.Recompiler.EnableEE && EmuConfig.Cpu.Recompiler.EnableFastmem)

//------------ SPECIAL GAME FIXES!!! ---------------
#define CHECK_VUADDSUBHACK (EmuConfig.Gamefixes.VuAddSubHack) // Special Fix for Tri-ace games, they use an encryption algorithm that requires VU addi opcode to be bit-accurate.
#define CHECK_FPUMULHACK (EmuConfig.Gamefixes.FpuMulHack) // Special Fix for Tales of Destiny hangs.
#define CHECK_FPUNEGDIVHACK (EmuConfig.Gamefixes.FpuNegDivHack) // Special Fix for Gundam games messed up camera-view.
#define CHECK_XGKICKHACK (EmuConfig.Gamefixes.XgKickHack) // Special Fix for Erementar Gerad, adds more delay to VU XGkick instructions. Corrects the color of some graphics.
#define CHECK_EETIMINGHACK (EmuConfig.Gamefixes.EETimingHack) // Fix all scheduled events to happen in 1 cycle.
#define CHECK_INSTANTDMAHACK (EmuConfig.Gamefixes.InstantDMAHack) // Attempt to finish DMA's instantly, useful for games which rely on cache emulation.
#define CHECK_SKIPMPEGHACK (EmuConfig.Gamefixes.SkipMPEGHack) // Finds sceMpegIsEnd pattern to tell the game the mpeg is finished (Katamari and a lot of games need this)
#define CHECK_OPHFLAGHACK (EmuConfig.Gamefixes.OPHFlagHack) // Bleach Blade Battlers
#define CHECK_DMABUSYHACK (EmuConfig.Gamefixes.DMABusyHack) // Denies writes to the DMAC when it's busy. This is correct behaviour but bad timing can cause problems.
#define CHECK_VIFFIFOHACK (EmuConfig.Gamefixes.VIFFIFOHack) // Pretends to fill the non-existant VIF FIFO Buffer.
#define CHECK_VIF1STALLHACK (EmuConfig.Gamefixes.VIF1StallHack) // Like above, processes FIFO data before the stall is allowed (to make sure data goes over).
#define CHECK_GIFFIFOHACK (EmuConfig.Gamefixes.GIFFIFOHack) // Enabled the GIF FIFO (more correct but slower)
#define CHECK_VUOVERFLOWHACK (EmuConfig.Gamefixes.VUOverflowHack) // Special Fix for Superman Returns, they check for overflows on PS2 floats which we can't do without soft floats.
#define CHECK_FULLVU0SYNCHACK (EmuConfig.Gamefixes.FullVU0SyncHack)

//------------ Advanced Options!!! ---------------
#define CHECK_VU_OVERFLOW(vunum) (((vunum) == 0) ? EmuConfig.Cpu.Recompiler.vu0Overflow : EmuConfig.Cpu.Recompiler.vu1Overflow)
#define CHECK_VU_EXTRA_OVERFLOW(vunum) (((vunum) == 0) ? EmuConfig.Cpu.Recompiler.vu0ExtraOverflow : EmuConfig.Cpu.Recompiler.vu1ExtraOverflow) // If enabled, Operands are clamped before being used in the VU recs
#define CHECK_VU_SIGN_OVERFLOW(vunum) (((vunum) == 0) ? EmuConfig.Cpu.Recompiler.vu0SignOverflow : EmuConfig.Cpu.Recompiler.vu1SignOverflow)
#define CHECK_VU_UNDERFLOW(vunum) (((vunum) == 0) ? EmuConfig.Cpu.Recompiler.vu0Underflow : EmuConfig.Cpu.Recompiler.vu1Underflow)

#define CHECK_FPU_OVERFLOW (EmuConfig.Cpu.Recompiler.fpuOverflow)
#define CHECK_FPU_EXTRA_OVERFLOW (EmuConfig.Cpu.Recompiler.fpuExtraOverflow) // If enabled, Operands are checked for infinities before being used in the FPU recs
#define CHECK_FPU_EXTRA_FLAGS 1 // Always enabled now // Sets D/I flags on FPU instructions
#define CHECK_FPU_FULL (EmuConfig.Cpu.Recompiler.fpuFullMode)

//------------ EE Recompiler defines - Comment to disable a recompiler ---------------

#define SHIFT_RECOMPILE // Speed majorly reduced if disabled
#define BRANCH_RECOMPILE // Speed extremely reduced if disabled - more then shift

// Disabling all the recompilers in this block is interesting, as it still runs at a reasonable rate.
// It also adds a few glitches. Really reminds me of the old Linux 64-bit version. --arcum42
#define ARITHMETICIMM_RECOMPILE
#define ARITHMETIC_RECOMPILE
#define MULTDIV_RECOMPILE
#define JUMP_RECOMPILE
#define LOADSTORE_RECOMPILE
#define MOVE_RECOMPILE
#define MMI_RECOMPILE
#define MMI0_RECOMPILE
#define MMI1_RECOMPILE
#define MMI2_RECOMPILE
#define MMI3_RECOMPILE
#define FPU_RECOMPILE
#define CP0_RECOMPILE
#define CP2_RECOMPILE

// You can't recompile ARITHMETICIMM without ARITHMETIC.
#ifndef ARITHMETIC_RECOMPILE
#undef ARITHMETICIMM_RECOMPILE
#endif

#define EE_CONST_PROP 1 // rec2 - enables constant propagation (faster)

// Change to 1 for console logs of SIF, GPU (PS1 mode) and MDEC (PS1 mode).
// These do spam a lot though!
#define PSX_EXTRALOGS 0
