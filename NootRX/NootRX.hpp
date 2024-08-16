// Copyright © 2023-2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include "AMDCommon.hpp"
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
    void setRMMIOIfNecessary();
    void processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size);

    UInt32 readReg32(UInt32 reg) {
        if ((reg * 4) < this->rmmio->getLength()) {
            return this->rmmioPtr[reg];
        } else {
            this->rmmioPtr[mmPCIE_INDEX2] = reg;
            return this->rmmioPtr[mmPCIE_DATA2];
        }
    }

    void writeReg32(UInt32 reg, UInt32 val) {
        if ((reg * 4) < this->rmmio->getLength()) {
            this->rmmioPtr[reg] = val;
        } else {
            this->rmmioPtr[mmPCIE_INDEX2] = reg;
            this->rmmioPtr[mmPCIE_DATA2] = val;
        }
    }

    static const char *getGCPrefix() {
        PANIC_COND(callback->chipType == ChipType::Unknown, "NootRX", "Unknown chip type");
        static const char *gcPrefixes[] = {"gc_10_3_", "gc_10_3_2_", "gc_10_3_4_"};
        return gcPrefixes[static_cast<int>(callback->chipType)];
    }

    ChipType chipType {ChipType::Unknown};
    IOMemoryMap *rmmio {nullptr};
    volatile UInt32 *rmmioPtr {nullptr};
    UInt32 deviceId {0};
    UInt16 enumRevision {0};
    UInt16 revision {0};
    UInt32 pciRevision {0};
    IOPCIDevice *GPU {nullptr};
    mach_vm_address_t orgAddDrivers {0};

    static bool wrapAddDrivers(void *that, OSArray *array, bool doNubMatching);
};

//------ Patches ------//

// Neutralise access to AGDP configuration by board identifier.
static const UInt8 kAGDPBoardIDKeyOriginal[] = "board-id";
static const UInt8 kAGDPBoardIDKeyPatched[] = "applehax";
