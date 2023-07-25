//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#ifndef kern_x6000_hpp
#define kern_x6000_hpp
#include "kern_amd.hpp"
#include "kern_patcherplus.hpp"
#include "kern_x6000p.hpp"
#include <Headers/kern_util.hpp>
#include <IOKit/IOService.h>

class X6000 {
    public:
    static X6000 *callback;
    void init();
    bool processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size);

    private:
    mach_vm_address_t orgGetHWInfo {0};

    static void wrapGetHWInfo(IOService *accelVideoCtx, void *hwInfo);
};

#endif /* kern_x6000_hpp */
