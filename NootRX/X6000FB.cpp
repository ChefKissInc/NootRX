// Copyright © 2023-2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include "X6000FB.hpp"
#include "NootRX.hpp"
#include "PatcherPlus.hpp"
#include <Headers/kern_api.hpp>

static const char *pathRadeonX6000Framebuffer =
    "/System/Library/Extensions/AMDRadeonX6000Framebuffer.kext/Contents/MacOS/AMDRadeonX6000Framebuffer";

static KernelPatcher::KextInfo kextRadeonX6000Framebuffer {
    "com.apple.kext.AMDRadeonX6000Framebuffer",
    &pathRadeonX6000Framebuffer,
    1,
    {},
    {},
    KernelPatcher::KextInfo::Unloaded,
};

X6000FB *X6000FB::callback = nullptr;

void X6000FB::init() {
    SYSLOG("X6000FB", "Module initialised");

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

        if (NootRXMain::callback->attributes.isNavi22()) {
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
                {"_dm_logger_write", wrapDmLoggerWrite, kDmLoggerWritePattern},
            };
            PANIC_COND(!RouteRequestPlus::routeAll(patcher, id, requests, slide, size), "X6000FB",
                "Failed to route debug symbols");
        }

        PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "X6000FB",
            "Failed to enable kernel writing");
        *orgAsicCapsTable = {
            .familyId = AMDGPU_FAMILY_NAVI,
            .deviceId = NootRXMain::callback->deviceID,
            .revision = NootRXMain::callback->devRevision,
            .extRevision = static_cast<UInt32>(NootRXMain::callback->enumRevision) + NootRXMain::callback->devRevision,
            .pciRevision = NootRXMain::callback->pciRevision,
            .caps = ddiCapsNavi2Universal,
        };
        MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
        DBGLOG("X6000FB", "Applied DDI Caps patches");

        if (ADDPR(debugEnabled)) {
            auto *logEnableMaskMinors =
                patcher.solveSymbol<void *>(id, "__ZN14AmdDalDmLogger19LogEnableMaskMinorsE", slide, size);
            patcher.clearError();

            if (logEnableMaskMinors == nullptr) {
                size_t offset = 0;
                PANIC_COND(!KernelPatcher::findPattern(kDalDmLoggerShouldLogPartialPattern,
                               kDalDmLoggerShouldLogPartialPatternMask, arrsize(kDalDmLoggerShouldLogPartialPattern),
                               reinterpret_cast<const void *>(slide), size, &offset),
                    "X6000FB", "Failed to solve LogEnableMaskMinors");
                auto *instAddr = reinterpret_cast<UInt8 *>(slide + offset);
                // inst + instSize + imm32 = addr
                logEnableMaskMinors = instAddr + 7 + *reinterpret_cast<SInt32 *>(instAddr + 3);
            }

            PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "X6000FB",
                "Failed to enable kernel writing");
            memset(logEnableMaskMinors, 0xFF, 0x80);    // Enable all DalDmLogger logs
            MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);

            // Enable all Display Core and BiosParserHelper logs
            const LookupPatchPlus patches[] = {
                {&kextRadeonX6000Framebuffer, kInitPopulateDcInitDataOriginal, kInitPopulateDcInitDataPatched, 1},
                {&kextRadeonX6000Framebuffer, kBiosParserHelperInitWithDataOriginal,
                    kBiosParserHelperInitWithDataPatched, 1},
            };
            PANIC_COND(!LookupPatchPlus::applyAll(patcher, patches, slide, size), "X6000FB",
                "Failed to apply debug enablement patches");
        }

        return true;
    }

    return false;
}

UInt16 X6000FB::wrapGetEnumeratedRevision() { return NootRXMain::callback->enumRevision; }

bool X6000FB::wrapInitWithPciInfo(void *that, void *pciDevice) {
    auto ret = FunctionCast(wrapInitWithPciInfo, callback->orgInitWithPciInfo)(that, pciDevice);
    getMember<UInt64>(that, 0x28) = 0xFFFFFFFFFFFFFFFF;    // Enable all log types
    getMember<UInt32>(that, 0x30) = 0xFF;                  // Enable all log severities
    return ret;
}

void X6000FB::wrapDoGPUPanic(void *, char const *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    auto *buf = static_cast<char *>(IOMalloc(1000));
    bzero(buf, 1000);
    vsnprintf(buf, 1000, fmt, va);
    va_end(va);

    DBGLOG("X6000FB", "doGPUPanic: %s", buf);
    IOSleep(10000);
    panic("%s", buf);
}

constexpr static const char *LogTypes[] = {
    "Error",
    "Warning",
    "Debug",
    "DC_Interface",
    "DTN",
    "Surface",
    "HW_Hotplug",
    "HW_LKTN",
    "HW_Mode",
    "HW_Resume",
    "HW_Audio",
    "HW_HPDIRQ",
    "MST",
    "Scaler",
    "BIOS",
    "BWCalcs",
    "BWValidation",
    "I2C_AUX",
    "Sync",
    "Backlight",
    "Override",
    "Edid",
    "DP_Caps",
    "Resource",
    "DML",
    "Mode",
    "Detect",
    "LKTN",
    "LinkLoss",
    "Underflow",
    "InterfaceTrace",
    "PerfTrace",
    "DisplayStats",
};

// Needed to prevent stack overflow
void X6000FB::wrapDmLoggerWrite(void *, const UInt32 logType, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    auto *message = static_cast<char *>(IOMalloc(0x1000));
    vsnprintf(message, 0x1000, fmt, va);
    va_end(va);
    auto *epilogue = message[strnlen(message, 0x1000) - 1] == '\n' ? "" : "\n";
    if (logType < arrsize(LogTypes)) {
        kprintf("[%s]\t%s%s", LogTypes[logType], message, epilogue);
    } else {
        kprintf("%s%s", message, epilogue);
    }
    IOFree(message, 0x1000);
}
