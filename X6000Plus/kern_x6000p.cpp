//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#include "kern_x6000p.hpp"
#include "kern_hwlibs.hpp"
#include "kern_model.hpp"
#include "kern_patcherplus.hpp"
#include "kern_patches.hpp"
#include "kern_patterns.hpp"
#include "kern_x6000.hpp"
#include "kern_x6000fb.hpp"
#include <Headers/kern_api.hpp>
#include <Headers/kern_devinfo.hpp>

static const char *pathAGDP = "/System/Library/Extensions/AppleGraphicsControl.kext/Contents/PlugIns/"
                              "AppleGraphicsDevicePolicy.kext/Contents/MacOS/AppleGraphicsDevicePolicy";

static KernelPatcher::KextInfo kextAGDP {"com.apple.driver.AppleGraphicsDevicePolicy", &pathAGDP, 1, {true}, {},
    KernelPatcher::KextInfo::Unloaded};

X6000P *X6000P::callback = nullptr;

static X6000FB x6000fb;
static HWLibs hwlibs;
static X6000 x6000;

void X6000P::init() {
    SYSLOG("x6000p", "Copyright 2022-2023 ChefKiss Inc. If you've paid for this, you've been scammed.");
    callback = this;

    lilu.onKextLoadForce(&kextAGDP);
    x6000fb.init();

    if (!checkKernelArgument("-x6kpfbonly")) {
        hwlibs.init();
        // x6000.init();
    }

    lilu.onPatcherLoadForce(
        [](void *user, KernelPatcher &patcher) { static_cast<X6000P *>(user)->processPatcher(patcher); }, this);
    lilu.onKextLoadForce(
        nullptr, 0,
        [](void *user, KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size) {
            static_cast<X6000P *>(user)->processKext(patcher, id, slide, size);
        },
        this);
}

void X6000P::processPatcher(KernelPatcher &patcher) {
    auto *devInfo = DeviceInfo::create();
    if (devInfo) {
        devInfo->processSwitchOff();
        char name[256] = {0};
        for (size_t i = 0, ii = 0; i < devInfo->videoExternal.size(); i++) {
            auto *device = OSDynamicCast(IOPCIDevice, devInfo->videoExternal[i].video);
            if (!device) { continue; }
            auto devid = WIOKit::readPCIConfigValue(device, WIOKit::kIOPCIConfigDeviceID) & 0xFF00;
            if (WIOKit::readPCIConfigValue(device, WIOKit::kIOPCIConfigVendorID) == WIOKit::VendorID::ATIAMD &&
                (devid == 0x7300 || devid == 0x7400)) {
                this->GPU = device;
                snprintf(name, arrsize(name), "GFX%zu", ii++);
                WIOKit::renameDevice(device, name);
                WIOKit::awaitPublishing(device);
                break;
            }
        }

        static uint8_t builtin[] = {0x00};
        this->GPU->setProperty("built-in", builtin, arrsize(builtin));
        this->deviceId = WIOKit::readPCIConfigValue(this->GPU, WIOKit::kIOPCIConfigDeviceID);
        this->pciRevision = WIOKit::readPCIConfigValue(this->GPU, WIOKit::kIOPCIConfigRevisionID);
        if (!this->GPU->getProperty("model")) {
            auto *model = getBranding(this->deviceId, this->pciRevision);
            this->GPU->setProperty("model", const_cast<char *>(model), static_cast<uint32_t>(strlen(model) + 1));
        }
        DeviceInfo::deleter(devInfo);
    } else {
        SYSLOG("x6000p", "Failed to create DeviceInfo");
    }
}

void X6000P::setRMMIOIfNecessary() {
    if (UNLIKELY(!this->rmmio || !this->rmmio->getLength())) {
        this->rmmio = this->GPU->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress5);
        PANIC_COND(UNLIKELY(!this->rmmio || !this->rmmio->getLength()), "x6000p", "Failed to map RMMIO");
        this->rmmioPtr = reinterpret_cast<uint32_t *>(this->rmmio->getVirtualAddress());

        this->revision = (this->readReg32(0xD31) & 0xF000000) >> 0x18;
        switch (this->deviceId) {
            case 0x73A5:
                this->chipType = ChipType::Navi21;
                this->enumRevision = 0x28;
                break;
            case 0x73DF:
                this->chipType = ChipType::Navi22;
                this->enumRevision = 0x32;
                break;
            case 0x73EF:
                this->chipType = ChipType::Navi23;
                this->enumRevision = 0x3c;
                break;
            default:
                PANIC("x6000p", "Unknown device ID");
        }
    }
}

void X6000P::processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size) {
    if (kextAGDP.loadIndex == id) {
        auto boardId = BaseDeviceInfo::get().boardIdentifier;
        bool agdpboardid = true;
        const char *compatibleBoards[] {
            "Mac-27AD2F918AE68F61",    // MacPro7,1
            "Mac-7BA5B2D9E42DDD94"     // iMacPro1,1
        };
        for (size_t i = 0; i < arrsize(compatibleBoards); i++) {
            if (!strcmp(compatibleBoards[i], boardId)) { agdpboardid = false; }
        }

        static const uint8_t kAGDPBoardIDKeyOriginal[] = "board-id";
        static const uint8_t kAGDPBoardIDKeyPatched[] = "applehax";
        const LookupPatchPlus patches[] = {
            {&kextAGDP, kAGDPBoardIDKeyOriginal, kAGDPBoardIDKeyPatched, 1, agdpboardid},
        };
        PANIC_COND(!LookupPatchPlus::applyAll(patcher, patches, slide, size), "x6000p",
            "Failed to apply AGDP patch: %d", patcher.getError());
    } else if (x6000fb.processKext(patcher, id, slide, size)) {
        DBGLOG("x6000p", "Processed AMDRadeonX6000Framebuffer");
    } else if (hwlibs.processKext(patcher, id, slide, size)) {
        DBGLOG("x6000p", "Processed AMDRadeonX68x0HWLibs");
    } /*else if (x6000.processKext(patcher, id, slide, size)) {
        DBGLOG("x6000p", "Processed AMDRadeonX6000");
    }*/
}
