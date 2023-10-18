//! Copyright © 2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5.
//! See LICENSE for details.

#include "HWLibs.hpp"
#include "Firmware.hpp"
#include "NootRX.hpp"
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

#define DEF_FAKECPY(ident, filename)              \
    extern "C" void ident(void *dest) {           \
        auto &fwDesc = getFWDescByName(filename); \
        memcpy(dest, fwDesc.data, fwDesc.size);   \
    }

DEF_FAKECPY(fakecpyNavi22Kdb, "psp_key_database_navi22.bin")
DEF_FAKECPY(fakecpyNavi22Sos, "psp_sos_navi22.bin");
DEF_FAKECPY(fakecpyNavi22SysDrv, "psp_sys_drv_navi22.bin");
DEF_FAKECPY(fakecpyNavi22TosSpl, "psp_tos_spl_navi22.bin");

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

        if (NootRXMain::callback->chipType == ChipType::Navi22) {
            RouteRequestPlus requests[] = {
                {"_psp_cmd_km_submit", wrapPspCmdKmSubmit, this->orgPspCmdKmSubmit, kPspCmdKmSubmitPattern,
                    kPspCmdKmSubmitMask},
                {"_smu_11_0_7_send_message_with_parameter", wrapSmu1107SendMessageWithParameter,
                    this->orgSmu1107SendMessageWithParameter, kSmu1107SendMessageWithParameterPattern,
                    kSmu1107SendMessageWithParameterPatternMask},
            };
            PANIC_COND(!RouteRequestPlus::routeAll(patcher, id, requests, slide, size), "HWLibs",
                "Failed to route Navi 22 symbols");
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
                if (NootRXMain::callback->chipType == ChipType::Navi22) {
                    orgDevCapTable->asicGoldenSettings->goldenSettings = goldenSettingsNavi22;
                }
                break;
            }
        }
        PANIC_COND(!orgDevCapTable->familyId, "HWLibs", "Failed to find device capability table entry");
        MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
        DBGLOG("HWLibs", "Applied DDI Caps patches");

        if (NootRXMain::callback->chipType == ChipType::Navi22) {
            const LookupPatchPlus patches[] = {
                {&kextRadeonX6810HWLibs, kGcSwInitOriginal, kGcSwInitOriginalMask, kGcSwInitPatched,
                    kGcSwInitPatchedMask, 1},
                {&kextRadeonX6810HWLibs, kGcSetFwEntryInfoOriginal, kGcSetFwEntryInfoOriginalMask,
                    kGcSetFwEntryInfoPatched, kGcSetFwEntryInfoPatchedMask, 1},
                {&kextRadeonX6810HWLibs, kPspSwInit1Original, kPspSwInit1OriginalMask, kPspSwInit1Patched,
                    kPspSwInit1PatchedMask, 1},
                {&kextRadeonX6810HWLibs, kPspSwInit2Original, kPspSwInit2OriginalMask, kPspSwInit2Patched,
                    kPspSwInit2PatchedMask, 1},
                {&kextRadeonX6810HWLibs, kPspSwInit3Original, kPspSwInit3OriginalMask, kPspSwInit3Patched,
                    kPspSwInit3PatchedMask, 1},
                {&kextRadeonX6810HWLibs, kSmu1107CheckFwVersionOriginal, kSmu1107CheckFwVersionOriginalMask,
                    kSmu1107CheckFwVersionPatched, kSmu1107CheckFwVersionPatchedMask, 1},
            };
            PANIC_COND(!LookupPatchPlus::applyAll(patcher, patches, slide, size), "HWLibs",
                "Failed to apply Navi 22 patches");
            if (getKernelVersion() >= KernelVersion::Ventura) {
                const LookupPatchPlus patch {&kextRadeonX6810HWLibs, kSdmaInitFunctionPointerOriginal,
                    kSdmaInitFunctionPointerOriginalMask, kSdmaInitFunctionPointerPatched, 1};
                PANIC_COND(!patch.apply(patcher, slide, size), "HWLibs",
                    "Failed to apply Navi 22 Ventura SDMA patch");
            }

            auto findMemCpyBlock = [=](UInt32 arg1, UInt32 arg1Mask) {
                const UInt8 find[] = {0x48, 0x8D, 0x35, 0x00, 0x00, 0x00, 0x00, 0xBA, static_cast<UInt8>(arg1 & 0xFF),
                    static_cast<UInt8>((arg1 >> 8) & 0xFF), static_cast<UInt8>((arg1 >> 16) & 0xFF),
                    static_cast<UInt8>((arg1 >> 24) & 0xFF), 0x4C, 0x89, 0x00, 0xE8, 0x00, 0x00, 0x00, 0x00};
                const UInt8 mask[] = {0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF,
                    static_cast<UInt8>(arg1Mask & 0xFF), static_cast<UInt8>((arg1Mask >> 8) & 0xFF),
                    static_cast<UInt8>((arg1Mask >> 16) & 0xFF), static_cast<UInt8>((arg1Mask >> 24) & 0xFF), 0xFF,
                    0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00};
                size_t dataOffset;
                PANIC_COND(!KernelPatcher::findPattern(find, mask, arrsize(find), reinterpret_cast<void *>(slide), size,
                               &dataOffset),
                    "HWLibs", "Failed to find memcpy block 0x%X&0x%X", arg1, arg1Mask);
                return slide + dataOffset;
            };
            //! movabs rsi, ident
            //! nop
            //! mov rdi, r*
            //! call rsi
            //! nop
            //! nop
#define HIJACK_MEMCPY(arg1, arg1Mask, ident)                                      \
    do {                                                                          \
        auto block = findMemCpyBlock(arg1, arg1Mask);                             \
        *reinterpret_cast<UInt16 *>(block) = 0xBE48;                              \
        *reinterpret_cast<UInt64 *>(block + 2) = reinterpret_cast<UInt64>(ident); \
        *reinterpret_cast<UInt16 *>(block + 10) = 0x6690;                         \
        *reinterpret_cast<UInt16 *>(block + 15) = 0xD6FF;                         \
        *reinterpret_cast<UInt16 *>(block + 17) = 0x6690;                         \
        *reinterpret_cast<UInt8 *>(block + 19) = 0x90;                            \
    } while (0)
            PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "HWLibs",
                "Failed to enable kernel writing");
            HIJACK_MEMCPY(0x00001070, 0xFFFFFFFF, fakecpyNavi22Kdb);
            HIJACK_MEMCPY(0x00014350, 0xFFFFFFFF, fakecpyNavi22Sos);
            HIJACK_MEMCPY(0x00010790, 0xFFFF0FFF, fakecpyNavi22SysDrv);
            HIJACK_MEMCPY(0x000003A0, 0xFFFFFFFF, fakecpyNavi22TosSpl);
            MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
