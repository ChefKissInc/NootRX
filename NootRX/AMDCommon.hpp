//! Copyright © 2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5.
//! See LICENSE for details.

#pragma once
#include <Headers/kern_util.hpp>

constexpr UInt32 AMDGPU_FAMILY_NAVI = 0x8F;

//-------- Generic Registers --------//

constexpr UInt32 mmPCIE_INDEX2 = 0xE;
constexpr UInt32 mmPCIE_DATA2 = 0xF;

//-------- GC Registers --------//

constexpr UInt32 mmCGTT_SPI_CS_CLK_CTRL = 0x507C;
constexpr UInt32 mmCGTT_SPI_CS_CLK_CTRL_BASE_IDX = 1;
constexpr UInt32 mmCGTT_SPI_PS_CLK_CTRL = 0x507D;
constexpr UInt32 mmCGTT_SPI_PS_CLK_CTRL_BASE_IDX = 1;
constexpr UInt32 mmCGTT_SPI_RA0_CLK_CTRL = 0x507A;
constexpr UInt32 mmCGTT_SPI_RA0_CLK_CTRL_BASE_IDX = 1;
constexpr UInt32 mmCGTT_SPI_RA1_CLK_CTRL = 0x507B;
constexpr UInt32 mmCGTT_SPI_RA1_CLK_CTRL_BASE_IDX = 1;
constexpr UInt32 mmCPF_GCR_CNTL = 0x1F53;
constexpr UInt32 mmCPF_GCR_CNTL_BASE_IDX = 0;
constexpr UInt32 mmDB_DEBUG3 = 0x13AE;
constexpr UInt32 mmDB_DEBUG3_BASE_IDX = 0;
constexpr UInt32 mmDB_DEBUG4 = 0x13AF;
constexpr UInt32 mmDB_DEBUG4_BASE_IDX = 0;
constexpr UInt32 mmDB_EXCEPTION_CONTROL = 0x13BF;
constexpr UInt32 mmDB_EXCEPTION_CONTROL_BASE_IDX = 0;
constexpr UInt32 mmGCR_GENERAL_CNTL_Sienna_Cichlid = 0x1580;
constexpr UInt32 mmGCR_GENERAL_CNTL_Sienna_Cichlid_BASE_IDX = 0;
constexpr UInt32 mmGCUTCL2_CGTT_CLK_CTRL_Sienna_Cichlid = 0x16F3;
constexpr UInt32 mmGCUTCL2_CGTT_CLK_CTRL_Sienna_Cichlid_BASE_IDX = 0;
constexpr UInt32 mmGCVM_L2_CGTT_CLK_CTRL_Sienna_Cichlid = 0x15DB;
constexpr UInt32 mmGCVM_L2_CGTT_CLK_CTRL_Sienna_Cichlid_BASE_IDX = 0;
constexpr UInt32 mmGE_PC_CNTL = 0x0FE5;
constexpr UInt32 mmGE_PC_CNTL_BASE_IDX = 0;
constexpr UInt32 mmGL2A_ADDR_MATCH_MASK = 0x2E21;
constexpr UInt32 mmGL2A_ADDR_MATCH_MASK_BASE_IDX = 1;
constexpr UInt32 mmGL2C_ADDR_MATCH_MASK = 0x2E03;
constexpr UInt32 mmGL2C_ADDR_MATCH_MASK_BASE_IDX = 1;
constexpr UInt32 mmGL2C_CM_CTRL1 = 0x2E08;
constexpr UInt32 mmGL2C_CM_CTRL1_BASE_IDX = 1;
constexpr UInt32 mmGL2C_CTRL3 = 0x2E0C;
constexpr UInt32 mmGL2C_CTRL3_BASE_IDX = 1;
constexpr UInt32 mmPA_CL_ENHANCE = 0x1025;
constexpr UInt32 mmPA_CL_ENHANCE_BASE_IDX = 0;
constexpr UInt32 mmPA_SC_BINNER_TIMEOUT_COUNTER = 0x1070;
constexpr UInt32 mmPA_SC_BINNER_TIMEOUT_COUNTER_BASE_IDX = 0;
constexpr UInt32 mmPA_SC_ENHANCE_2 = 0x107C;
constexpr UInt32 mmPA_SC_ENHANCE_2_BASE_IDX = 0;
constexpr UInt32 mmSPI_CONFIG_CNTL_1 = 0x11EF;
constexpr UInt32 mmSPI_CONFIG_CNTL_1_BASE_IDX = 0;
constexpr UInt32 mmSPI_START_PHASE = 0x11DB;
constexpr UInt32 mmSPI_START_PHASE_BASE_IDX = 0;
constexpr UInt32 mmSQ_CONFIG = 0x10A0;
constexpr UInt32 mmSQ_CONFIG_BASE_IDX = 0;
constexpr UInt32 mmSQ_PERFCOUNTER0_SELECT = 0x39C0;
constexpr UInt32 mmSQ_PERFCOUNTER0_SELECT_BASE_IDX = 1;
constexpr UInt32 mmSQ_PERFCOUNTER1_SELECT = 0x39C1;
constexpr UInt32 mmSQ_PERFCOUNTER1_SELECT_BASE_IDX = 1;
constexpr UInt32 mmSQ_PERFCOUNTER10_SELECT = 0x39CA;
constexpr UInt32 mmSQ_PERFCOUNTER10_SELECT_BASE_IDX = 1;
constexpr UInt32 mmSQ_PERFCOUNTER11_SELECT = 0x39CB;
constexpr UInt32 mmSQ_PERFCOUNTER11_SELECT_BASE_IDX = 1;
constexpr UInt32 mmSQ_PERFCOUNTER12_SELECT = 0x39CC;
constexpr UInt32 mmSQ_PERFCOUNTER12_SELECT_BASE_IDX = 1;
constexpr UInt32 mmSQ_PERFCOUNTER13_SELECT = 0x39CD;
constexpr UInt32 mmSQ_PERFCOUNTER13_SELECT_BASE_IDX = 1;
constexpr UInt32 mmSQ_PERFCOUNTER14_SELECT = 0x39CE;
constexpr UInt32 mmSQ_PERFCOUNTER14_SELECT_BASE_IDX = 1;
constexpr UInt32 mmSQ_PERFCOUNTER15_SELECT = 0x39CF;
constexpr UInt32 mmSQ_PERFCOUNTER15_SELECT_BASE_IDX = 1;
constexpr UInt32 mmSQ_PERFCOUNTER2_SELECT = 0x39C2;
constexpr UInt32 mmSQ_PERFCOUNTER2_SELECT_BASE_IDX = 1;
constexpr UInt32 mmSQ_PERFCOUNTER3_SELECT = 0x39C3;
constexpr UInt32 mmSQ_PERFCOUNTER3_SELECT_BASE_IDX = 1;
constexpr UInt32 mmSQ_PERFCOUNTER4_SELECT = 0x39C4;
constexpr UInt32 mmSQ_PERFCOUNTER4_SELECT_BASE_IDX = 1;
constexpr UInt32 mmSQ_PERFCOUNTER5_SELECT = 0x39C5;
constexpr UInt32 mmSQ_PERFCOUNTER5_SELECT_BASE_IDX = 1;
constexpr UInt32 mmSQ_PERFCOUNTER6_SELECT = 0x39C6;
constexpr UInt32 mmSQ_PERFCOUNTER6_SELECT_BASE_IDX = 1;
constexpr UInt32 mmSQ_PERFCOUNTER7_SELECT = 0x39C7;
constexpr UInt32 mmSQ_PERFCOUNTER7_SELECT_BASE_IDX = 1;
constexpr UInt32 mmSQ_PERFCOUNTER8_SELECT = 0x39C8;
constexpr UInt32 mmSQ_PERFCOUNTER8_SELECT_BASE_IDX = 1;
constexpr UInt32 mmSQ_PERFCOUNTER9_SELECT = 0x39C9;
constexpr UInt32 mmSQ_PERFCOUNTER9_SELECT_BASE_IDX = 1;
constexpr UInt32 mmTA_CNTL_AUX = 0x12E2;
constexpr UInt32 mmTA_CNTL_AUX_BASE_IDX = 0;
constexpr UInt32 mmUTCL1_CTRL = 0x1588;
constexpr UInt32 mmUTCL1_CTRL_BASE_IDX = 0;
constexpr UInt32 mmVGT_GS_MAX_WAVE_ID = 0x1009;
constexpr UInt32 mmVGT_GS_MAX_WAVE_ID_BASE_IDX = 0;
constexpr UInt32 mmLDS_CONFIG = 0x10A2;
constexpr UInt32 mmLDS_CONFIG_BASE_IDX = 0;

