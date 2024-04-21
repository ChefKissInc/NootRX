//! Copyright © 2023-2024 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5.
//! See LICENSE for details.

#pragma once
#include "AMDCommon.hpp"
#include "NootRX.hpp"
#include "PatcherPlus.hpp"
#include <Headers/kern_patcher.hpp>
#include <Headers/kern_util.hpp>

class X6000FB {
    public:
    static X6000FB *callback;
    void init();
    bool processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size);

    private:
    mach_vm_address_t orgInitWithPciInfo {0};

    static UInt16 wrapGetEnumeratedRevision();
    static bool wrapInitWithPciInfo(void *that, void *param1);
    static void wrapDoGPUPanic();
    static void wrapDmLoggerWrite([[maybe_unused]] void *dalLogger, UInt32 logType, char *fmt, ...);
};

//------ Patterns ------//

static const UInt8 kCailAsicCapsTablePattern[] = {0x6E, 0x00, 0x00, 0x00, 0x98, 0x67, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};

static const UInt8 kDmLoggerWritePattern[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41, 0x54,
    0x53, 0x48, 0x81, 0xEC, 0x88, 0x04, 0x00, 0x00};
