//  Copyright © 2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5. See LICENSE for
//  details.

#include "NootRX.hpp"
#include <Headers/kern_api.hpp>
#include <Headers/kern_version.hpp>
#include <Headers/plugin_start.hpp>
#include <IOKit/IOCatalogue.h>

static NootRXMain x6000p;

static const char *bootargDebug = "-NRXDebug";
static const char *bootargBeta = "-NRXBeta";

PluginConfiguration ADDPR(config) {
    xStringify(PRODUCT_NAME),
    parseModuleVersion(xStringify(MODULE_VERSION)),
    LiluAPI::AllowNormal | LiluAPI::AllowInstallerRecovery | LiluAPI::AllowSafeMode,
    nullptr,
    0,
    &bootargDebug,
    1,
    &bootargBeta,
    1,
    KernelVersion::BigSur,
    KernelVersion::Sonoma,
    []() { x6000p.init(); },
};

OSDefineMetaClassAndStructors(PRODUCT_NAME, IOService);

IOService *PRODUCT_NAME::probe(IOService *provider, SInt32 *score) {
    setProperty("VersionInfo", kextVersion);
    return ADDPR(startSuccess) ? IOService::probe(provider, score) : nullptr;
}

bool PRODUCT_NAME::start(IOService *provider) {
    if (!ADDPR(startSuccess)) { return false; }

    if (!IOService::start(provider)) {
        SYSLOG("Init", "Failed to start the parent");
        return false;
    }

    if (!(lilu.getRunMode() & LiluAPI::RunningInstallerRecovery) && !checkKernelArgument("-CKFBOnly")) {
        auto *prop = OSDynamicCast(OSArray, this->getProperty("Drivers"));
        if (!prop) {
            SYSLOG("Init", "Failed to get Drivers property");
            return false;
        }
        auto *propCopy = prop->copyCollection();
        if (!propCopy) {
            SYSLOG("Init", "Failed to copy Drivers property");
            return false;
        }
        auto *drivers = OSDynamicCast(OSArray, propCopy);
        if (!drivers) {
            SYSLOG("Init", "Failed to cast Drivers property");
            OSSafeReleaseNULL(propCopy);
            return false;
        }
        if (!gIOCatalogue->addDrivers(drivers)) {
            SYSLOG("Init", "Failed to add drivers");
            OSSafeReleaseNULL(drivers);
            return false;
        }
        OSSafeReleaseNULL(drivers);
    }

    return true;
}
