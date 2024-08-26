// Copyright © 2023-2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include "DYLDPatches.hpp"
#include "HWLibs.hpp"
#include "X6000.hpp"
#include "X6000FB.hpp"
#include <Headers/kern_patcher.hpp>
#include <IOKit/acpi/IOACPIPlatformExpert.h>
#include <IOKit/graphics/IOFramebuffer.h>
#include <IOKit/pci/IOPCIDevice.h>

enum struct ChipType : UInt32 {
    Navi21 = 0,
    Navi22,
    Navi23,
    Unknown,
};

class NootRXMain {
    friend class HWLibs;
    friend class X6000;
    friend class X6000FB;

    static NootRXMain *callback;

    public:
    void init();
    void processPatcher(KernelPatcher &patcher);

    private:
    void ensureRMMIO();
    void processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size);

    UInt32 readReg32(UInt32 reg);
    void writeReg32(UInt32 reg, UInt32 val);
    const char *getGCPrefix();

    ChipType chipType {ChipType::Unknown};
    IOMemoryMap *rmmio {nullptr};
    volatile UInt32 *rmmioPtr {nullptr};
    UInt32 deviceID {0};
    UInt16 enumRevision {0};
    UInt16 devRevision {0};
    UInt32 pciRevision {0};
    IOPCIDevice *GPU {nullptr};
    mach_vm_address_t orgAddDrivers {0};

    X6000FB x6000fb {};
    HWLibs hwlibs {};
    X6000 x6000 {};
    DYLDPatches dyldpatches {};

    static bool wrapAddDrivers(void *that, OSArray *array, bool doNubMatching);
};

//------ Patches ------//

// Neutralise access to AGDP configuration by board identifier.
static const UInt8 kAGDPBoardIDKeyOriginal[] = "board-id";
static const UInt8 kAGDPBoardIDKeyPatched[] = "applehax";
