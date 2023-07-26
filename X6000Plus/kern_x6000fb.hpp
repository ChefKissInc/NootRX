//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#ifndef kern_x6000fb_hpp
#define kern_x6000fb_hpp
#include "kern_amd.hpp"
#include "kern_patcherplus.hpp"
#include "kern_x6000p.hpp"
#include <Headers/kern_patcher.hpp>
#include <Headers/kern_util.hpp>

class X6000FB {
    public:
    static X6000FB *callback;
    void init();
    bool processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size);

    private:
    mach_vm_address_t orgInitWithPciInfo {0};

    static bool wrapInitWithPciInfo(void *that, void *param1);
    static void wrapDoGPUPanic();
    static void wrapDmLoggerWrite([[maybe_unused]] void *dalLogger, uint32_t logType, char *fmt, ...);
};

#endif /* kern_x6000fb_hpp */