//-------- AMD Catalyst Data Types --------//

struct CAILAsicCapsEntry {
    UInt32 familyId, deviceId;
    UInt32 revision, extRevision;
    UInt32 pciRevision;
    UInt32 _reserved;
    const UInt32 *caps;
    const UInt32 *skeleton;
} PACKED;

struct CAILAsicCapsInitEntry {
    UInt64 familyId, deviceId;
    UInt64 revision, extRevision;
    UInt64 pciRevision;
    const UInt32 *caps;
    const void *goldenCaps;
} PACKED;

enum CAILResult : UInt32 {
    kCAILResultSuccess = 0,
    kCAILResultInvalidArgument,
    kCAILResultGeneralFailure,
    kCAILResultResourcesExhausted,
    kCAILResultUnsupported,
};

struct CAILDeviceTypeEntry {
    UInt32 deviceId, deviceType;
} PACKED;

struct CAILIPGoldenRegister {
    const UInt32 regOffset;
    const UInt32 segment;
    const UInt32 andMask;
    const UInt32 orMask;
} PACKED;

#define GOLDEN_REGISTER(reg, and, or) \
    { .regOffset = reg, .segment = reg##_BASE_IDX, .andMask = and, .orMask = or }
#define GOLDEN_REGISTER_TERMINATOR \
    { .regOffset = 0xFFFFFFFF, .segment = 0xFFFFFFFF, .andMask = 0xFFFFFFFF, .orMask = 0xFFFFFFFF }

enum CAILIPType : UInt32 {
    kCAILIPTypeUnknown = 0,
    kCAILIPTypeGC = 11,
};

struct CAILASICGoldenRegisters {
    const CAILIPType ipType;
    const UInt32 instance;    //! Not sure about that one.
    const CAILIPGoldenRegister *entries;
} PACKED;

#define GOLDEN_REGISTERS(type, inst, ents) \
    { .ipType = kCAILIPType##type, .instance = inst, .entries = ents }

#define GOLDEN_REGISTERS_TERMINATOR \
    { .ipType = kCAILIPTypeUnknown, .instance = 0, .entries = nullptr }

struct CAILASICGoldenSettings {
    //! Golden settings for GPUs emulated using the Cadence Palladium Emulation platform. We don't care.
    const CAILASICGoldenRegisters *palladiumGoldenSettings;
    const CAILASICGoldenRegisters *goldenSettings;
} PACKED;

struct DeviceCapabilityEntry {
    UInt64 familyId, extRevision;
    UInt64 deviceId, revision, enumRevision;
    const void *swipInfo, *swipInfoMinimal;
    const UInt32 *devAttrFlags;
    CAILASICGoldenSettings *asicGoldenSettings;
    void *doorbellRange;
} PACKED;

enum AMDPSPCommand : UInt32 {
    kPSPCommandLoadTA = 1,
    kPSPCommandLoadASD = 4,
    kPSPCommandLoadIPFW = 6,
};

enum AMDUCodeID : UInt32 {
    kUCodeSMU = 1,
    kUCodeCE,
    kUCodePFP,
    kUCodeME,
    kUCodeMEC1JT,
    kUCodeMEC2JT,
    kUCodeMEC1,
    kUCodeMEC2,
    kUCodeMES,
    kUCodeMESStack,
    kUCodeRLC,
    kUCodeSDMA0,
    kUCodeVCN0,
    kUCodeRLCP = 22,
    kUCodeRLCSRListGPM,
    kUCodeRLCSRListSRM,
    kUCodeRLCSRListCntl,
    kUCodeRLCLX6Iram,
    kUCodeRLCLX6Dram,
    kUCodeVCNSram,
    kUCodeGlobalTapDelays = 30,
    kUCodeSE0TapDelays,
    kUCodeSE1TapDelays,
    kUCodeSE2TapDelays,
    kUCodeSE3TapDelays,
    kUCodeDMCUB,
};

//-------- AMD Catalyst Constants --------//

constexpr UInt64 DEVICE_CAP_ENTRY_REV_DONT_CARE = 0xDEADCAFEU;

static const UInt32 ddiCapsNavi21[16] = {0x800001, 0x1FE, 0x0, 0x0, 0x200, 0x8000000, 0x8000000, 0x2, 0x200A0101,
    0xA20600, 0x42000028, 0x0, 0x0, 0x0, 0x0, 0x0};

static const UInt32 ddiCapsNavi22[16] = {0x800001, 0x1FE, 0x0, 0x0, 0x200, 0x8000000, 0x8000000, 0x2, 0x200A0101,
    0xA20600, 0x42000020, 0x0, 0x0, 0x0, 0x0, 0x0};

//---- Golden Settings ----//

static const CAILIPGoldenRegister gcGoldenSettingsNavi22[] = {
    GOLDEN_REGISTER(mmCGTT_SPI_CS_CLK_CTRL, 0xFF7F0FFF, 0x78000100),
    GOLDEN_REGISTER(mmCGTT_SPI_PS_CLK_CTRL, 0xFF7F0FFF, 0x78000100),
    GOLDEN_REGISTER(mmCGTT_SPI_RA0_CLK_CTRL, 0xFF7F0FFF, 0x30000100),
    GOLDEN_REGISTER(mmCGTT_SPI_RA1_CLK_CTRL, 0xFF7F0FFF, 0x7E000100),
    GOLDEN_REGISTER(mmCPF_GCR_CNTL, 0x0007FFFF, 0x0000C000),
    GOLDEN_REGISTER(mmDB_DEBUG3, 0xFFFFFFFF, 0x00000280),
    GOLDEN_REGISTER(mmDB_DEBUG4, 0xFFFFFFFF, 0x00800000),
    GOLDEN_REGISTER(mmDB_EXCEPTION_CONTROL, 0x7FFF0F1F, 0x00B80000),
    GOLDEN_REGISTER(mmGCR_GENERAL_CNTL_Sienna_Cichlid, 0x1FF1FFFF, 0x00000500),
    GOLDEN_REGISTER(mmGCUTCL2_CGTT_CLK_CTRL_Sienna_Cichlid, 0xFFFFFFFF, 0xFF008080),
    GOLDEN_REGISTER(mmGCVM_L2_CGTT_CLK_CTRL_Sienna_Cichlid, 0xFFFF8FFF, 0xFF008080),
    GOLDEN_REGISTER(mmGE_PC_CNTL, 0x003FFFFF, 0x00280400),
    GOLDEN_REGISTER(mmGL2A_ADDR_MATCH_MASK, 0xFFFFFFFF, 0xFFFFFFCF),
    GOLDEN_REGISTER(mmGL2C_ADDR_MATCH_MASK, 0xFFFFFFFF, 0xFFFFFFCF),
    GOLDEN_REGISTER(mmGL2C_CM_CTRL1, 0xFF8FFF0F, 0x580F1008),
    GOLDEN_REGISTER(mmGL2C_CTRL3, 0xF7FFFFFF, 0x00F80988),
    GOLDEN_REGISTER(mmPA_CL_ENHANCE, 0xF17FFFFF, 0x01200007),
    GOLDEN_REGISTER(mmPA_SC_BINNER_TIMEOUT_COUNTER, 0xFFFFFFFF, 0x00000800),
    GOLDEN_REGISTER(mmPA_SC_ENHANCE_2, 0xFFFFFFBF, 0x00000820),
    GOLDEN_REGISTER(mmSPI_CONFIG_CNTL_1, 0xFFFFFFFF, 0x00070104),
    GOLDEN_REGISTER(mmSPI_START_PHASE, 0x000000FF, 0x00000004),
    GOLDEN_REGISTER(mmSQ_CONFIG, 0xE07DF47F, 0x00180070),
    GOLDEN_REGISTER(mmSQ_PERFCOUNTER0_SELECT, 0xF0F001FF, 0x00000000),
    GOLDEN_REGISTER(mmSQ_PERFCOUNTER1_SELECT, 0xF0F001FF, 0x00000000),
    GOLDEN_REGISTER(mmSQ_PERFCOUNTER10_SELECT, 0xF0F001FF, 0x00000000),
    GOLDEN_REGISTER(mmSQ_PERFCOUNTER11_SELECT, 0xF0F001FF, 0x00000000),
    GOLDEN_REGISTER(mmSQ_PERFCOUNTER12_SELECT, 0xF0F001FF, 0x00000000),
    GOLDEN_REGISTER(mmSQ_PERFCOUNTER13_SELECT, 0xF0F001FF, 0x00000000),
    GOLDEN_REGISTER(mmSQ_PERFCOUNTER14_SELECT, 0xF0F001FF, 0x00000000),
    GOLDEN_REGISTER(mmSQ_PERFCOUNTER15_SELECT, 0xF0F001FF, 0x00000000),
    GOLDEN_REGISTER(mmSQ_PERFCOUNTER2_SELECT, 0xF0F001FF, 0x00000000),
    GOLDEN_REGISTER(mmSQ_PERFCOUNTER3_SELECT, 0xF0F001FF, 0x00000000),
    GOLDEN_REGISTER(mmSQ_PERFCOUNTER4_SELECT, 0xF0F001FF, 0x00000000),
    GOLDEN_REGISTER(mmSQ_PERFCOUNTER5_SELECT, 0xF0F001FF, 0x00000000),
    GOLDEN_REGISTER(mmSQ_PERFCOUNTER6_SELECT, 0xF0F001FF, 0x00000000),
    GOLDEN_REGISTER(mmSQ_PERFCOUNTER7_SELECT, 0xF0F001FF, 0x00000000),
    GOLDEN_REGISTER(mmSQ_PERFCOUNTER8_SELECT, 0xF0F001FF, 0x00000000),
    GOLDEN_REGISTER(mmSQ_PERFCOUNTER9_SELECT, 0xF0F001FF, 0x00000000),
    GOLDEN_REGISTER(mmTA_CNTL_AUX, 0xFFF7FFFF, 0x01030000),
    GOLDEN_REGISTER(mmUTCL1_CTRL, 0xFFBFFFFF, 0x00A00000),
    GOLDEN_REGISTER(mmVGT_GS_MAX_WAVE_ID, 0x00000FFF, 0x000003FF),
    GOLDEN_REGISTER(mmLDS_CONFIG, 0x00000020, 0x00000020),
    GOLDEN_REGISTER_TERMINATOR,
};

static const CAILASICGoldenRegisters goldenSettingsNavi22[] = {
    GOLDEN_REGISTERS(GC, 0, gcGoldenSettingsNavi22),
    GOLDEN_REGISTERS_TERMINATOR,
};
