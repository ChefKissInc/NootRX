//  Copyright © 2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5. See LICENSE for
//  details.

#include "kern_hwlibs.hpp"
#include "kern_fw.hpp"
#include "kern_patches.hpp"
#include "kern_patterns.hpp"
#include "kern_x6000p.hpp"
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

bool HWLibs::processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size) {
    if (kextRadeonX6000HWServices.loadIndex == id) {
        X6000P::callback->setRMMIOIfNecessary();

        RouteRequestPlus requests[] = {
            {"__ZN38AMDRadeonX6000_AMDRadeonHWServicesNavi16getMatchPropertyEv", wrapGetMatchProperty},
        };

        PANIC_COND(!RouteRequestPlus::routeAll(patcher, id, requests, slide, size), "hwservices",
            "Failed to route symbols");
    } else if ((kextRadeonX6810HWLibs.loadIndex == id) || (kextRadeonX6800HWLibs.loadIndex == id)) {
        X6000P::callback->setRMMIOIfNecessary();

        CAILAsicCapsEntry *orgCapsTable = nullptr;
        CAILDeviceTypeEntry *orgDeviceTypeTable = nullptr;
        DeviceCapabilityEntry *orgDevCapTable = nullptr;
        CAILAsicCapsInitEntry *orgCapsInitTable = nullptr;

        SolveRequestPlus solveRequests[] = {
            {"__ZL15deviceTypeTable", orgDeviceTypeTable, kDeviceTypeTablePattern},
            {"__ZL20CAIL_ASIC_CAPS_TABLE", orgCapsTable, kCailAsicCapsTableHWLibsPattern},
            {"_DeviceCapabilityTbl", orgDevCapTable, kDeviceCapabilityTblPattern},
        };
        PANIC_COND(!SolveRequestPlus::solveAll(patcher, id, solveRequests, slide, size), "hwlibs",
            "Failed to resolve symbols");
        SolveRequestPlus solveRequest {"_CAILAsicCapsInitTable", orgCapsInitTable, kCAILAsicCapsInitTablePattern};
        solveRequest.solve(patcher, id, slide, size);

        RouteRequestPlus requests[] = {
            {"_psp_cos_log", wrapPspCosLog, this->orgPspCosLog, kPspCosLogPattern},
            {"_psp_assertion", wrapIpAssertion},
        };
        SYSLOG_COND(!RouteRequestPlus::routeAll(patcher, id, requests, slide, size), "hwlibs",
            "Failed to route symbols");

        PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "hwlibs",
            "Failed to enable kernel writing");

        *orgDeviceTypeTable = {
            .deviceId = X6000P::callback->deviceId,
            .deviceType = (kextRadeonX6800HWLibs.loadIndex == id) ? 6U : 8,
        };

        bool found = false;
        uint32_t targetDeviceId = X6000P::callback->chipType == ChipType::Navi21 ? 0x73BF : 0x73FF;
        while (orgCapsTable->deviceId != 0xFFFFFFFF) {
            if (orgCapsTable->familyId == AMDGPU_FAMILY_NAVI && orgCapsTable->deviceId == targetDeviceId) {
                orgCapsTable->deviceId = X6000P::callback->deviceId;
                orgCapsTable->revision = X6000P::callback->revision;
                orgCapsTable->extRevision =
                    static_cast<uint32_t>(X6000P::callback->enumRevision) + X6000P::callback->revision;
                orgCapsTable->pciRevision = X6000P::callback->pciRevision;
                if (orgCapsInitTable) {
                    *orgCapsInitTable = {
                        .familyId = AMDGPU_FAMILY_NAVI,
                        .deviceId = X6000P::callback->deviceId,
                        .revision = X6000P::callback->revision,
                        .extRevision = static_cast<uint32_t>(orgCapsTable->extRevision),
                        .pciRevision = X6000P::callback->pciRevision,
                        .caps = orgCapsTable->caps,
                    };
                }
                found = true;
                break;
            }
            orgCapsTable++;
        }
        PANIC_COND(!found, "hwlibs", "Failed to find caps init table entry");

        found = false;
        while (orgDevCapTable->familyId) {
            if (orgDevCapTable->familyId == AMDGPU_FAMILY_NAVI && orgDevCapTable->deviceId == targetDeviceId) {
                orgDevCapTable->deviceId = X6000P::callback->deviceId;
                orgDevCapTable->extRevision =
                    static_cast<uint64_t>(X6000P::callback->enumRevision) + X6000P::callback->revision;
                orgDevCapTable->revision = DEVICE_CAP_ENTRY_REV_DONT_CARE;
                orgDevCapTable->enumRevision = DEVICE_CAP_ENTRY_REV_DONT_CARE;
                found = true;
                break;
            }
            orgDevCapTable++;
        }
        PANIC_COND(!found, "hwlibs", "Failed to find device capability table entry");
        MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
        DBGLOG("hwlibs", "Applied DDI Caps patches");

        if (X6000P::callback->chipType == ChipType::Navi22) {
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
                {&kextRadeonX6810HWLibs, getFWDescByName("psp_key_database_navi23.bin").data,
                    getFWDescByName("psp_key_database_navi22.bin").data, 4208, 1},
                {&kextRadeonX6810HWLibs, getFWDescByName("psp_spl_navi23.bin").data,
                    getFWDescByName("psp_spl_navi22.bin").data, 928, 1},
                {&kextRadeonX6810HWLibs, getFWDescByName("psp_sysdrv_navi23.bin").data,
									getFWDescByName("psp_sysdrv_navi22.bin").data, 82768, 1},
            };
            PANIC_COND(!LookupPatchPlus::applyAll(patcher, patches, slide, size), "hwlibs",
                "Failed to apply patches: %d", patcher.getError());
        }

        return true;
    }

    return false;
}

const char *HWLibs::wrapGetMatchProperty() {
    if (X6000P::callback->chipType == ChipType::Navi21) {
        DBGLOG("hwservices", "Forced X6800HWLibs");
        return "Load6800";
    } else {
        DBGLOG("hwservices", "Forced X6810HWLibs");
        return "Load6810";
    }
}

void HWLibs::wrapIpAssertion([[maybe_unused]] void *data, uint32_t cond, char *func, char *file, uint32_t line,
    char *msg) {
    if (!cond) { kprintf("HWLibs assertion failed: %s %s %d %s", func, file, line, msg); }
}

void HWLibs::wrapPspCosLog(void *pspData, uint32_t param2, uint64_t param3, uint32_t param4, char *param5) {
    if (param5) {
        kprintf("AMD TTL COS: %s", param5);
        auto len = strlen(param5);
        if (len > 0 && param5[len - 1] != '\n') { kprintf("\n"); }
    }
    FunctionCast(wrapPspCosLog, callback->orgPspCosLog)(pspData, param2, param3, param4, param5);
}
