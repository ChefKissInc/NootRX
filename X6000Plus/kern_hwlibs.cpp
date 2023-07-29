//  Copyright © 2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5. See LICENSE for
//  details.

#include "kern_hwlibs.hpp"
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

        SolveRequestPlus solveRequests[] = {
            {"__ZL15deviceTypeTable", orgDeviceTypeTable, kDeviceTypeTablePattern},
            {"__ZL20CAIL_ASIC_CAPS_TABLE", orgCapsTable, kCailAsicCapsTableHWLibsPattern},
            {"_DeviceCapabilityTbl", orgDevCapTable, kDeviceCapabilityTblPattern},
        };
        PANIC_COND(!SolveRequestPlus::solveAll(patcher, id, solveRequests, slide, size), "hwlibs",
            "Failed to resolve symbols");

        PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "hwlibs",
            "Failed to enable kernel writing");

        *orgDeviceTypeTable = {
            .deviceId = X6000P::callback->deviceId,
            .deviceType = (kextRadeonX6800HWLibs.loadIndex == id) ? 6U : 8,
        };

        *orgCapsTable = {
            .familyId = 0x8F,
            // Navi 23 uses Navi 22 caps, we also assume the same for Navi 24 here
            .caps = X6000P::callback->chipType == ChipType::Navi21 ? ddiCapsNavi21 : ddiCapsNavi22,
            .deviceId = X6000P::callback->deviceId,
            .revision = X6000P::callback->revision,
            .extRevision = static_cast<uint32_t>(X6000P::callback->enumRevision) + X6000P::callback->revision,
            .pciRevision = X6000P::callback->pciRevision,
        };

        CAILAsicCapsInitEntry *orgCapsInitTable = nullptr;
        SolveRequestPlus solveRequest {"_CAILAsicCapsInitTable", orgCapsInitTable, kCAILAsicCapsInitTablePattern};
        solveRequest.solve(patcher, id, slide, size);
        if (orgCapsInitTable) {
            *orgCapsInitTable = {
                .familyId = 0x8F,
                // Ditto
                .caps = X6000P::callback->chipType == ChipType::Navi21 ? ddiCapsNavi21 : ddiCapsNavi22,
                .deviceId = X6000P::callback->deviceId,
                .revision = X6000P::callback->revision,
                .extRevision = static_cast<uint64_t>(X6000P::callback->enumRevision) + X6000P::callback->revision,
                .pciRevision = X6000P::callback->pciRevision,
            };
        }

        bool found = false;
        uint32_t targetDeviceId = X6000P::callback->chipType == ChipType::Navi21 ? 0x73BF :
                                  X6000P::callback->chipType == ChipType::Navi22 ? 0x73DF :
                                                                                   0x73FF;
        while (orgDevCapTable->familyId) {
            if (orgDevCapTable->familyId == 0x8F && orgDevCapTable->deviceId == targetDeviceId) {
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

        return true;
    }

    return false;
}

const char *HWLibs::wrapGetMatchProperty() {
    if (X6000P::callback->chipType < ChipType::Navi23) {
        DBGLOG("hwservices", "Forced X6800HWLibs");
        return "Load6800";
    } else {
        DBGLOG("hwservices", "Forced X6810HWLibs");
        return "Load6810";
    }
}
