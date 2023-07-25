//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#include "kern_x6000fb.hpp"
#include "kern_patterns.hpp"
#include "kern_x6000p.hpp"
#include <Headers/kern_api.hpp>

static const char *pathRadeonX6000Framebuffer =
    "/System/Library/Extensions/AMDRadeonX6000Framebuffer.kext/Contents/MacOS/AMDRadeonX6000Framebuffer";

static KernelPatcher::KextInfo kextRadeonX6000Framebuffer {"com.apple.kext.AMDRadeonX6000Framebuffer",
    &pathRadeonX6000Framebuffer, 1, {}, {}, KernelPatcher::KextInfo::Unloaded};

X6000FB *X6000FB::callback = nullptr;

void X6000FB::init() {
    callback = this;
    lilu.onKextLoadForce(&kextRadeonX6000Framebuffer);
}

void X6000FB::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
    if (kextRadeonX6000Framebuffer.loadIndex == index) {
        X6000P::callback->setRMMIOIfNecessary();
        CAILAsicCapsEntry *orgAsicCapsTable = nullptr;

        SolveRequestPlus solveRequests[] = {
            {"__ZL20CAIL_ASIC_CAPS_TABLE", orgAsicCapsTable, kCailAsicCapsTablePattern},
        };
        PANIC_COND(!SolveRequestPlus::solveAll(&patcher, index, solveRequests, address, size), "x6000fb",
            "Failed to resolve symbols");

        PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "x6000fb",
            "Failed to enable kernel writing");

        *orgAsicCapsTable = {
            .familyId = 0x8F,
            .caps = X6000P::callback->chipType == ChipType::Navi21 ?
                        ddiCapsNavi21 :
                        ddiCapsNavi22,    // Navi 23 uses Navi 22 caps, we also assume the same for Navi 24 here
            .deviceId = X6000P::callback->deviceId,
            .revision = X6000P::callback->revision,
            .extRevision = static_cast<uint32_t>(X6000P::callback->enumRevision) + X6000P::callback->revision,
            .pciRevision = X6000P::callback->pciRevision,
        };
        MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
        DBGLOG("x6000fb", "Applied DDI Caps patches");
    }
}
