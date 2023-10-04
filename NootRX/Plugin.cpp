//  Copyright © 2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5. See LICENSE for
//  details.

#include "NootRX.hpp"
#include <Headers/kern_api.hpp>
#include <Headers/kern_version.hpp>
#include <Headers/plugin_start.hpp>
#include <IOKit/IOCatalogue.h>

static NootRXMain nrx;

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
    []() { nrx.init(); },
};
