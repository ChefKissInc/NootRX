//  Copyright © 2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5. See LICENSE for
//  details.

#include "kern_x6000fb.hpp"
#include "kern_patterns.hpp"
#include "kern_x6000p.hpp"
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
        X6000P::callback->setRMMIOIfNecessary();

        CAILAsicCapsEntry *orgAsicCapsTable = nullptr;

        SolveRequestPlus solveRequests[] = {
            {"__ZL20CAIL_ASIC_CAPS_TABLE", orgAsicCapsTable, kCailAsicCapsTablePattern},
        };
        PANIC_COND(!SolveRequestPlus::solveAll(patcher, id, solveRequests, slide, size), "x6000fb",
            "Failed to resolve symbols");

        RouteRequestPlus requests[] = {
            {"__ZNK32AMDRadeonX6000_AmdAsicInfoNavi2127getEnumeratedRevisionNumberEv", wrapGetEnumeratedRevision},
            {"__ZN24AMDRadeonX6000_AmdLogger15initWithPciInfoEP11IOPCIDevice", wrapInitWithPciInfo,
                this->orgInitWithPciInfo, ADDPR(debugEnabled)},
            {"__ZN34AMDRadeonX6000_AmdRadeonController10doGPUPanicEPKcz", wrapDoGPUPanic, ADDPR(debugEnabled)},
            {"_dm_logger_write", wrapDmLoggerWrite, kDmLoggerWritePattern, checkKernelArgument("-x6kpdmlogger")},
        };
        PANIC_COND(!RouteRequestPlus::routeAll(patcher, id, requests, slide, size), "x6000fb",
            "Failed to route symbols");

        PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "x6000fb",
            "Failed to enable kernel writing");

        *orgAsicCapsTable = {
            .familyId = 0x8F,
            // Navi 23 uses Navi 22 caps, we also assume the same for Navi 24 here
            .caps = X6000P::callback->chipType == ChipType::Navi21 ? ddiCapsNavi21 : ddiCapsNavi22,
            .deviceId = X6000P::callback->deviceId,
            .revision = X6000P::callback->revision,
            .extRevision = static_cast<uint32_t>(X6000P::callback->enumRevision) + X6000P::callback->revision,
            .pciRevision = X6000P::callback->pciRevision,
        };
        MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
        DBGLOG("x6000fb", "Applied DDI Caps patches");

        return true;
    }

    return false;
}

uint16_t X6000FB::wrapGetEnumeratedRevision() { return X6000P::callback->enumRevision; }

bool X6000FB::wrapInitWithPciInfo(void *that, void *param1) {
    auto ret = FunctionCast(wrapInitWithPciInfo, callback->orgInitWithPciInfo)(that, param1);
    // Hack AMDRadeonX6000_AmdLogger to log everything
    getMember<uint64_t>(that, 0x28) = ~0ULL;
    getMember<uint32_t>(that, 0x30) = 0xFF;
    return ret;
}

void X6000FB::wrapDoGPUPanic() {
    DBGLOG("x6000fb", "doGPUPanic << ()");
    while (true) { IOSleep(3600000); }
}

constexpr static const char *LogTypes[] = {"Error", "Warning", "Debug", "DC_Interface", "DTN", "Surface", "HW_Hotplug",
    "HW_LKTN", "HW_Mode", "HW_Resume", "HW_Audio", "HW_HPDIRQ", "MST", "Scaler", "BIOS", "BWCalcs", "BWValidation",
    "I2C_AUX", "Sync", "Backlight", "Override", "Edid", "DP_Caps", "Resource", "DML", "Mode", "Detect", "LKTN",
    "LinkLoss", "Underflow", "InterfaceTrace", "PerfTrace", "DisplayStats"};

void X6000FB::wrapDmLoggerWrite(void *, uint32_t logType, char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    auto *ns = new char[0x10000];
    vsnprintf(ns, 0x10000, fmt, args);
    va_end(args);
    const char *logTypeStr = arrsize(LogTypes) > logType ? LogTypes[logType] : "Info";
    kprintf("[%s] %s", logTypeStr, ns);
    delete[] ns;
}
