// Copyright © 2023-2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

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
    SYSLOG("NootRX", "Copyright 2023-2024 ChefKiss. If you've paid for this, you've been scammed.");
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
                devid == 0x7300) {
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

        PANIC_COND(!this->GPU, "NootRX", "Failed to find a compatible GPU");

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
                if (this->pciRevision == 0xDF) {
                    PANIC_COND(getKernelVersion() < KernelVersion::Monterey, "NootRX",
                        "Unsupported macOS version; Navi 22 requires macOS Monterey or newer");
                    this->chipType = ChipType::Navi22;
                    this->enumRevision = 0x32;
                    break;
                }
                PANIC_COND(getKernelVersion() < KernelVersion::Monterey, "NootRX",
                    "Unsupported macOS version; Navi 23 requires macOS Monterey or newer");
                this->chipType = ChipType::Navi23;
                this->enumRevision = 0x3C;
                break;
            default:
                PANIC("NootRX", "Unknown device ID");
        }

        DeviceInfo::deleter(devInfo);
    } else {
        SYSLOG("NootRX", "Failed to create DeviceInfo");
    }
    dyldpatches.processPatcher(patcher);

    KernelPatcher::RouteRequest request {"__ZN11IOCatalogue10addDriversEP7OSArrayb", wrapAddDrivers,
        this->orgAddDrivers};
    PANIC_COND(!patcher.routeMultipleLong(KernelPatcher::KernelID, &request, 1), "NootRX",
        "Failed to route addDrivers");
}

static const char *getDriverXMLForBundle(const char *bundleIdentifier, size_t *len) {
    const auto identifierLen = strlen(bundleIdentifier);
    const auto totalLen = identifierLen + 5;
    auto *filename = new char[totalLen];
    memcpy(filename, bundleIdentifier, identifierLen);
    strlcat(filename, ".xml", totalLen);

    const auto &driversXML = getFWByName(filename);
    delete[] filename;

    *len = driversXML.length + 1;
    auto *dataNull = new char[*len];
    memcpy(dataNull, driversXML.data, driversXML.length);
    dataNull[driversXML.length] = 0;

    return dataNull;
}

static const char *DriverBundleIdentifiers[] = {
    "com.apple.kext.AMDRadeonX6000",
    "com.apple.kext.AMDRadeonX6000HWServices",
};

bool NootRXMain::wrapAddDrivers(void *that, OSArray *array, bool doNubMatching) {
    bool matches[arrsize(DriverBundleIdentifiers)];
    bzero(matches, sizeof(matches));

    auto *iterator = OSCollectionIterator::withCollection(array);
    OSObject *object;
    while ((object = iterator->getNextObject())) {
        auto *dict = OSDynamicCast(OSDictionary, object);
        if (dict == nullptr) {
            DBGLOG("NootRX", "Warning: element in addDrivers is not a dictionary.");
            continue;
        }
        auto *bundleIdentifier = OSDynamicCast(OSString, dict->getObject("CFBundleIdentifier"));
        if (bundleIdentifier == nullptr) {
            DBGLOG("NootRX", "Warning: element in addDrivers has no bundle identifier.");
            continue;
        }
        for (size_t i = 0; i < arrsize(DriverBundleIdentifiers); i += 1) {
            if (matches[i]) { continue; }

            auto *matchingIdentifier = DriverBundleIdentifiers[i];
            if (strcmp(bundleIdentifier->getCStringNoCopy(), matchingIdentifier) == 0) {
                DBGLOG("NootRX", "Matched %s.", matchingIdentifier);
                matches[i] = true;
                break;
            }
        }
    }
    OSSafeReleaseNULL(iterator);

    auto res = FunctionCast(wrapAddDrivers, callback->orgAddDrivers)(that, array, doNubMatching);
    for (size_t i = 0; i < arrsize(DriverBundleIdentifiers); i += 1) {
        if (!matches[i]) { continue; }
        auto *identifier = DriverBundleIdentifiers[i];
        DBGLOG("NootRX", "Injecting personalities for %s.", identifier);
        size_t len;
        auto *driverXML = getDriverXMLForBundle(identifier, &len);

        OSString *errStr = nullptr;
        auto *dataUnserialized = OSUnserializeXML(driverXML, len, &errStr);
        delete[] driverXML;

        PANIC_COND(dataUnserialized == nullptr, "NootRX", "Failed to unserialize driver XML for %s: %s", identifier,
            errStr ? errStr->getCStringNoCopy() : "(nil)");

        auto *drivers = OSDynamicCast(OSArray, dataUnserialized);
        PANIC_COND(drivers == nullptr, "NootRX", "Failed to cast %s driver data", identifier);
        if (!FunctionCast(wrapAddDrivers, callback->orgAddDrivers)(that, drivers, doNubMatching)) {
            SYSLOG("NootRX", "Error: Failed to inject personalities for %s.", identifier);
        }
        dataUnserialized->release();
    }
    return res;
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
