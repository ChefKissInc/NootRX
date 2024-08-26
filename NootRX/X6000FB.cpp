// Copyright © 2023-2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include "X6000FB.hpp"
#include "NootRX.hpp"
#include "PatcherPlus.hpp"
#include <Headers/kern_api.hpp>

static const char *pathRadeonX6000Framebuffer =
    "/System/Library/Extensions/AMDRadeonX6000Framebuffer.kext/Contents/MacOS/AMDRadeonX6000Framebuffer";

static KernelPatcher::KextInfo kextRadeonX6000Framebuffer {"com.apple.kext.AMDRadeonX6000Framebuffer",
    &pathRadeonX6000Framebuffer, 1, {}, {}, KernelPatcher::KextInfo::Unloaded};

X6000FB *X6000FB::callback = nullptr;

void X6000FB::init() {
    callback = this;
    lilu.onKextLoadForce(&kextRadeonX6000Framebuffer);
}

bool X6000FB::processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size) {
    if (kextRadeonX6000Framebuffer.loadIndex == id) {
        NootRXMain::callback->ensureRMMIO();

        CAILAsicCapsEntry *orgAsicCapsTable = nullptr;

        SolveRequestPlus solveRequests[] = {
            {"__ZL20CAIL_ASIC_CAPS_TABLE", orgAsicCapsTable, kCailAsicCapsTablePattern},
        };
        PANIC_COND(!SolveRequestPlus::solveAll(patcher, id, solveRequests, slide, size), "X6000FB",
            "Failed to resolve CailAsicCapsTable");

        if (NootRXMain::callback->chipType == ChipType::Navi22) {
            RouteRequestPlus request {"__ZNK32AMDRadeonX6000_AmdAsicInfoNavi2327getEnumeratedRevisionNumberEv",
                wrapGetEnumeratedRevision};
            PANIC_COND(!request.route(patcher, id, slide, size), "X6000FB",
                "Failed to route getEnumeratedRevisionNumber");
        }

        if (ADDPR(debugEnabled)) {
            RouteRequestPlus requests[] = {
                {"__ZN24AMDRadeonX6000_AmdLogger15initWithPciInfoEP11IOPCIDevice", wrapInitWithPciInfo,
                    this->orgInitWithPciInfo},
                {"__ZN34AMDRadeonX6000_AmdRadeonController10doGPUPanicEPKcz", wrapDoGPUPanic},
            };
            PANIC_COND(!RouteRequestPlus::routeAll(patcher, id, requests, slide, size), "X6000FB",
                "Failed to route debug symbols");
        }

        if (checkKernelArgument("-NRXDMLogger")) {
            RouteRequestPlus request {"_dm_logger_write", wrapDmLoggerWrite, kDmLoggerWritePattern};
            PANIC_COND(!request.route(patcher, id, slide, size), "X6000FB", "Failed to route dm_logger_write");
        }

        PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "X6000FB",
            "Failed to enable kernel writing");

        *orgAsicCapsTable = {
            .familyId = AMDGPU_FAMILY_NAVI,
            .deviceId = NootRXMain::callback->deviceId,
            .revision = NootRXMain::callback->revision,
            .extRevision = static_cast<UInt32>(NootRXMain::callback->enumRevision) + NootRXMain::callback->revision,
            .pciRevision = NootRXMain::callback->pciRevision,
            .caps = ddiCapsNavi2Universal,
        };
        MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
        DBGLOG("X6000FB", "Applied DDI Caps patches");

        return true;
    }

    return false;
}

UInt16 X6000FB::wrapGetEnumeratedRevision() { return NootRXMain::callback->enumRevision; }

bool X6000FB::wrapInitWithPciInfo(void *that, void *param1) {
    auto ret = FunctionCast(wrapInitWithPciInfo, callback->orgInitWithPciInfo)(that, param1);
    // Hack AMDRadeonX6000_AmdLogger to log everything
    getMember<UInt64>(that, 0x28) = ~0ULL;
    getMember<UInt32>(that, 0x30) = 0xFF;
    return ret;
}

void X6000FB::wrapDoGPUPanic() {
    DBGLOG("X6000FB", "doGPUPanic << ()");
    while (true) { IOSleep(3600000); }
}

constexpr static const char *LogTypes[] = {"Error", "Warning", "Debug", "DC_Interface", "DTN", "Surface", "HW_Hotplug",
    "HW_LKTN", "HW_Mode", "HW_Resume", "HW_Audio", "HW_HPDIRQ", "MST", "Scaler", "BIOS", "BWCalcs", "BWValidation",
    "I2C_AUX", "Sync", "Backlight", "Override", "Edid", "DP_Caps", "Resource", "DML", "Mode", "Detect", "LKTN",
    "LinkLoss", "Underflow", "InterfaceTrace", "PerfTrace", "DisplayStats"};

void X6000FB::wrapDmLoggerWrite(void *, UInt32 logType, char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    auto *ns = new char[0x10000];
    vsnprintf(ns, 0x10000, fmt, args);
    va_end(args);
    const char *logTypeStr = arrsize(LogTypes) > logType ? LogTypes[logType] : "Info";
    kprintf("[%s] %s", logTypeStr, ns);
    delete[] ns;
}
