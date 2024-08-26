// Copyright © 2023-2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include "X6000.hpp"
#include "NootRX.hpp"
#include "PatcherPlus.hpp"
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
        NootRXMain::callback->ensureRMMIO();

        if (!checkKernelArgument("-NRXNoVCN")) {
            RouteRequestPlus request {"__ZN35AMDRadeonX6000_AMDAccelVideoContext9getHWInfoEP13sHardwareInfo",
                wrapGetHWInfo, this->orgGetHWInfo};
            PANIC_COND(!request.route(patcher, id, slide, size), "X6000", "Failed to route getHWInfo");
        }

        if (NootRXMain::callback->chipType == ChipType::Navi22 && getKernelVersion() >= KernelVersion::Ventura) {
            const LookupPatchPlus patch = {&kextRadeonX6000, kHwlConvertChipFamilyOriginal,
                kHwlConvertChipFamilyOriginalMask, kHwlConvertChipFamilyPatched, 1};
            PANIC_COND(!patch.apply(patcher, slide, size), "X6000",
                "Failed to apply Navi 22 HwlConvertChipFamily patch");
        }

        return true;
    }

    return false;
}

IOReturn X6000::wrapGetHWInfo(IOService *accelVideoCtx, void *hwInfo) {
    auto ret = FunctionCast(wrapGetHWInfo, callback->orgGetHWInfo)(accelVideoCtx, hwInfo);
    getMember<UInt16>(hwInfo, 0x4) = NootRXMain::callback->chipType == ChipType::Navi21 ? 0x73BF : 0x73FF;
    return ret;
}
