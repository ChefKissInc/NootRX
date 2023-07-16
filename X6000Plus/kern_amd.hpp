//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#pragma once
#include <Headers/kern_util.hpp>

using t_GenericConstructor = void (*)(void *that);

constexpr uint32_t mmHUBP0_DCSURF_ADDR_CONFIG = 0x55A;
constexpr uint32_t mmHUBP1_DCSURF_ADDR_CONFIG = 0x61E;
constexpr uint32_t mmHUBP0_DCSURF_SURFACE_CONFIG = 0x559;
constexpr uint32_t mmHUBP0_DCSURF_TILING_CONFIG = 0x55B;
constexpr uint32_t mmHUBP0_DCSURF_PRI_VIEWPORT_START = 0x55C;
constexpr uint32_t mmHUBP0_DCSURF_PRI_VIEWPORT_DIMENSION = 0x55D;
constexpr uint32_t mmHUBP2_DCSURF_ADDR_CONFIG = 0x6E2;
constexpr uint32_t mmHUBP1_DCSURF_SURFACE_CONFIG = 0x61D;
constexpr uint32_t mmHUBP1_DCSURF_TILING_CONFIG = 0x61F;
constexpr uint32_t mmHUBP1_DCSURF_PRI_VIEWPORT_START = 0x620;
constexpr uint32_t mmHUBP1_DCSURF_PRI_VIEWPORT_DIMENSION = 0x621;
constexpr uint32_t mmHUBP3_DCSURF_ADDR_CONFIG = 0x7A6;
constexpr uint32_t mmHUBP2_DCSURF_SURFACE_CONFIG = 0x6E1;
constexpr uint32_t mmHUBP2_DCSURF_TILING_CONFIG = 0x6E3;
constexpr uint32_t mmHUBP2_DCSURF_PRI_VIEWPORT_START = 0x6E4;
constexpr uint32_t mmHUBP2_DCSURF_PRI_VIEWPORT_DIMENSION = 0x6E5;
constexpr uint32_t mmHUBP3_DCSURF_SURFACE_CONFIG = 0x7A5;
constexpr uint32_t mmHUBP3_DCSURF_TILING_CONFIG = 0x7A7;
constexpr uint32_t mmHUBP3_DCSURF_PRI_VIEWPORT_START = 0x7A8;
constexpr uint32_t mmHUBP3_DCSURF_PRI_VIEWPORT_DIMENSION = 0x7A9;

constexpr uint32_t mmHUBPREQ0_DCSURF_SURFACE_PITCH = 0x57B;
constexpr uint32_t mmHUBPREQ0_DCSURF_PRIMARY_SURFACE_ADDRESS = 0x57D;
constexpr uint32_t mmHUBPREQ0_DCSURF_PRIMARY_SURFACE_ADDRESS_HIGH = 0x57E;
constexpr uint32_t mmHUBPREQ0_DCSURF_FLIP_CONTROL = 0x58E;
constexpr uint32_t mmHUBPREQ0_DCSURF_SURFACE_EARLIEST_INUSE = 0x597;
constexpr uint32_t mmHUBPREQ0_DCSURF_SURFACE_EARLIEST_INUSE_HIGH = 0x598;
constexpr uint32_t mmHUBPREQ1_DCSURF_SURFACE_PITCH = 0x63F;
constexpr uint32_t mmHUBPREQ1_DCSURF_PRIMARY_SURFACE_ADDRESS = 0x641;
constexpr uint32_t mmHUBPREQ1_DCSURF_PRIMARY_SURFACE_ADDRESS_HIGH = 0x642;
constexpr uint32_t mmHUBPREQ1_DCSURF_FLIP_CONTROL = 0x652;
constexpr uint32_t mmHUBPREQ1_DCSURF_SURFACE_EARLIEST_INUSE = 0x65B;
constexpr uint32_t mmHUBPREQ1_DCSURF_SURFACE_EARLIEST_INUSE_HIGH = 0x65C;
constexpr uint32_t mmHUBPREQ2_DCSURF_SURFACE_PITCH = 0x703;
constexpr uint32_t mmHUBPREQ2_DCSURF_PRIMARY_SURFACE_ADDRESS = 0x705;
constexpr uint32_t mmHUBPREQ2_DCSURF_PRIMARY_SURFACE_ADDRESS_HIGH = 0x706;
constexpr uint32_t mmHUBPREQ2_DCSURF_FLIP_CONTROL = 0x716;
constexpr uint32_t mmHUBPREQ2_DCSURF_SURFACE_EARLIEST_INUSE = 0x71F;
constexpr uint32_t mmHUBPREQ2_DCSURF_SURFACE_EARLIEST_INUSE_HIGH = 0x720;
constexpr uint32_t mmHUBPREQ3_DCSURF_SURFACE_PITCH = 0x7C7;
constexpr uint32_t mmHUBPREQ3_DCSURF_PRIMARY_SURFACE_ADDRESS = 0x7C9;
constexpr uint32_t mmHUBPREQ3_DCSURF_PRIMARY_SURFACE_ADDRESS_HIGH = 0x7CA;
constexpr uint32_t mmHUBPREQ3_DCSURF_FLIP_CONTROL = 0x7DA;
constexpr uint32_t mmHUBPREQ3_DCSURF_SURFACE_EARLIEST_INUSE = 0x7E3;
constexpr uint32_t mmHUBPREQ3_DCSURF_SURFACE_EARLIEST_INUSE_HIGH = 0x7E4;

constexpr uint32_t mmHUBPRET0_HUBPRET_CONTROL = 0x5E0;
constexpr uint32_t mmHUBPRET1_HUBPRET_CONTROL = 0x6A4;
constexpr uint32_t mmHUBPRET2_HUBPRET_CONTROL = 0x768;
constexpr uint32_t mmHUBPRET3_HUBPRET_CONTROL = 0x82C;

