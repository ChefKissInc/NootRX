//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#ifndef kern_hwlibs_hpp
#define kern_hwlibs_hpp
#include "kern_amd.hpp"
#include "kern_x6000p.hpp"
#include "kern_patcherplus.hpp"
#include <Headers/kern_util.hpp>

class HWLibs {
    public:
    static HWLibs *callback;
    void init();
    bool processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);
};

#endif /* kern_hwlibs_hpp */
