//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#include "kern_hwlibs.hpp"
#include "kern_patterns.hpp"
#include "kern_x6000p.hpp"
#include <Headers/kern_api.hpp>

static const char *pathRadeonX6810HWLibs = "/System/Library/Extensions/AMDRadeonX6000HWServices.kext/Contents/PlugIns/"
                                           "AMDRadeonX6810HWLibs.kext/Contents/MacOS/AMDRadeonX6810HWLibs";

static KernelPatcher::KextInfo kextRadeonX6810HWLibs {"com.apple.kext.AMDRadeonX6800HWLibs", &pathRadeonX6810HWLibs, 1,
    {}, {}, KernelPatcher::KextInfo::Unloaded};

HWLibs *HWLibs::callback = nullptr;

void HWLibs::init() {
    callback = this;
    lilu.onKextLoadForce(&kextRadeonX6810HWLibs);
}

bool HWLibs::processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size) {
    if (kextRadeonX6810HWLibs.loadIndex == id) {
        X6000P::callback->setRMMIOIfNecessary();

        CAILAsicCapsEntry *orgCapsTable = nullptr;
        CAILAsicCapsInitEntry *orgCapsInitTable = nullptr;

        SolveRequestPlus solveRequests[] = {
            {"__ZL20CAIL_ASIC_CAPS_TABLE", orgCapsTable, kCailAsicCapsTableHWLibsPattern},
            {"_CAILAsicCapsInitTable", orgCapsInitTable, kCAILAsicCapsInitTablePattern},
        };
        PANIC_COND(!SolveRequestPlus::solveAll(patcher, id, solveRequests, slide, size), "hwlibs",
            "Failed to resolve symbols");

        PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "hwlibs",
            "Failed to enable kernel writing");

        auto found = false;
        auto targetDeviceId = X6000P::callback->deviceId;
        if (X6000P::callback->chipType == ChipType::Navi21) {targetDeviceId = 0x73BF;}
        else if (X6000P::callback->chipType == ChipType::Navi22) {targetDeviceId = 0x73DF;}
        else {targetDeviceId = 0x73FF;}

        while (orgCapsInitTable->deviceId != 0xFFFFFFFF) {
            if (orgCapsInitTable->familyId == 0x8F && orgCapsInitTable->deviceId == targetDeviceId) {
                orgCapsInitTable->deviceId = X6000P::callback->deviceId;
                orgCapsInitTable->revision = X6000P::callback->revision;
                orgCapsInitTable->extRevision =
                    static_cast<uint64_t>(X6000P::callback->enumRevision) + X6000P::callback->revision;
                orgCapsInitTable->pciRevision = X6000P::callback->pciRevision;
                *orgCapsTable = {
                    .familyId = 0x8F,
                    .deviceId = X6000P::callback->deviceId,
                    .revision = X6000P::callback->revision,
                    .extRevision = static_cast<uint32_t>(X6000P::callback->enumRevision) + X6000P::callback->revision,
                    .pciRevision = 0xFFFFFFFF,
                };
                found = true;
                break;
            }
            orgCapsInitTable++;
        }
        PANIC_COND(!found, "hwlibs", "Failed to find caps init table entry");

        MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
        DBGLOG("hwlibs", "Applied DDI Caps patches");

        return true;
    }

    return false;
}