#undef HIJACK_MEMCPY
        }

        return true;
    }

    return false;
}

const char *HWLibs::wrapGetMatchProperty() {
    if (NootRXMain::callback->chipType == ChipType::Navi21) {
        return "Load6800";
    }
    return "Load6810";
}

CAILResult HWLibs::wrapPspCmdKmSubmit(void *ctx, void *cmd, void *param3, void *param4) {
    char filename[128] = {0};
    auto &size = getMember<UInt32>(ctx, 0xC);
    auto cmdID = getMember<AMDPSPCommand>(cmd, 0x0);
    size_t off;
    switch (getKernelVersion()) {
        case KernelVersion::BigSur ... KernelVersion::Monterey:
            off = 0xAF8;
            break;
        default:
            off = 0xB48;
            break;
    }
    auto *data = getMember<UInt8 *>(ctx, off);

    switch (cmdID) {
        case kPSPCommandLoadTA: {
            const char *name = reinterpret_cast<char *>(data + 0x8DB);
            if (!strncmp(name, "AMD DTM Application", 20)) {
                DBGLOG("HWLibs", "DTM is being loaded (size: 0x%X)", size);
                strncpy(filename, "psp_dtm.bin", 12);
                break;
            }
            if (!strncmp(name, "AMD RAP Application", 20)) {
                DBGLOG("HWLibs", "RAP is being loaded (size: 0x%X)", size);
                strncpy(filename, "psp_rap.bin", 12);
                break;
            }
            if (!strncmp(name, "AMD HDCP Application", 21)) {
                DBGLOG("HWLibs", "HDCP is being loaded (size: 0x%X)", size);
                strncpy(filename, "psp_hdcp.bin", 13);
                break;
            }
            if (!strncmp(name, "AMD AUC Application", 20)) {
                DBGLOG("HWLibs", "AUC is being loaded (size: 0x%X)", size);
                strncpy(filename, "psp_auc.bin", 12);
                break;
            }
            if (!strncmp(name, "AMD FP Application", 19)) {
                DBGLOG("HWLibs", "FP is being loaded (size: 0x%X)", size);
                strncpy(filename, "psp_fp.bin", 11);
                break;
            }

            DBGLOG("HWLibs", "Other PSP TA is being loaded: (name: %s size: 0x%X)", data + 0x8DB, size);
            return FunctionCast(wrapPspCmdKmSubmit, callback->orgPspCmdKmSubmit)(ctx, cmd, param3, param4);
        }

        case kPSPCommandLoadASD: {
            DBGLOG("HWLibs", "ASD is being loaded (size: 0x%X)", size);
            strncpy(filename, "psp_asd.bin", 12);
            break;
        }

        case kPSPCommandLoadIPFW: {
            auto *prefix = NootRXMain::getGCPrefix();
            auto uCodeID = getMember<UInt32>(ctx, 0x10);
            switch (uCodeID) {
                case kUCodeSMU:
                    DBGLOG("HWLibs", "SMU is being loaded (size: 0x%X)", size);
                    strncpy(filename, "navi22_smc_firmware.bin", 24);
                    break;
                case kUCodeCE:
                    DBGLOG("HWLibs", "CE is being loaded (size: 0x%X)", size);
                    snprintf(filename, 128, "%sce_ucode.bin", prefix);
                    break;
                case kUCodePFP:
                    DBGLOG("HWLibs", "PFP is being loaded (size: 0x%X)", size);
                    snprintf(filename, 128, "%spfp_ucode.bin", prefix);
                    break;
                case kUCodeME:
                    DBGLOG("HWLibs", "ME is being loaded (size: 0x%X)", size);
                    snprintf(filename, 128, "%sme_ucode.bin", prefix);
                    break;
                case kUCodeMEC1:
                    DBGLOG("HWLibs", "MEC1 is being loaded (size: 0x%X)", size);
                    snprintf(filename, 128, "%smec_ucode.bin", prefix);
                    break;
                case kUCodeMEC2:
                    DBGLOG("HWLibs", "MEC2 is being loaded (size: 0x%X)", size);
                    snprintf(filename, 128, "%smec_ucode.bin", prefix);
                    break;
                case kUCodeMEC1JT:
                    DBGLOG("HWLibs", "MEC1 JT is being loaded (size: 0x%X)", size);
                    snprintf(filename, 128, "%smec_jt_ucode.bin", prefix);
                    break;
                case kUCodeMEC2JT:
                    DBGLOG("HWLibs", "MEC2 JT is being loaded (size: 0x%X)", size);
                    snprintf(filename, 128, "%smec_jt_ucode.bin", prefix);
                    break;
                case kUCodeMES:
                    DBGLOG("HWLibs", "MES is being loaded (size: 0x%X)", size);
                    snprintf(filename, 128, "%smes_ucode.bin", prefix);
                    break;
                case kUCodeMESStack:
                    DBGLOG("HWLibs", "MES STACK is being loaded (size: 0x%X)", size);
                    snprintf(filename, 128, "%smes_stack_data.bin", prefix);
                    break;
                case kUCodeRLC:
                    DBGLOG("HWLibs", "RLC is being loaded (size: 0x%X)", size);
                    snprintf(filename, 128, "%srlc_ucode.bin", prefix);
                    break;
                case kUCodeSDMA0:
                    DBGLOG("HWLibs", "SDMA0 is being loaded (size: 0x%X)", size);
                    strncpy(filename, "sdma_5_2_2_ucode.bin", 21);
                    break;
                case kUCodeVCN0:
                    DBGLOG("HWLibs", "VCN0 is being loaded (size: 0x%X)", size);
                    strncpy(filename, "ativvaxy_vcn3.dat", 18);
                    break;
                case kUCodeRLCP:
                    DBGLOG("HWLibs", "RLC P is being loaded (size: 0x%X)", size);
                    snprintf(filename, 128, "%srlcp_ucode.bin", prefix);
                    break;
                case kUCodeRLCSRListGPM:
                    DBGLOG("HWLibs", "RLC SRList GPM is being loaded (size: 0x%X)", size);
                    snprintf(filename, 128, "%srlc_srlist_gpm_mem_ucode.bin", prefix);
                    break;
                case kUCodeRLCSRListSRM:
                    DBGLOG("HWLibs", "RLC SRList SRM is being loaded (size: 0x%X)", size);
                    snprintf(filename, 128, "%srlc_srlist_srm_mem_ucode.bin", prefix);
                    break;
                case kUCodeRLCSRListCntl:
                    DBGLOG("HWLibs", "RLC SRList Cntl is being loaded (size: 0x%X)", size);
                    snprintf(filename, 128, "%srlc_srlist_cntl_ucode.bin", prefix);
                    break;
                case kUCodeRLCLX6Iram:
                    DBGLOG("HWLibs", "RLC LX6 IRAM is being loaded (size: 0x%X)", size);
                    snprintf(filename, 128, "%srlc_lx6_iram_ucode.bin", prefix);
                    break;
                case kUCodeRLCLX6Dram:
                    DBGLOG("HWLibs", "RLC LX6 DRAM is being loaded (size: 0x%X)", size);
                    snprintf(filename, 128, "%srlc_lx6_dram_ucode.bin", prefix);
                    break;
                case kUCodeGlobalTapDelays:
                    DBGLOG("HWLibs", "Global Tap Delays is being loaded (size: 0x%X)", size);
                    snprintf(filename, 128, "%sglobal_tap_delays.bin", prefix);
                    break;
                case kUCodeSE0TapDelays:
                    DBGLOG("HWLibs", "SE0 Tap Delays is being loaded (size: 0x%X)", size);
                    snprintf(filename, 128, "%sse0_tap_delays.bin", prefix);
                    break;
                case kUCodeSE1TapDelays:
                    DBGLOG("HWLibs", "SE1 Tap Delays is being loaded (size: 0x%X)", size);
                    snprintf(filename, 128, "%sse1_tap_delays.bin", prefix);
                    break;
                case kUCodeSE2TapDelays:
                    DBGLOG("HWLibs", "SE2 Tap Delays is being loaded (size: 0x%X)", size);
                    snprintf(filename, 128, "%sse2_tap_delays.bin", prefix);
                    break;
                case kUCodeSE3TapDelays:
                    DBGLOG("HWLibs", "SE3 Tap Delays is being loaded (size: 0x%X)", size);
                    snprintf(filename, 128, "%sse3_tap_delays.bin", prefix);
                    break;
                case kUCodeDMCUB:
                    DBGLOG("HWLibs", "DMCUB is being loaded (size: 0x%X)", size);
                    strncpy(filename, "atidmcub_instruction_dcn30.bin", 31);
                    break;
                case kUCodeVCNSram:
                    DBGLOG("HWLibs", "VCN SRAM is being loaded (size: 0x%X)", size);
                    return FunctionCast(wrapPspCmdKmSubmit, callback->orgPspCmdKmSubmit)(ctx, cmd, param3, param4);
                default:
                    DBGLOG("HWLibs", "0x%X is being loaded (size: 0x%X)", uCodeID, size);
                    return FunctionCast(wrapPspCmdKmSubmit, callback->orgPspCmdKmSubmit)(ctx, cmd, param3, param4);
            }
            break;
        }
        default:
            DBGLOG("HWLibs", "Not hijacking command id 0x%X", cmdID);
            return FunctionCast(wrapPspCmdKmSubmit, callback->orgPspCmdKmSubmit)(ctx, cmd, param3, param4);
    }

    auto &fwDesc = getFWDescByName(filename);
    memcpy(data, fwDesc.data, fwDesc.size);
    size = fwDesc.size;

    return FunctionCast(wrapPspCmdKmSubmit, callback->orgPspCmdKmSubmit)(ctx, cmd, param3, param4);
}

CAILResult HWLibs::wrapSmu1107SendMessageWithParameter(void *smum, UInt32 msgId, UInt32 param) {
    if (param == 0x10000 && (msgId == 0x2A || msgId == 0x2B)) {
        DBGLOG("HWLibs", "Skipped VCN1 PG command (msgId: 0x%X)", msgId);
        return kCAILResultSuccess;
    }
    return FunctionCast(wrapSmu1107SendMessageWithParameter, callback->orgSmu1107SendMessageWithParameter)(smum, msgId,
        param);
}
