// Copyright (C) 2023-2026  ChefKiss.
//
// Licensed under the ChefKiss Open Knowledge License version 1.
//
// This work is licensed to you under the terms of the ChefKiss Open Knowledge
// License, either version 1 of the License, or (at your option) any later
// version published by ChefKiss.
//
// This work may not be used to train or develop artificial intelligence,
// and may not be used with artificial intelligence in any other way. See
// the ChefKiss Open Knowledge License for details.
//
// This work is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// ChefKiss Open Knowledge License for more details.
//
// You should have received a copy of the ChefKiss Open Knowledge License
// along with this work.  If not, see https://git.chefkiss.dev/ChefKiss/OpenKnowledge/raw/branch/master/LICENSE-v1.md.

#include "X6000.hpp"
#include "NootRX.hpp"
#include "PatcherPlus.hpp"
#include <Headers/kern_api.hpp>
#include <IOKit/IOService.h>

static const char *pathRadeonX6000 = "/System/Library/Extensions/AMDRadeonX6000.kext/Contents/MacOS/AMDRadeonX6000";

static KernelPatcher::KextInfo kextRadeonX6000 {
    "com.apple.kext.AMDRadeonX6000",
    &pathRadeonX6000,
    1,
    {},
    {},
    KernelPatcher::KextInfo::Unloaded,
};

X6000 *X6000::callback = nullptr;

void X6000::init() {
    SYSLOG("X6000", "Module initialised");

    callback = this;

    lilu.onKextLoadForce(&kextRadeonX6000);
}

bool X6000::processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size) {
    if (kextRadeonX6000.loadIndex == id) {
        NootRXMain::callback->ensureRMMIO();

        RouteRequestPlus request {"__ZN35AMDRadeonX6000_AMDAccelVideoContext9getHWInfoEP13sHardwareInfo", wrapGetHWInfo,
            this->orgGetHWInfo};
        PANIC_COND(!request.route(patcher, id, slide, size), "X6000", "Failed to route getHWInfo");

        if (NootRXMain::callback->attributes.isNavi22() && NootRXMain::callback->attributes.isVenturaAndLater()) {
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
    getMember<UInt16>(hwInfo, 0x4) = NootRXMain::callback->attributes.isNavi21() ? 0x73BF : 0x73FF;
    return ret;
}
