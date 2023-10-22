//! Copyright © 2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5.
//! See LICENSE for details.

#include "NootRX.hpp"
#include "DYLDPatches.hpp"
#include "Firmware.hpp"
#include "HWLibs.hpp"
#include "Model.hpp"
#include "PatcherPlus.hpp"
#include "X6000.hpp"
#include "X6000FB.hpp"
#include <Headers/kern_api.hpp>
#include <Headers/kern_devinfo.hpp>
#include <IOKit/IOCatalogue.h>

static const char *pathAGDP = "/System/Library/Extensions/AppleGraphicsControl.kext/Contents/PlugIns/"
                              "AppleGraphicsDevicePolicy.kext/Contents/MacOS/AppleGraphicsDevicePolicy";

static KernelPatcher::KextInfo kextAGDP {"com.apple.driver.AppleGraphicsDevicePolicy", &pathAGDP, 1, {true}, {},
    KernelPatcher::KextInfo::Unloaded};

NootRXMain *NootRXMain::callback = nullptr;

static X6000FB x6000fb;
static HWLibs hwlibs;
static X6000 x6000;
static DYLDPatches dyldpatches;

void NootRXMain::init() {
    SYSLOG("NootRX", "Copyright 2023 ChefKiss Inc. If you've paid for this, you've been scammed.");
    callback = this;

    lilu.onKextLoadForce(&kextAGDP);
    dyldpatches.init();
    x6000fb.init();
    hwlibs.init();
    x6000.init();

    lilu.onPatcherLoadForce(
        [](void *user, KernelPatcher &patcher) { static_cast<NootRXMain *>(user)->processPatcher(patcher); }, this);
    lilu.onKextLoadForce(
        nullptr, 0,
        [](void *user, KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size) {
            static_cast<NootRXMain *>(user)->processKext(patcher, id, slide, size);
        },
        this);
}

