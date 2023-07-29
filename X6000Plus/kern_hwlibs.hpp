//  Copyright © 2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5. See LICENSE for
//  details.

#ifndef kern_hwlibs_hpp
#define kern_hwlibs_hpp
#include "kern_amd.hpp"
#include "kern_patcherplus.hpp"
#include "kern_x6000p.hpp"
#include <Headers/kern_util.hpp>

class HWLibs {
    public:
    static HWLibs *callback;
    void init();
    bool processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size);

    private:
    static char *wrapGetMatchProperty(void);
};

#endif /* kern_hwlibs_hpp */
