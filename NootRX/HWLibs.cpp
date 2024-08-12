//! Copyright © 2023-2024 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5.
//! See LICENSE for details.

#include "HWLibs.hpp"
#include "Firmware.hpp"
#include "NootRX.hpp"
#include "PatcherPlus.hpp"
#include <Headers/kern_api.hpp>

static const char *pathRadeonX6000HWServices =
    "/System/Library/Extensions/AMDRadeonX6000HWServices.kext/Contents/MacOS/AMDRadeonX6000HWServices";

static const char *pathRadeonX6800HWLibs = "/System/Library/Extensions/AMDRadeonX6000HWServices.kext/Contents/PlugIns/"
                                           "AMDRadeonX6800HWLibs.kext/Contents/MacOS/AMDRadeonX6800HWLibs";

static const char *pathRadeonX6810HWLibs = "/System/Library/Extensions/AMDRadeonX6000HWServices.kext/Contents/PlugIns/"
                                           "AMDRadeonX6810HWLibs.kext/Contents/MacOS/AMDRadeonX6810HWLibs";

static KernelPatcher::KextInfo kextRadeonX6000HWServices {"com.apple.kext.AMDRadeonX6000HWServices",
    &pathRadeonX6000HWServices, 1, {}, {}, KernelPatcher::KextInfo::Unloaded};

static KernelPatcher::KextInfo kextRadeonX6810HWLibs {"com.apple.kext.AMDRadeonX6810HWLibs", &pathRadeonX6810HWLibs, 1,
    {}, {}, KernelPatcher::KextInfo::Unloaded};

static KernelPatcher::KextInfo kextRadeonX6800HWLibs {"com.apple.kext.AMDRadeonX6800HWLibs", &pathRadeonX6800HWLibs, 1,
    {}, {}, KernelPatcher::KextInfo::Unloaded};

HWLibs *HWLibs::callback = nullptr;

void HWLibs::init() {
    callback = this;
    lilu.onKextLoadForce(&kextRadeonX6000HWServices);
    lilu.onKextLoadForce(&kextRadeonX6800HWLibs);
    lilu.onKextLoadForce(&kextRadeonX6810HWLibs);
}

#define DEF_FAKECPY(ident, filename)            \
    static void ident(void *data) {             \
        const auto &fw = getFWByName(filename); \
        memcpy(data, fw.data, fw.length);       \
    }

DEF_FAKECPY(fakecpyNavi21Kdb, "psp_key_database_navi21.bin");
DEF_FAKECPY(fakecpyNavi21Sos, "psp_sos_navi21.bin");
DEF_FAKECPY(fakecpyNavi21SysDrv, "psp_sys_drv_navi21.bin");
DEF_FAKECPY(fakecpyNavi21TosSpl, "psp_tos_spl_navi21.bin");

DEF_FAKECPY(fakecpyNavi22Kdb, "psp_key_database_navi22.bin");
DEF_FAKECPY(fakecpyNavi22Sos, "psp_sos_navi22.bin");
DEF_FAKECPY(fakecpyNavi22SysDrv, "psp_sys_drv_navi22.bin");
DEF_FAKECPY(fakecpyNavi22TosSpl, "psp_tos_spl_navi22.bin");

DEF_FAKECPY(fakecpyNavi23Kdb, "psp_key_database_navi23.bin");
DEF_FAKECPY(fakecpyNavi23Sos, "psp_sos_navi23.bin");
DEF_FAKECPY(fakecpyNavi23SysDrv, "psp_sys_drv_navi23.bin");
DEF_FAKECPY(fakecpyNavi23TosSpl, "psp_tos_spl_navi23.bin");