void NootRXMain::processPatcher(KernelPatcher &patcher) {
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
                if (!device->getProperty("AAPL,slot-name")) {
                    snprintf(name, sizeof(name), "Slot-%zu", ii++);
                    device->setProperty("AAPL,slot-name", name, sizeof("Slot-1"));
                }
                break;
            }
        }

        PANIC_COND(!this->GPU, "NootRX", "Failed to find GPU");

        if (!GPU->getProperty("built-in")) {
            static UInt8 builtin[] = {0x00};
            this->GPU->setProperty("built-in", builtin, arrsize(builtin));
        }

        this->deviceId = WIOKit::readPCIConfigValue(this->GPU, WIOKit::kIOPCIConfigDeviceID);
        this->pciRevision = WIOKit::readPCIConfigValue(this->GPU, WIOKit::kIOPCIConfigRevisionID);
        if (!this->GPU->getProperty("model")) {
            auto *model = getBranding(this->deviceId, this->pciRevision);
            auto modelLen = static_cast<UInt32>(strlen(model) + 1);
            if (model) {
                this->GPU->setProperty("model", const_cast<char *>(model), modelLen);
                this->GPU->setProperty("ATY,FamilyName", const_cast<char *>("Radeon RX"), 10);
                this->GPU->setProperty("ATY,DeviceName", const_cast<char *>(model) + 14,
                    modelLen - 14);    // 6600 XT...
            }
        }

        switch (this->deviceId) {
            case 0x73A2 ... 0x73A3:
                [[fallthrough]];
            case 0x73A5:
                [[fallthrough]];
            case 0x73AB:
                [[fallthrough]];
            case 0x73AF:
                [[fallthrough]];
            case 0x73BF:
                this->chipType = ChipType::Navi21;
                this->enumRevision = 0x28;
                break;
            case 0x73DF:
                PANIC_COND(getKernelVersion() < KernelVersion::Monterey, "NootRX",
                    "Unsupported macOS version; Navi 22 requires macOS Monterey or newer");
                this->chipType = ChipType::Navi22;
                this->enumRevision = 0x32;
                break;
            case 0x73E0 ... 0x73E1:
                [[fallthrough]];
            case 0x73E3:
                [[fallthrough]];
            case 0x73EF:
                [[fallthrough]];
            case 0x73FF:
                PANIC_COND(getKernelVersion() < KernelVersion::Monterey, "NootRX",
                    "Unsupported macOS version; Navi 23 requires macOS Monterey or newer");
                this->chipType = ChipType::Navi23;
                this->enumRevision = 0x3C;
                break;
            default:
                PANIC("NootRX", "Unknown device ID");
        }

        // No named framebuffer for Navi 22 for now
        if (this->chipType == ChipType::Navi21) {
            if (this->pciRevision == 0xC1 || this->pciRevision == 0xC3) {
                // RX 6800 (XT)
                this->GPU->setProperty("@0,name", const_cast<char *>("ATY,Belknap"), 12);
            } else {
                // RX 6900 XT / RX 6950 XT
                this->GPU->setProperty("@0,name", const_cast<char *>("ATY,Carswell"), 13);
            }
        } else if (this->chipType == ChipType::Navi23) {
            this->GPU->setProperty("@0,name", const_cast<char *>("ATY,Henbury"), 12);
        }

        DeviceInfo::deleter(devInfo);
    } else {
        SYSLOG("NootRX", "Failed to create DeviceInfo");
    }
    dyldpatches.processPatcher(patcher);

    if ((lilu.getRunMode() & LiluAPI::RunningInstallerRecovery) || checkKernelArgument("-NRXFBOnly")) { return; }

    const auto driversXML = getFWByName("Drivers.xml");
    auto *dataNull = new char[driversXML.size + 1];
    memcpy(dataNull, driversXML.data, driversXML.size);
    dataNull[driversXML.size] = 0;
    OSString *errStr = nullptr;
    auto *dataUnserialized = OSUnserializeXML(dataNull, driversXML.size + 1, &errStr);
    delete[] dataNull;
    PANIC_COND(!dataUnserialized, "NootRX", "Failed to unserialize Drivers.xml: %s",
        errStr ? errStr->getCStringNoCopy() : "Unspecified");
    auto *drivers = OSDynamicCast(OSArray, dataUnserialized);
    PANIC_COND(!drivers, "NootRX", "Failed to cast Drivers.xml data");
    PANIC_COND(!gIOCatalogue->addDrivers(drivers), "NootRX", "Failed to add drivers");
    OSSafeReleaseNULL(dataUnserialized);
    IOFree(driversXML.data, driversXML.size);
}

void NootRXMain::setRMMIOIfNecessary() {
    if (UNLIKELY(!this->rmmio || !this->rmmio->getLength())) {
        OSSafeReleaseNULL(this->rmmio);
        this->rmmio = this->GPU->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress5);
        PANIC_COND(UNLIKELY(!this->rmmio || !this->rmmio->getLength()), "NootRX", "Failed to map RMMIO");
        this->rmmioPtr = reinterpret_cast<UInt32 *>(this->rmmio->getVirtualAddress());
        this->revision = (this->readReg32(0xD31) & 0xF000000) >> 0x18;
    }
}

void NootRXMain::processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size) {
    if (kextAGDP.loadIndex == id) {
        // Don't apply AGDP patch on MacPro7,1
        auto boardId = BaseDeviceInfo::get().boardIdentifier;
        if (!strncmp("Mac-27AD2F918AE68F61", boardId, 21)) { return; }

        const LookupPatchPlus patch {&kextAGDP, kAGDPBoardIDKeyOriginal, kAGDPBoardIDKeyPatched, 1};
        PANIC_COND(!patch.apply(patcher, slide, size), "NootRX", "Failed to apply AGDP patch");
    } else if (x6000fb.processKext(patcher, id, slide, size)) {
        DBGLOG("NootRX", "Processed AMDRadeonX6000Framebuffer");
    } else if (hwlibs.processKext(patcher, id, slide, size)) {
        DBGLOG("NootRX", "Processed AMDRadeonX68x0HWLibs");
    } else if (x6000.processKext(patcher, id, slide, size)) {
        DBGLOG("NootRX", "Processed AMDRadeonX6000");
    }
}
