//  Copyright © 2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5. See LICENSE for
//  details.

#pragma once
#include <Headers/kern_util.hpp>

/**
 * `_gc_sw_init`
 * AMDRadeonX6800HWLibs.kext
 * Replace call to `_gc_get_hw_version` with constant (0x0A0304).
 */
static const uint8_t kGcSwInitOriginal[] = {0x0C, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x41, 0x89, 0xC7};
static const uint8_t kGcSwInitOriginalMask[] = {0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF};
static const uint8_t kGcSwInitPatched[] = {0x00, 0xB8, 0x04, 0x03, 0x0A, 0x00, 0x00, 0x00, 0x00};
static const uint8_t kGcSwInitPatchedMask[] = {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00};
static_assert(arrsize(kGcSwInitOriginal) == arrsize(kGcSwInitOriginalMask));
static_assert(arrsize(kGcSwInitOriginal) == arrsize(kGcSwInitPatched));
static_assert(arrsize(kGcSwInitPatched) == arrsize(kGcSwInitPatchedMask));

/**
 * `_gc_set_fw_entry_info`
 * AMDRadeonX6800HWLibs.kext
 * Replace call to `_gc_get_hw_version` with constant (0x0A0304).
 */
static const uint8_t kGcSetFwEntryInfoOriginal[] = {0xE8, 0x00, 0x00, 0x00, 0x00, 0x31, 0x00, 0x41, 0x89, 0x00, 0x10,
    0x41, 0x89, 0x00, 0x00, 0x00};
static const uint8_t kGcSetFwEntryInfoOriginalMask[] = {0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00,
    0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00};
static const uint8_t kGcSetFwEntryInfoPatched[] = {0xB8, 0x04, 0x03, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t kGcSetFwEntryInfoPatchedMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00};
static_assert(arrsize(kGcSetFwEntryInfoOriginal) == arrsize(kGcSetFwEntryInfoOriginalMask));
static_assert(arrsize(kGcSetFwEntryInfoOriginal) == arrsize(kGcSetFwEntryInfoPatched));
static_assert(arrsize(kGcSetFwEntryInfoPatched) == arrsize(kGcSetFwEntryInfoPatchedMask));

/**
 * `_psp_sw_init`
 * AMDRadeonX6810HWLibs.kext
 * Force `pIn->pspVerMinor == 0x5` path.
 */
static const uint8_t kPspSwInit1Original[] = {0x8B, 0x43, 0x10, 0x83, 0xF8, 0x05, 0x74, 0x00, 0x85, 0xC0};
static const uint8_t kPspSwInit1OriginalMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF};
static const uint8_t kPspSwInit1Patched[] = {0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0xEB, 0x00, 0x00, 0x00};
static const uint8_t kPspSwInit1PatchedMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00};
static_assert(arrsize(kPspSwInit1Original) == arrsize(kPspSwInit1OriginalMask));
static_assert(arrsize(kPspSwInit1Original) == arrsize(kPspSwInit1Patched));
static_assert(arrsize(kPspSwInit1Patched) == arrsize(kPspSwInit1PatchedMask));

/**
 * `_psp_sw_init`
 * AMDRadeonX6810HWLibs.kext
 * Force `pIn->pspVerPatch == 0x0` path.
 */
static const uint8_t kPspSwInit2Original[] = {0x83, 0x7B, 0x14, 0x00, 0x74, 0x00, 0x41};
static const uint8_t kPspSwInit2OriginalMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF};
static const uint8_t kPspSwInit2Patched[] = {0x66, 0x90, 0x66, 0x90, 0xEB, 0x00, 0x00};
static const uint8_t kPspSwInit2PatchedMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00};
static_assert(arrsize(kPspSwInit2Original) == arrsize(kPspSwInit2OriginalMask));
static_assert(arrsize(kPspSwInit2Original) == arrsize(kPspSwInit2Patched));
static_assert(arrsize(kPspSwInit2Patched) == arrsize(kPspSwInit2PatchedMask));

/**
 * `_psp_sw_init`
 * AMDRadeonX6810HWLibs.kext
 * Set field `0x7???` to `0xE`.
 */
static const uint8_t kPspSwInit3Original[] = {0x41, 0xC7, 0x84, 0x24, 0x00, 0x70, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00};
static const uint8_t kPspSwInit3OriginalMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF};
static const uint8_t kPspSwInit3Patched[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00};
static const uint8_t kPspSwInit3PatchedMask[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00,
    0x00};
static_assert(arrsize(kPspSwInit3Original) == arrsize(kPspSwInit3OriginalMask));
static_assert(arrsize(kPspSwInit3Original) == arrsize(kPspSwInit3Patched));
static_assert(arrsize(kPspSwInit3Patched) == arrsize(kPspSwInit3PatchedMask));

/**
 * `_smu_11_0_7_ucode`
 * AMDRadeonX6810HWLibs.kext
 * Set ucode version to 0x412A00
 */
static const uint8_t kSmuUcodeOriginal[] = {0x00, 0x24, 0x3B, 0x00, 0x00, 0xB2, 0x03, 0x00};
static const uint8_t kSmuUcodePatched[] = {0x00, 0x2A, 0x41, 0x00, 0x00, 0xB2, 0x03, 0x00};
static_assert(arrsize(kSmuUcodeOriginal) == arrsize(kSmuUcodePatched));

/**
 * `_smu_11_0_7_check_fw_version`
 * AMDRadeonX6810HWLibs.kext
 * Skip check for firmware version.
 */
static const uint8_t kSmu1107CheckFwVersionOriginal[] = {0x83, 0xFE, 0x40, 0x75, 0x00, 0xEB, 0x00};
static const uint8_t kSmu1107CheckFwVersionOriginalMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0x00};
static const uint8_t kSmu1107CheckFwVersionPatched[] = {0x66, 0x90, 0x66, 0x90, 0x90, 0x00, 0x00};
static const uint8_t kSmu1107CheckFwVersionPatchedMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00};
static_assert(arrsize(kSmu1107CheckFwVersionOriginal) == arrsize(kSmu1107CheckFwVersionOriginalMask));
static_assert(arrsize(kSmu1107CheckFwVersionOriginal) == arrsize(kSmu1107CheckFwVersionPatched));
static_assert(arrsize(kSmu1107CheckFwVersionPatched) == arrsize(kSmu1107CheckFwVersionPatchedMask));