bool HWLibs::processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size) {
    if (kextRadeonX6000HWServices.loadIndex == id) {
        RouteRequestPlus request {"__ZN38AMDRadeonX6000_AMDRadeonHWServicesNavi16getMatchPropertyEv",
            wrapGetMatchProperty};
        PANIC_COND(!request.route(patcher, id, slide, size), "HWServices", "Failed to route getMatchProperty");
    } else if ((kextRadeonX6810HWLibs.loadIndex == id) || (kextRadeonX6800HWLibs.loadIndex == id)) {
        NootRXMain::callback->setRMMIOIfNecessary();

        CAILAsicCapsEntry *orgCapsTable = nullptr;
        CAILDeviceTypeEntry *orgDeviceTypeTable = nullptr;
        DeviceCapabilityEntry *orgDevCapTable = nullptr;
        CAILAsicCapsInitEntry *orgCapsInitTable = nullptr;

        SolveRequestPlus solveRequests[] = {
            {"__ZL15deviceTypeTable", orgDeviceTypeTable, kDeviceTypeTablePattern},
            {"__ZL20CAIL_ASIC_CAPS_TABLE", orgCapsTable, kCailAsicCapsTableHWLibsPattern},
            {"_DeviceCapabilityTbl", orgDevCapTable, kDeviceCapabilityTblPattern},
        };
        PANIC_COND(!SolveRequestPlus::solveAll(patcher, id, solveRequests, slide, size), "HWLibs",
            "Failed to resolve symbols");
        SolveRequestPlus solveRequest {"_CAILAsicCapsInitTable", orgCapsInitTable, kCAILAsicCapsInitTablePattern};
        solveRequest.solve(patcher, id, slide, size);

        bool sonoma144 = getKernelVersion() > KernelVersion::Sonoma ||
                         (getKernelVersion() == KernelVersion::Sonoma && getKernelMinorVersion() >= 4);
        if (sonoma144) {
            RouteRequestPlus request = {"_psp_cmd_km_submit", wrapPspCmdKmSubmit, this->orgPspCmdKmSubmit,
                kPspCmdKmSubmitPattern14_4, kPspCmdKmSubmitMask14_4};
            PANIC_COND(!request.route(patcher, id, slide, size), "HWLibs", "Failed to route psp_cmd_km_submit (14.4+)");
        } else {
            RouteRequestPlus request = {"_psp_cmd_km_submit", wrapPspCmdKmSubmit, this->orgPspCmdKmSubmit,
                kPspCmdKmSubmitPattern, kPspCmdKmSubmitMask};
            PANIC_COND(!request.route(patcher, id, slide, size), "HWLibs", "Failed to route psp_cmd_km_submit");
        }

        if (NootRXMain::callback->chipType == ChipType::Navi22) {
            if (sonoma144) {
                RouteRequestPlus request = {"_smu_11_0_7_send_message_with_parameter",
                    wrapSmu1107SendMessageWithParameter, this->orgSmu1107SendMessageWithParameter,
                    kSmu1107SendMessageWithParameterPattern14_4, kSmu1107SendMessageWithParameterPatternMask14_4};
                PANIC_COND(!request.route(patcher, id, slide, size), "HWLibs",
                    "Failed to route smu_11_0_7_send_message_with_parameter (14.4+)");
            } else {
                RouteRequestPlus request = {"_smu_11_0_7_send_message_with_parameter",
                    wrapSmu1107SendMessageWithParameter, this->orgSmu1107SendMessageWithParameter,
                    kSmu1107SendMessageWithParameterPattern, kSmu1107SendMessageWithParameterPatternMask};
                PANIC_COND(!request.route(patcher, id, slide, size), "HWLibs",
                    "Failed to route smu_11_0_7_send_message_with_parameter");
            }
        }

        PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "HWLibs",
            "Failed to enable kernel writing");

        *orgDeviceTypeTable = {
            .deviceId = NootRXMain::callback->deviceId,
            .deviceType = (kextRadeonX6800HWLibs.loadIndex == id) ? 6U : 8,
        };

        UInt32 targetDeviceId = NootRXMain::callback->chipType == ChipType::Navi21 ? 0x73BF : 0x73FF;
        for (; orgCapsTable->deviceId != 0xFFFFFFFF; orgCapsTable++) {
            if (orgCapsTable->familyId == AMDGPU_FAMILY_NAVI && orgCapsTable->deviceId == targetDeviceId) {
                orgCapsTable->deviceId = NootRXMain::callback->deviceId;
                orgCapsTable->revision = NootRXMain::callback->revision;
                orgCapsTable->extRevision =
                    static_cast<UInt32>(NootRXMain::callback->enumRevision) + NootRXMain::callback->revision;
                orgCapsTable->pciRevision = NootRXMain::callback->pciRevision;
                orgCapsTable->caps = ddiCapsNavi2Universal;
                if (orgCapsInitTable) {
                    *orgCapsInitTable = {
                        .familyId = AMDGPU_FAMILY_NAVI,
                        .deviceId = NootRXMain::callback->deviceId,
                        .revision = NootRXMain::callback->revision,
                        .extRevision = static_cast<UInt32>(orgCapsTable->extRevision),
                        .pciRevision = NootRXMain::callback->pciRevision,
                        .caps = orgCapsTable->caps,
                    };
                }
                break;
            }
        }
        PANIC_COND(orgCapsTable->deviceId == 0xFFFFFFFF, "HWLibs", "Failed to find ASIC caps init table entry");

        for (; orgDevCapTable->familyId; orgDevCapTable++) {
            if (orgDevCapTable->familyId == AMDGPU_FAMILY_NAVI && orgDevCapTable->deviceId == targetDeviceId) {
                orgDevCapTable->deviceId = NootRXMain::callback->deviceId;
                orgDevCapTable->extRevision =
                    static_cast<UInt64>(NootRXMain::callback->enumRevision) + NootRXMain::callback->revision;
                orgDevCapTable->revision = DEVICE_CAP_ENTRY_REV_DONT_CARE;
                orgDevCapTable->enumRevision = DEVICE_CAP_ENTRY_REV_DONT_CARE;
                switch (NootRXMain::callback->chipType) {
                    case ChipType::Navi21:
                        orgDevCapTable->asicGoldenSettings->goldenSettings = goldenSettingsNavi21;
                        break;
                    case ChipType::Navi22:
                        orgDevCapTable->asicGoldenSettings->goldenSettings = goldenSettingsNavi22;
                        break;
                    case ChipType::Navi23:
                        orgDevCapTable->asicGoldenSettings->goldenSettings = goldenSettingsNavi23;
                        break;
                    default:
                        break;
                }
                break;
            }
        }
        PANIC_COND(!orgDevCapTable->familyId, "HWLibs", "Failed to find device capability table entry");
        MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
        DBGLOG("HWLibs", "Applied DDI Caps patches");

        auto hijackMemCpyBlock = [=](UInt32 arg1, UInt32 arg1Mask, void (*func)(void *data)) {
            const UInt8 find[] = {0x48, 0x8D, 0x35, 0x00, 0x00, 0x00, 0x00, 0xBA, static_cast<UInt8>(arg1 & 0xFF),
                static_cast<UInt8>((arg1 >> 8) & 0xFF), static_cast<UInt8>((arg1 >> 16) & 0xFF),
                static_cast<UInt8>((arg1 >> 24) & 0xFF), 0x4C, 0x89, 0x00, 0xE8, 0x00, 0x00, 0x00, 0x00};
            const UInt8 mask[] = {0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, static_cast<UInt8>(arg1Mask & 0xFF),
                static_cast<UInt8>((arg1Mask >> 8) & 0xFF), static_cast<UInt8>((arg1Mask >> 16) & 0xFF),
                static_cast<UInt8>((arg1Mask >> 24) & 0xFF), 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00};
            size_t dataOffset = 0;
            if (!KernelPatcher::findPattern(find, mask, arrsize(find), reinterpret_cast<void *>(slide), size,
                    &dataOffset)) {
                const UInt8 find[] = {0x48, 0x8D, 0x35, 0x00, 0x00, 0x00, 0x00, 0xBA, static_cast<UInt8>(arg1 & 0xFF),
                    static_cast<UInt8>((arg1 >> 8) & 0xFF), static_cast<UInt8>((arg1 >> 16) & 0xFF),
                    static_cast<UInt8>((arg1 >> 24) & 0xFF), 0xE8, 0x00, 0x00, 0x00, 0x00, 0x40};
                const UInt8 mask[] = {0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF,
                    static_cast<UInt8>(arg1Mask & 0xFF), static_cast<UInt8>((arg1Mask >> 8) & 0xFF),
                    static_cast<UInt8>((arg1Mask >> 16) & 0xFF), static_cast<UInt8>((arg1Mask >> 24) & 0xFF), 0xFF,
                    0x00, 0x00, 0x00, 0x00, 0xF0};
                PANIC_COND(!KernelPatcher::findPattern(find, mask, arrsize(find), reinterpret_cast<void *>(slide), size,
                               &dataOffset),
                    "HWLibs", "Failed to find memcpy block 0x%04X&0x%04X", arg1, arg1Mask);
                auto block = slide + dataOffset;
                //! 0x0 lea rsi, [rel ...]
                //! movabs rsi, 0x<FAKECPY_FUNC_ADDR>
                *reinterpret_cast<UInt16 *>(block) = 0xBE48;
                *reinterpret_cast<UInt64 *>(block + 2) = reinterpret_cast<UInt64>(func);
                *reinterpret_cast<UInt16 *>(block + 10) = 0xD6FF;    //! call rsi
                //! 0xC call _memcpy
                *reinterpret_cast<UInt32 *>(block + 12) = 0x66906690;    //! nop nop
                *reinterpret_cast<UInt16 *>(block + 15) = 0x6690;        //! nop
                //! 0xF lea, mov, etc.
                return;
            }
            auto block = slide + dataOffset;
            //! 0x0 lea rsi, [rel ...]
            *reinterpret_cast<UInt16 *>(block) = 0xBE48;                                //! movabs rsi,
            *reinterpret_cast<UInt64 *>(block + 2) = reinterpret_cast<UInt64>(func);    //! 0x<FAKECPY_FUNC_ADDR>
            *reinterpret_cast<UInt16 *>(block + 10) = 0x6690;                           //! nop
            //! 0xC mov rdi, ...
            //! 0xF call _memcpy
            *reinterpret_cast<UInt16 *>(block + 15) = 0xD6FF;    //! call rsi
            *reinterpret_cast<UInt16 *>(block + 17) = 0x6690;    //! nop
            *reinterpret_cast<UInt8 *>(block + 19) = 0x90;       //! nop
        };

        PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "HWLibs",
            "Failed to enable kernel writing");
        switch (NootRXMain::callback->chipType) {
            case ChipType::Navi21:
                hijackMemCpyBlock(0x00001310, 0xFFFFFFFF, fakecpyNavi21Kdb);
                hijackMemCpyBlock(0x00014350, 0xFFFFFFFF, fakecpyNavi21Sos);
                hijackMemCpyBlock(0x00000770, 0xFFFC0FFF, fakecpyNavi21SysDrv);
                hijackMemCpyBlock(0x000003A0, 0xFFFFFFFF, fakecpyNavi21TosSpl);
                break;
            case ChipType::Navi22:
                hijackMemCpyBlock(0x00001070, 0xFFFFFFFF, fakecpyNavi22Kdb);
                hijackMemCpyBlock(0x00014350, 0xFFFFFFFF, fakecpyNavi22Sos);
                hijackMemCpyBlock(0x00010790, 0xFFFF0FFF, fakecpyNavi22SysDrv);
                hijackMemCpyBlock(0x000003A0, 0xFFFFFFFF, fakecpyNavi22TosSpl);
                break;
            case ChipType::Navi23:
                hijackMemCpyBlock(0x00001070, 0xFFFFFFFF, fakecpyNavi23Kdb);
                hijackMemCpyBlock(0x00014350, 0xFFFFFFFF, fakecpyNavi23Sos);
                hijackMemCpyBlock(0x00010790, 0xFFFF0FFF, fakecpyNavi23SysDrv);
                hijackMemCpyBlock(0x000003A0, 0xFFFFFFFF, fakecpyNavi23TosSpl);
                break;
            default:
                break;
        }
        MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
        DBGLOG("HWLibs", "Applied PSP memcpy firmware patches");

        if (NootRXMain::callback->chipType == ChipType::Navi22) {
            const LookupPatchPlus patches[] = {
                {&kextRadeonX6810HWLibs, kGcSwInitOriginal, kGcSwInitOriginalMask, kGcSwInitPatched,
                    kGcSwInitPatchedMask, 1},
                {&kextRadeonX6810HWLibs, kSmu1107CheckFwVersionOriginal, kSmu1107CheckFwVersionOriginalMask,
                    kSmu1107CheckFwVersionPatched, kSmu1107CheckFwVersionPatchedMask, 1},
            };
            PANIC_COND(!LookupPatchPlus::applyAll(patcher, patches, slide, size), "HWLibs",
                "Failed to apply Navi 22 patches (all vers)");
            if (sonoma144) {
                const LookupPatchPlus patches[] = {
                    {&kextRadeonX6810HWLibs, kGcSetFwEntryInfoOriginal14_4, kGcSetFwEntryInfoOriginalMask14_4,
                        kGcSetFwEntryInfoPatched14_4, kGcSetFwEntryInfoPatchedMask14_4, 1},
                    {&kextRadeonX6810HWLibs, kPspSwInit1Original14_4, kPspSwInit1OriginalMask14_4,
                        kPspSwInit1Patched14_4, kPspSwInit1PatchedMask14_4, 1},
                    {&kextRadeonX6810HWLibs, kPspSwInit2Original14_4, kPspSwInit2OriginalMask14_4,
                        kPspSwInit2Patched14_4, kPspSwInit2PatchedMask14_4, 1},
                    {&kextRadeonX6810HWLibs, kPspSwInit3Original14_4, kPspSwInit3Patched14_4, 1},
                    {&kextRadeonX6810HWLibs, kSdmaInitFunctionPointerOriginal14_4,
                        kSdmaInitFunctionPointerOriginalMask14_4, kSdmaInitFunctionPointerPatched14_4, 1},
                };
                PANIC_COND(!LookupPatchPlus::applyAll(patcher, patches, slide, size), "HWLibs",
                    "Failed to apply Navi 22 patches (>=14.4)");
            } else {
                const LookupPatchPlus patches[] = {
                    {&kextRadeonX6810HWLibs, kGcSetFwEntryInfoOriginal, kGcSetFwEntryInfoOriginalMask,
                        kGcSetFwEntryInfoPatched, kGcSetFwEntryInfoPatchedMask, 1},
                    {&kextRadeonX6810HWLibs, kPspSwInit1Original, kPspSwInit1OriginalMask, kPspSwInit1Patched,
                        kPspSwInit1PatchedMask, 1},
                    {&kextRadeonX6810HWLibs, kPspSwInit2Original, kPspSwInit2OriginalMask, kPspSwInit2Patched,
                        kPspSwInit2PatchedMask, 1},
                    {&kextRadeonX6810HWLibs, kPspSwInit3Original, kPspSwInit3OriginalMask, kPspSwInit3Patched,
                        kPspSwInit3PatchedMask, 1},
                };
                PANIC_COND(!LookupPatchPlus::applyAll(patcher, patches, slide, size), "HWLibs",
                    "Failed to apply Navi 22 patches (<14.4)");
                if (getKernelVersion() >= KernelVersion::Ventura) {
                    const LookupPatchPlus patch {&kextRadeonX6810HWLibs, kSdmaInitFunctionPointerOriginal,
                        kSdmaInitFunctionPointerOriginalMask, kSdmaInitFunctionPointerPatched, 1};
                    PANIC_COND(!patch.apply(patcher, slide, size), "HWLibs",
                        "Failed to apply Navi 22 Ventura SDMA patch");
                }
            }
        }
        return true;
    }

    return false;
}

