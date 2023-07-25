//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#pragma once
#include <Headers/kern_patcher.hpp>
#include <IOKit/acpi/IOACPIPlatformExpert.h>
#include <IOKit/graphics/IOFramebuffer.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <kern_amd.hpp>

class EXPORT PRODUCT_NAME : public IOService {
    OSDeclareDefaultStructors(PRODUCT_NAME);

    public:
    IOService *probe(IOService *provider, SInt32 *score) override;
    bool start(IOService *provider) override;
};

enum struct ChipType : uint32_t {
    Navi21 = 0,
    Navi22,
    Navi23,
    Navi24,
    Unknown,
};

// Hack
class AppleACPIPlatformExpert : IOACPIPlatformExpert {
    friend class X6000P;
};

class X6000P {
    public:
    static X6000P *callback;

    void init();
    void processPatcher(KernelPatcher &patcher);
    void setRMMIOIfNecessary();
    void processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size);

    uint32_t readReg32(uint32_t reg) {
        if (reg * 4 < this->rmmio->getLength()) {
            return this->rmmioPtr[reg];
        } else {
            this->rmmioPtr[mmPCIE_INDEX2] = reg;
            return this->rmmioPtr[mmPCIE_DATA2];
        }
    }

    void writeReg32(uint32_t reg, uint32_t val) {
        if ((reg * 4) < this->rmmio->getLength()) {
            this->rmmioPtr[reg] = val;
        } else {
            this->rmmioPtr[mmPCIE_INDEX2] = reg;
            this->rmmioPtr[mmPCIE_DATA2] = val;
        }
    }

    ChipType chipType {ChipType::Unknown};
    IOMemoryMap *rmmio {nullptr};
    volatile uint32_t *rmmioPtr {nullptr};
    uint32_t deviceId {0};
    uint16_t enumRevision {0};
    uint16_t revision {0};
    uint32_t pciRevision {0};
    IOPCIDevice *GPU {nullptr};
    OSMetaClass *metaClassMap[4][2] = {{nullptr}};
    mach_vm_address_t orgSafeMetaCast {0};

    static OSMetaClassBase *wrapSafeMetaCast(const OSMetaClassBase *anObject, const OSMetaClass *toMeta);
};
