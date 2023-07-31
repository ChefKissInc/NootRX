//  Copyright © 2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5. See LICENSE for
//  details.

#pragma once
#include <Headers/kern_util.hpp>

constexpr uint32_t mmPCIE_INDEX2 = 0xE;
constexpr uint32_t mmPCIE_DATA2 = 0xF;

constexpr uint32_t MP_BASE = 0x16000;

constexpr uint32_t AMDGPU_MAX_USEC_TIMEOUT = 100000;

constexpr uint32_t mmMP1_SMN_C2PMSG_90 = 0x29A;
constexpr uint32_t mmMP1_SMN_C2PMSG_82 = 0x292;
constexpr uint32_t mmMP1_SMN_C2PMSG_66 = 0x282;

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

static const uint32_t ddiCapsNavi21[16] = {0x800001U, 0x1FEU, 0x0U, 0x0U, 0x200U, 0x8000000U, 0x8000000U, 0x2U,
    0x200A0101U, 0xA20600U, 0x42000028U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U};

static const uint32_t ddiCapsNavi22[16] = {0x800001U, 0x1FEU, 0x0U, 0x0U, 0x200U, 0x8000000U, 0x8000000U, 0x2U,
    0x200A0101U, 0xA20600U, 0x42000020U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U};

struct CAILDeviceTypeEntry {
    uint32_t deviceId;
    uint32_t deviceType;
} PACKED;

struct DeviceCapabilityEntry {
    uint64_t familyId, extRevision;
    uint64_t deviceId, revision, enumRevision;
    const void *swipInfo, *swipInfoMinimal;
    const uint32_t *devAttrFlags;
    const void *goldenRegisterSetings, *doorbellRange;
} PACKED;

constexpr uint64_t DEVICE_CAP_ENTRY_REV_DONT_CARE = 0xDEADCAFEU;