const char *HWLibs::wrapGetMatchProperty() {
    if (NootRXMain::callback->chipType == ChipType::Navi21) { return "Load6800"; }
    return "Load6810";
}

CAILResult HWLibs::wrapPspCmdKmSubmit(void *ctx, void *cmd, void *param3, void *param4) {
    char filename[128];
    bzero(filename, sizeof(filename));
    auto &size = getMember<UInt32>(cmd, 0xC);
    auto cmdID = getMember<AMDPSPCommand>(cmd, 0x0);
    size_t off;
    switch (getKernelVersion()) {
        case KernelVersion::BigSur... KernelVersion::Monterey:
            off = 0xAF8;
            break;
        case KernelVersion::Ventura... KernelVersion::Sequoia:
            off = NootRXMain::callback->chipType == ChipType::Navi21 ? 0xAF8 : 0xB48;
            break;
        default:
            PANIC("HWLibs", "Unsupported kernel version %d", getKernelVersion());
    }
    auto *data = getMember<UInt8 *>(ctx, off);

    switch (cmdID) {
        case kPSPCommandLoadTA: {
            const char *name = reinterpret_cast<char *>(data + 0x8DB);
            if (!strncmp(name, "AMD DTM Application", 20)) {
                strncpy(filename, "psp_dtm.bin", 12);
                break;
            }
            if (!strncmp(name, "AMD RAP Application", 20)) {
                strncpy(filename, "psp_rap.bin", 12);
                break;
            }
            if (!strncmp(name, "AMD HDCP Application", 21)) {
                strncpy(filename, "psp_hdcp.bin", 13);
                break;
            }
            if (!strncmp(name, "AMD AUC Application", 20)) {
                strncpy(filename, "psp_auc.bin", 12);
                break;
            }
            if (!strncmp(name, "AMD FP Application", 19)) {
                strncpy(filename, "psp_fp.bin", 11);
                break;
            }

            return FunctionCast(wrapPspCmdKmSubmit, callback->orgPspCmdKmSubmit)(ctx, cmd, param3, param4);
        }
        case kPSPCommandLoadASD: {
            strncpy(filename, "psp_asd.bin", 12);
            break;
        }
        case kPSPCommandLoadIPFW: {
            auto *prefix = NootRXMain::getGCPrefix();
            auto uCodeID = getMember<AMDUCodeID>(cmd, 0x10);
            switch (uCodeID) {
                case kUCodeSMU:
                    switch (NootRXMain::callback->chipType) {
                        case ChipType::Navi21:
                            // strncpy(filename, "navi21_smc_firmware.bin", 24);
                            // break;
                            return FunctionCast(wrapPspCmdKmSubmit, callback->orgPspCmdKmSubmit)(ctx, cmd, param3,
                                param4);
                        case ChipType::Navi22:
                            strncpy(filename, "navi22_smc_firmware.bin", 24);
                            break;
                        case ChipType::Navi23:
                            strncpy(filename, "navi23_smc_firmware.bin", 24);
                            break;
                        default:
                            PANIC("HWLibs", "Unknown chip type");
                    }
                    break;
                case kUCodeCE:
                    snprintf(filename, sizeof(filename), "%sce_ucode.bin", prefix);
                    break;
                case kUCodePFP:
                    snprintf(filename, sizeof(filename), "%spfp_ucode.bin", prefix);
                    break;
                case kUCodeME:
                    snprintf(filename, sizeof(filename), "%sme_ucode.bin", prefix);
                    break;
                case kUCodeMEC1:
                    snprintf(filename, sizeof(filename), "%smec_ucode.bin", prefix);
                    break;
                case kUCodeMEC2:
                    snprintf(filename, sizeof(filename), "%smec_ucode.bin", prefix);
                    break;
                case kUCodeMEC1JT:
                    snprintf(filename, sizeof(filename), "%smec_jt_ucode.bin", prefix);
                    break;
                case kUCodeMEC2JT:
                    snprintf(filename, sizeof(filename), "%smec_jt_ucode.bin", prefix);
                    break;
                // case kUCodeMES:
                //     strncpy(filename, "mes_10_3_mes0_ucode.bin", 24);
                //     break;
                // case kUCodeMESStack:
                //     strncpy(filename, "mes_10_3_mes0_data.bin", 23);
                //     break;
                case kUCodeRLC:
                    snprintf(filename, sizeof(filename), "%srlc_ucode.bin", prefix);
                    break;
                case kUCodeSDMA0:
                    switch (NootRXMain::callback->chipType) {
                        case ChipType::Navi21:
                            strncpy(filename, "sdma_5_2_ucode.bin", 19);
                            break;
                        case ChipType::Navi22:
                            strncpy(filename, "sdma_5_2_2_ucode.bin", 21);
                            break;
                        case ChipType::Navi23:
                            strncpy(filename, "sdma_5_2_4_ucode.bin", 21);
                            break;
                        default:
                            PANIC("HWLibs", "Unknown chip type");
                    }
                    break;
                case kUCodeVCN0:
                    strncpy(filename, "ativvaxy_vcn3.dat", 18);
                    break;
                case kUCodeVCN1:
                    strncpy(filename, "ativvaxy_vcn3.dat", 18);
                    break;
                case kUCodeRLCP:
                    snprintf(filename, sizeof(filename), "%srlcp_ucode.bin", prefix);
                    break;
                case kUCodeRLCSRListGPM:
                    snprintf(filename, sizeof(filename), "%srlc_srlist_gpm_mem.bin", prefix);
                    break;
                case kUCodeRLCSRListSRM:
                    snprintf(filename, sizeof(filename), "%srlc_srlist_srm_mem.bin", prefix);
                    break;
                case kUCodeRLCSRListCntl:
                    snprintf(filename, sizeof(filename), "%srlc_srlist_cntl.bin", prefix);
                    break;
                case kUCodeRLCLX6Iram:
                    snprintf(filename, sizeof(filename), "%srlc_lx6_iram_ucode.bin", prefix);
                    break;
                case kUCodeRLCLX6Dram:
                    snprintf(filename, sizeof(filename), "%srlc_lx6_dram_ucode.bin", prefix);
                    break;
                case kUCodeGlobalTapDelays:
                    snprintf(filename, sizeof(filename), "%sglobal_tap_delays.bin", prefix);
                    break;
                case kUCodeSE0TapDelays:
                    snprintf(filename, sizeof(filename), "%sse0_tap_delays.bin", prefix);
                    break;
                case kUCodeSE1TapDelays:
                    snprintf(filename, sizeof(filename), "%sse1_tap_delays.bin", prefix);
                    break;
                case kUCodeSE2TapDelays:
                    snprintf(filename, sizeof(filename), "%sse2_tap_delays.bin", prefix);
                    break;
                case kUCodeSE3TapDelays:
                    snprintf(filename, sizeof(filename), "%sse3_tap_delays.bin", prefix);
                    break;
                case kUCodeDMCUB:
                    switch (NootRXMain::callback->chipType) {
                        case ChipType::Navi21:
                            [[fallthrough]];
                        case ChipType::Navi22:
                            strncpy(filename, "atidmcub_instruction_dcn30.bin", 31);
                            break;
                        case ChipType::Navi23:
                            strncpy(filename, "atidmcub_instruction_dcn302.bin", 32);
                            break;
                        default:
                            PANIC("HWLibs", "Unknown chip type");
                    }
                    break;
                default:
                    return FunctionCast(wrapPspCmdKmSubmit, callback->orgPspCmdKmSubmit)(ctx, cmd, param3, param4);
            }
            break;
        }
        default:
            return FunctionCast(wrapPspCmdKmSubmit, callback->orgPspCmdKmSubmit)(ctx, cmd, param3, param4);
    }

    const auto &fw = getFWByName(filename);
    memcpy(data, fw.data, fw.length);
    size = fw.length;

    return FunctionCast(wrapPspCmdKmSubmit, callback->orgPspCmdKmSubmit)(ctx, cmd, param3, param4);
}

CAILResult HWLibs::wrapSmu1107SendMessageWithParameter(void *smum, UInt32 msgId, UInt32 param) {
    if (param == 0x10000 && (msgId == 0x2A || msgId == 0x2B)) { return kCAILResultSuccess; }
    return FunctionCast(wrapSmu1107SendMessageWithParameter, callback->orgSmu1107SendMessageWithParameter)(smum, msgId,
        param);
}
