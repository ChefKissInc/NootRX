//  Copyright © 2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5. See LICENSE for
//  details.

#include "kern_x6000.hpp"
#include "kern_hwlibs.hpp"
#include "kern_patterns.hpp"
#include "kern_x6000p.hpp"
#include <Headers/kern_api.hpp>
#include <IOKit/IOService.h>

static const char *pathRadeonX6000 = "/System/Library/Extensions/AMDRadeonX6000.kext/Contents/MacOS/AMDRadeonX6000";

static KernelPatcher::KextInfo kextRadeonX6000 {"com.apple.kext.AMDRadeonX6000", &pathRadeonX6000, 1, {}, {},
    KernelPatcher::KextInfo::Unloaded};

X6000 *X6000::callback = nullptr;

void X6000::init() {
    callback = this;
    lilu.onKextLoadForce(&kextRadeonX6000);
}

bool X6000::processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size) {
    if (kextRadeonX6000.loadIndex == id) {
        X6000P::callback->setRMMIOIfNecessary();

        RouteRequestPlus requests[] = {
            {"__ZN35AMDRadeonX6000_AMDAccelVideoContext9getHWInfoEP13sHardwareInfo", wrapGetHWInfo, this->orgGetHWInfo,
                !checkKernelArgument("-x6kpnovcn")},
        };
        PANIC_COND(!RouteRequestPlus::routeAll(patcher, id, requests, slide, size), "x6000", "Failed to route symbols");

        return true;
    }

    return false;
}

IOReturn X6000::wrapGetHWInfo(IOService *accelVideoCtx, void *hwInfo) {
    auto ret = FunctionCast(wrapGetHWInfo, callback->orgGetHWInfo)(accelVideoCtx, hwInfo);
    getMember<uint16_t>(hwInfo, 0x4) = X6000P::callback->chipType < ChipType::Navi23 ? 0x73BF : 0x73FF;
    return ret;
}
