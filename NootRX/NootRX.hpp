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

class NootRXAttributes {
    UInt8 value {0};

    static constexpr UInt8 VCNEnabled = (1U << 0);
    static constexpr UInt8 BigSur = (1U << 1);
    static constexpr UInt8 VenturaAndLater = (1U << 2);
    static constexpr UInt8 Sonoma1404AndLater = (1U << 3);
    static constexpr UInt8 Navi21 = (1U << 4);
    static constexpr UInt8 Navi22 = (1U << 5);
    static constexpr UInt8 Navi23 = (1U << 6);

    public:
    inline bool isVCNEnabled() { return (this->value & VCNEnabled) != 0; }
    inline bool isBigSur() { return (this->value & BigSur) != 0; }
    inline bool isVenturaAndLater() { return (this->value & VenturaAndLater) != 0; }
    inline bool isSonoma1404AndLater() { return (this->value & Sonoma1404AndLater) != 0; }
    inline bool isNavi21() { return (this->value & Navi21) != 0; }
    inline bool isNavi22() { return (this->value & Navi22) != 0; }
    inline bool isNavi23() { return (this->value & Navi23) != 0; }

    inline void setVCNEnabled() { this->value |= VCNEnabled; }
    inline void setBigSur() { this->value |= BigSur; }
    inline void setVenturaAndLater() { this->value |= VenturaAndLater; }
    inline void setSonoma1404AndLater() { this->value |= Sonoma1404AndLater; }
    inline void setNavi21() { this->value |= Navi21; }
    inline void setNavi22() { this->value |= Navi22; }
    inline void setNavi23() { this->value |= Navi23; }
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

    NootRXAttributes attributes {};
    IOMemoryMap *rmmio {nullptr};
    volatile UInt32 *rmmioPtr {nullptr};
    UInt32 deviceId {0};
    UInt16 enumRevision {0};
    UInt16 devRevision {0};
    UInt32 pciRevision {0};
    IOPCIDevice *dGPU {nullptr};
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