constexpr uint32_t mmOTG0_OTG_CONTROL = 0x1B41;
constexpr uint32_t mmOTG0_OTG_INTERLACE_CONTROL = 0x1B44;
constexpr uint32_t mmOTG1_OTG_CONTROL = 0x1BC1;
constexpr uint32_t mmOTG1_OTG_INTERLACE_CONTROL = 0x1BC4;
constexpr uint32_t mmOTG2_OTG_CONTROL = 0x1C41;
constexpr uint32_t mmOTG2_OTG_INTERLACE_CONTROL = 0x1C44;
constexpr uint32_t mmOTG3_OTG_CONTROL = 0x1CC1;
constexpr uint32_t mmOTG3_OTG_INTERLACE_CONTROL = 0x1CC4;
constexpr uint32_t mmOTG4_OTG_CONTROL = 0x1D41;
constexpr uint32_t mmOTG4_OTG_INTERLACE_CONTROL = 0x1D44;
constexpr uint32_t mmOTG5_OTG_CONTROL = 0x1DC1;
constexpr uint32_t mmOTG5_OTG_INTERLACE_CONTROL = 0x1DC4;

constexpr uint32_t mmPCIE_INDEX2 = 0xE;
constexpr uint32_t mmPCIE_DATA2 = 0xF;

constexpr uint32_t mmIH_CHICKEN = 0x122C;
constexpr uint32_t mmIH_MC_SPACE_GPA_ENABLE = 0x10;
constexpr uint32_t mmIH_CLK_CTRL = 0x117B;
constexpr uint32_t mmIH_IH_BUFFER_MEM_CLK_SOFT_OVERRIDE_SHIFT = 0x1A;
constexpr uint32_t mmIH_DBUS_MUX_CLK_SOFT_OVERRIDE_SHIFT = 0x1B;

constexpr uint32_t MP_BASE = 0x16000;

constexpr uint32_t AMDGPU_MAX_USEC_TIMEOUT = 100000;

constexpr uint32_t mmMP1_SMN_C2PMSG_90 = 0x29A;
constexpr uint32_t mmMP1_SMN_C2PMSG_82 = 0x292;
constexpr uint32_t mmMP1_SMN_C2PMSG_66 = 0x282;

struct CommonFirmwareHeader {
	uint32_t size;
	uint32_t headerSize;
	uint16_t headerMajor;
	uint16_t headerMinor;
	uint16_t ipMajor;
	uint16_t ipMinor;
	uint32_t ucodeVer;
	uint32_t ucodeSize;
	uint32_t ucodeOff;
	uint32_t crc32;
} PACKED;

struct GPUInfoFirmware {
	uint32_t gcNumSe;
	uint32_t gcNumCuPerSh;
	uint32_t gcNumShPerSe;
	uint32_t gcNumRbPerSe;
	uint32_t gcNumTccs;
	uint32_t gcNumGprs;
	uint32_t gcNumMaxGsThds;
	uint32_t gcGsTableDepth;
	uint32_t gcGsPrimBuffDepth;
	uint32_t gcParameterCacheDepth;
	uint32_t gcDoubleOffchipLdsBuffer;
	uint32_t gcWaveSize;
	uint32_t gcMaxWavesPerSimd;
	uint32_t gcMaxScratchSlotsPerCu;
	uint32_t gcLdsSize;
} PACKED;

struct CAILAsicCapsEntry {
	uint32_t familyId, deviceId;
	uint32_t revision, extRevision;
	uint32_t pciRevision;
	uint32_t _reserved;
	const uint32_t *caps;
	const uint32_t *skeleton;
} PACKED;

struct CAILAsicCapsInitEntry {
	uint64_t familyId, deviceId;
	uint64_t revision, extRevision;
	uint64_t pciRevision;
	const uint32_t *caps;
	const void *goldenCaps;
} PACKED;

static const uint32_t ddiCapsNavi21[16] = {0x800001U, 0x1FEU, 0x0U, 0x0U, 0x200U, 0x8000000U, 0x8000000, 0x2U,
    0x200A0101U, 0xA20600U, 0x42000028U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U};

static const uint32_t ddiCapsNavi22[16] = {0x800001U, 0x1FEU, 0x0U, 0x0U, 0x200U, 0x8000000, 0x8000000, 0x2U,
    0x200A0101U, 0xA20600U, 0x42000020U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U};
	
enum CAILResult : uint32_t {
	kCAILResultSuccess = 0,
	kCAILResultInvalidArgument,
	kCAILResultGeneralFailure,
	kCAILResultResourcesExhausted,
	kCAILResultUnsupported,
};

struct CAILDeviceTypeEntry {
	uint32_t deviceId;
	uint32_t deviceType;
} PACKED;

static const uint32_t ravenDevAttrFlags = 0x49;

struct DeviceCapabilityEntry {
	uint64_t familyId, extRevision;
	uint64_t deviceId, revision, enumRevision;
	const void *swipInfo, *swipInfoMinimal;
	const uint32_t *devAttrFlags;
	const void *goldenRegisterSetings, *doorbellRange;
} PACKED;

constexpr uint64_t DEVICE_CAP_ENTRY_REV_DONT_CARE = 0xDEADCAFEU;

enum VideoMemoryType : uint32_t {
	kVideoMemoryTypeUnknown,
	kVideoMemoryTypeDDR2,
	kVideoMemoryTypeDDR3 = 3,
	kVideoMemoryTypeDDR4,
};

constexpr uint32_t PP_RESULT_OK = 1;
