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
    {},
    {},
    KernelPatcher::KextInfo::Unloaded,
};

NootRXMain *NootRXMain::callback = nullptr;

void NootRXMain::init() {
    SYSLOG("NootRX", "Copyright 2023-2024 ChefKiss. If you've paid for this, you've been scammed.");

    if (!checkKernelArgument("-NRXNoVCN")) { this->attributes.setVCNEnabled(); }

    switch (getKernelVersion()) {
        case KernelVersion::BigSur:
            this->attributes.setBigSur();
            break;
        case KernelVersion::Monterey:
            break;
        case KernelVersion::Ventura:
            this->attributes.setVenturaAndLater();
            break;
        case KernelVersion::Sonoma:
            this->attributes.setVenturaAndLater();
            if (getKernelMinorVersion() >= 4) { this->attributes.setSonoma1404AndLater(); }
            break;
        case KernelVersion::Sequoia:
            this->attributes.setVenturaAndLater();
            this->attributes.setSonoma1404AndLater();
            break;
        default:
            PANIC("NootRX", "Unsupported kernel version %d", getKernelVersion());
    }

    DBGLOG("NootRX", "isVCNEnabled: %s", this->attributes.isVCNEnabled() ? "yes" : "no");
    DBGLOG("NootRX", "isBigSur: %s", this->attributes.isBigSur() ? "yes" : "no");
    DBGLOG("NootRX", "isVenturaAndLater: %s", this->attributes.isVenturaAndLater() ? "yes" : "no");
    DBGLOG("NootRX", "isSonoma1404AndLater: %s", this->attributes.isSonoma1404AndLater() ? "yes" : "no");

    SYSLOG("NootRX", "Module initialised");

    callback = this;

    lilu.onKextLoadForce(&kextAGDP);

    if (NootRXMain::callback->attributes.isVCNEnabled()) { this->dyldpatches.init(); }
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

    char slotName[256];
    bzero(slotName, sizeof(slotName));
    for (size_t i = 0, ii = 0; i < devInfo->videoExternal.size(); i++) {
        auto *device = OSDynamicCast(IOPCIDevice, devInfo->videoExternal[i].video);
        if (device == nullptr) { continue; }
        if (WIOKit::readPCIConfigValue(device, WIOKit::kIOPCIConfigVendorID) == WIOKit::VendorID::ATIAMD &&
            (WIOKit::readPCIConfigValue(device, WIOKit::kIOPCIConfigDeviceID) & 0xFF00) == 0x7300) {
            this->dGPU = device;
            snprintf(slotName, arrsize(slotName), "GFX%zu", ii++);
            WIOKit::renameDevice(device, slotName);
            WIOKit::awaitPublishing(device);
            if (device->getProperty("AAPL,slot-name") == nullptr) {
                snprintf(slotName, sizeof(slotName), "Slot-%zu", ii++);
                device->setProperty("AAPL,slot-name", slotName,
                    static_cast<UInt32>(strnlen(slotName, sizeof(slotName)) + 1));
            }
            break;
        }
    }

    PANIC_COND(this->dGPU == nullptr, "NootRX", "Failed to find a compatible GPU");

    UInt8 builtIn[] = {0x00};
    this->dGPU->setProperty("built-in", builtIn, arrsize(builtIn));

    this->deviceId = WIOKit::readPCIConfigValue(this->dGPU, WIOKit::kIOPCIConfigDeviceID);
    this->pciRevision = WIOKit::readPCIConfigValue(this->dGPU, WIOKit::kIOPCIConfigRevisionID);

    SYSLOG_COND(this->dGPU->getProperty("model") != nullptr, "NootRX",
        "WARNING!!! Attempted to manually override the model, this is no longer supported!!");
    auto *model = getBranding(this->deviceId, this->pciRevision);
    auto modelLen = static_cast<UInt32>(strlen(model) + 1);
    this->dGPU->setProperty("model", const_cast<char *>(model), modelLen);
    if (model[11] == 'P' && model[12] == 'r' && model[13] == 'o' && model[14] == ' ') {
        this->dGPU->setProperty("ATY,FamilyName", const_cast<char *>("Radeon Pro"), 11);
        // Without AMD Radeon Pro prefix
        this->dGPU->setProperty("ATY,DeviceName", const_cast<char *>(model) + 15, modelLen - 15);
    } else {
        this->dGPU->setProperty("ATY,FamilyName", const_cast<char *>("Radeon RX"), 10);
        // Without AMD Radeon RX prefix
        this->dGPU->setProperty("ATY,DeviceName", const_cast<char *>(model) + 14, modelLen - 14);
    }

    switch (this->deviceId) {
        case 0x73A2:
        case 0x73A3:
        case 0x73A5:
        case 0x73AB:
        case 0x73AF:
        case 0x73BF:
            this->attributes.setNavi21();
            this->enumRevision = 0x28;
            break;
        case 0x73DF:
            PANIC_COND(this->attributes.isBigSur(), "NootRX", "Your GPU requires macOS 12 and newer");
            this->attributes.setNavi22();
            this->enumRevision = 0x32;
            break;
        case 0x73E0:
        case 0x73E1:
        case 0x73E3:
        case 0x73EF:
        case 0x73FF:
            PANIC_COND(this->attributes.isBigSur(), "NootRX", "Your GPU requires macOS 12 and newer");
            if (this->pciRevision == 0xDF) {
                this->attributes.setNavi22();
                this->enumRevision = 0x32;
            } else {
                this->attributes.setNavi23();
                this->enumRevision = 0x3C;
            }
            break;
        default:
            PANIC("NootRX", "Unknown device ID: 0x%04X", this->deviceId);
    }

    DBGLOG("NootRX", "deviceId: 0x%04X", this->deviceId);
    DBGLOG("NootRX", "pciRevision: 0x%X", this->pciRevision);
    DBGLOG("NootRX", "enumRevision: 0x%X", this->enumRevision);
    DBGLOG("NootRX", "isNavi21: %s", this->attributes.isNavi21() ? "yes" : "no");
    DBGLOG("NootRX", "isNavi22: %s", this->attributes.isNavi22() ? "yes" : "no");
    DBGLOG("NootRX", "isNavi23: %s", this->attributes.isNavi23() ? "yes" : "no");

    DeviceInfo::deleter(devInfo);

    if (NootRXMain::callback->attributes.isVCNEnabled()) { this->dyldpatches.processPatcher(patcher); }

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
static const char *DriverBundleXMLsBigSur[] = {
    nullptr,
    nullptr,
    "com.apple.kext.AMDRadeonX6000Framebuffer_BigSur",
};
static_assert(arrsize(DriverBundleIdentifiers) == arrsize(DriverBundleXMLsBigSur));

static UInt8 matchedDrivers = 0;

bool NootRXMain::wrapAddDrivers(void *that, OSArray *array, bool doNubMatching) {
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
            if ((matchedDrivers & (1U << identifierIndex)) != 0) { continue; }

            if (strcmp(bundleIdentifierCStr, DriverBundleIdentifiers[identifierIndex]) == 0) {
                matchedDrivers |= (1U << identifierIndex);

                DBGLOG("NootRX", "Matched %s, injecting.", bundleIdentifierCStr);

                size_t len;
                auto *driverBundle =
                    callback->attributes.isBigSur() ? DriverBundleXMLsBigSur[identifierIndex] : bundleIdentifierCStr;
                if (driverBundle == nullptr) { driverBundle = bundleIdentifierCStr; }
                auto *driverXML = getDriverXMLForBundle(driverBundle, &len);

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

    this->dGPU->setMemoryEnable(true);
    this->dGPU->setBusMasterEnable(true);
    this->rmmio =
        this->dGPU->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress5, kIOMapInhibitCache | kIOMapAnywhere);
    PANIC_COND(this->rmmio == nullptr || this->rmmio->getLength() == 0, "NootRX", "Failed to map RMMIO");
    this->rmmioPtr = reinterpret_cast<UInt32 *>(this->rmmio->getVirtualAddress());
    this->devRevision = (this->readReg32(0xD31) & 0xF000000) >> 0x18;
}

void NootRXMain::processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size) {
    if (kextAGDP.loadIndex == id) {
        // Don't apply AGDP patch on MacPro7,1
        if (strncmp("Mac-27AD2F918AE68F61", BaseDeviceInfo::get().boardIdentifier, 21) == 0) { return; }

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
    if ((reg * sizeof(UInt32)) < this->rmmio->getLength()) {
        return this->rmmioPtr[reg];
    } else {
        this->rmmioPtr[mmPCIE_INDEX2] = reg;
        return this->rmmioPtr[mmPCIE_DATA2];
    }
}

void NootRXMain::writeReg32(UInt32 reg, UInt32 val) {
    if ((reg * sizeof(UInt32)) < this->rmmio->getLength()) {
        this->rmmioPtr[reg] = val;
    } else {
        this->rmmioPtr[mmPCIE_INDEX2] = reg;
        this->rmmioPtr[mmPCIE_DATA2] = val;
    }
}

const char *NootRXMain::getGCPrefix() {
    if (this->attributes.isNavi21()) {
        return "gc_10_3_";
    } else if (this->attributes.isNavi22()) {
        return "gc_10_3_2_";
    } else if (this->attributes.isNavi23()) {
        return "gc_10_3_4_";
    } else {
        UNREACHABLE();
    }
}
