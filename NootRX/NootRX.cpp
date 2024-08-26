// Copyright © 2023-2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include "NootRX.hpp"
#include "Firmware.hpp"
#include "Model.hpp"
#include "PatcherPlus.hpp"
#include <Headers/kern_api.hpp>
#include <Headers/kern_devinfo.hpp>
#include <IOKit/IOCatalogue.h>

static const char *pathAGDP = "/System/Library/Extensions/AppleGraphicsControl.kext/Contents/PlugIns/"
                              "AppleGraphicsDevicePolicy.kext/Contents/MacOS/AppleGraphicsDevicePolicy";

static KernelPatcher::KextInfo kextAGDP {
    "com.apple.driver.AppleGraphicsDevicePolicy",
    &pathAGDP,
    1,
    {true},
    {},
    KernelPatcher::KextInfo::Unloaded,
};

NootRXMain *NootRXMain::callback = nullptr;

void NootRXMain::init() {
    SYSLOG("NootRX", "Copyright 2023-2024 ChefKiss. If you've paid for this, you've been scammed.");
    callback = this;

    lilu.onKextLoadForce(&kextAGDP);
    this->dyldpatches.init();
    this->x6000fb.init();
    this->hwlibs.init();
    this->x6000.init();

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
    PANIC_COND(devInfo == nullptr, "NootRX", "DeviceInfo::create failed");
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

    this->deviceID = WIOKit::readPCIConfigValue(this->GPU, WIOKit::kIOPCIConfigDeviceID);
    this->pciRevision = WIOKit::readPCIConfigValue(this->GPU, WIOKit::kIOPCIConfigRevisionID);
    if (!this->GPU->getProperty("model")) {
        auto *model = getBranding(this->deviceID, this->pciRevision);
        auto modelLen = static_cast<UInt32>(strlen(model) + 1);
        if (model) {
            this->GPU->setProperty("model", const_cast<char *>(model), modelLen);
            this->GPU->setProperty("ATY,FamilyName", const_cast<char *>("Radeon RX"), 10);
            this->GPU->setProperty("ATY,DeviceName", const_cast<char *>(model) + 14,
                modelLen - 14);    // 6600 XT...
        }
    }

    switch (this->deviceID) {
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

    this->dyldpatches.processPatcher(patcher);

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
    "com.apple.kext.AMDRadeonX6000Framebuffer",
};

bool NootRXMain::wrapAddDrivers(void *that, OSArray *array, bool doNubMatching) {
    UInt8 matched = 0;

    UInt32 driverCount = array->getCount();
    for (UInt32 driverIndex = 0; driverIndex < driverCount; driverIndex += 1) {
        OSObject *object = array->getObject(driverIndex);
        PANIC_COND(object == nullptr, "NootRX", "Critical error in addDrivers: Index is out of bounds.");
        auto *dict = OSDynamicCast(OSDictionary, object);
        if (dict == nullptr) { continue; }
        auto *bundleIdentifier = OSDynamicCast(OSString, dict->getObject("CFBundleIdentifier"));
        if (bundleIdentifier == nullptr || bundleIdentifier->getLength() == 0) { continue; }
        auto *bundleIdentifierCStr = bundleIdentifier->getCStringNoCopy();
        if (bundleIdentifierCStr == nullptr) { continue; }

        for (size_t identifierIndex = 0; identifierIndex < arrsize(DriverBundleIdentifiers); identifierIndex += 1) {
            if ((matched & (1U << identifierIndex)) != 0) { continue; }

            if (strcmp(bundleIdentifierCStr, DriverBundleIdentifiers[identifierIndex]) == 0) {
                matched |= (1U << identifierIndex);

                DBGLOG("NootRX", "Matched %s, injecting.", bundleIdentifierCStr);

                size_t len;
                auto *driverXML = getDriverXMLForBundle(bundleIdentifierCStr, &len);

                OSString *errStr = nullptr;
                auto *dataUnserialized = OSUnserializeXML(driverXML, len, &errStr);
                delete[] driverXML;

                PANIC_COND(dataUnserialized == nullptr, "NootRX", "Failed to unserialize driver XML for %s: %s",
                    bundleIdentifierCStr, errStr ? errStr->getCStringNoCopy() : "(nil)");

                auto *drivers = OSDynamicCast(OSArray, dataUnserialized);
                PANIC_COND(drivers == nullptr, "NootRX", "Failed to cast %s driver data", bundleIdentifierCStr);
                UInt32 injectedDriverCount = drivers->getCount();

                array->ensureCapacity(driverCount + injectedDriverCount);

                for (UInt32 injectedDriverIndex = 0; injectedDriverIndex < injectedDriverCount;
                     injectedDriverIndex += 1) {
                    array->setObject(driverIndex, drivers->getObject(injectedDriverIndex));
                    driverIndex += 1;
                    driverCount += 1;
                }

                dataUnserialized->release();
                break;
            }
        }
    }

    return FunctionCast(wrapAddDrivers, callback->orgAddDrivers)(that, array, doNubMatching);
}

void NootRXMain::ensureRMMIO() {
    if (this->rmmio != nullptr) { return; }

    this->rmmio = this->GPU->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress5, kIOMapInhibitCache | kIOMapAnywhere);
    PANIC_COND(this->rmmio == nullptr || this->rmmio->getLength() == 0, "NootRX", "Failed to map RMMIO");
    this->rmmioPtr = reinterpret_cast<UInt32 *>(this->rmmio->getVirtualAddress());
    this->devRevision = (this->readReg32(0xD31) & 0xF000000) >> 0x18;
}

void NootRXMain::processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size) {
    if (kextAGDP.loadIndex == id) {
        // Don't apply AGDP patch on MacPro7,1
        if (!strncmp("Mac-27AD2F918AE68F61", BaseDeviceInfo::get().boardIdentifier, 21)) { return; }

        const LookupPatchPlus patch {&kextAGDP, kAGDPBoardIDKeyOriginal, kAGDPBoardIDKeyPatched, 1};
        PANIC_COND(!patch.apply(patcher, slide, size), "NootRX", "Failed to apply AGDP patch");

        DBGLOG("NootRX", "Processed Apple Graphics Device Policy");
    } else if (this->x6000fb.processKext(patcher, id, slide, size)) {
        DBGLOG("NootRX", "Processed Framebuffer");
    } else if (this->hwlibs.processKext(patcher, id, slide, size)) {
        DBGLOG("NootRX", "Processed HW Library");
    } else if (this->x6000.processKext(patcher, id, slide, size)) {
        DBGLOG("NootRX", "Processed Accelerator");
    }
}

UInt32 NootRXMain::readReg32(UInt32 reg) {
    if ((reg * 4) < this->rmmio->getLength()) {
        return this->rmmioPtr[reg];
    } else {
        this->rmmioPtr[mmPCIE_INDEX2] = reg;
        return this->rmmioPtr[mmPCIE_DATA2];
    }
}

void NootRXMain::writeReg32(UInt32 reg, UInt32 val) {
    if ((reg * 4) < this->rmmio->getLength()) {
        this->rmmioPtr[reg] = val;
    } else {
        this->rmmioPtr[mmPCIE_INDEX2] = reg;
        this->rmmioPtr[mmPCIE_DATA2] = val;
    }
}

const char *NootRXMain::getGCPrefix() {
    PANIC_COND(callback->chipType == ChipType::Unknown, "NootRX", "Unknown chip type");
    static const char *gcPrefixes[] = {"gc_10_3_", "gc_10_3_2_", "gc_10_3_4_"};
    return gcPrefixes[static_cast<int>(callback->chipType)];
}
