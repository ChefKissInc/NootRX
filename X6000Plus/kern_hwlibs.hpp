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
    mach_vm_address_t orgPspCosLog {0};

    static const char *wrapGetMatchProperty(void);
    static void wrapIpAssertion(void *data, uint32_t cond, char *func, char *file, uint32_t line, char *msg);
    static void wrapPspCosLog(void *pspData, uint32_t param2, uint64_t param3, uint32_t param4, char *param5);
};

#endif /* kern_hwlibs_hpp */
