//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#include "kern_x6000p.hpp"
#include "kern_patcherplus.hpp"
#include "kern_patches.hpp"
#include <Headers/kern_api.hpp>
#include <Headers/kern_devinfo.hpp>

static const char *pathAGDP = "/System/Library/Extensions/AppleGraphicsControl.kext/Contents/PlugIns/"
                              "AppleGraphicsDevicePolicy.kext/Contents/MacOS/AppleGraphicsDevicePolicy";

static const char *pathRadeonX6000Framebuffer =
    "/System/Library/Extensions/AMDRadeonX6000Framebuffer.kext/Contents/MacOS/AMDRadeonX6000Framebuffer";

static KernelPatcher::KextInfo kextAGDP {"com.apple.driver.AppleGraphicsDevicePolicy", &pathAGDP, 1, {true}, {},
    KernelPatcher::KextInfo::Unloaded};

static KernelPatcher::KextInfo kextRadeonX6000Framebuffer {"com.apple.kext.AMDRadeonX6000Framebuffer",
    &pathRadeonX6000Framebuffer, 1, {}, {}, KernelPatcher::KextInfo::Unloaded};

X6000P *X6000P::callback = nullptr;

void X6000P::init() {
	SYSLOG("x6000p", "Copyright 2022-2023 ChefKiss Inc. If you've paid for this, you've been scammed.");
	callback = this;

	lilu.onKextLoadForce(&kextAGDP);

	lilu.onPatcherLoadForce(
		[](void *user, KernelPatcher &patcher) { static_cast<X6000P *>(user)->processPatcher(patcher); }, this);
	lilu.onKextLoadForce(
		nullptr, 0,
		[](void *user, KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
			static_cast<X6000P *>(user)->processKext(patcher, index, address, size);
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
            if (device) {
                snprintf(name, arrsize(name), "GFX%zu", ii++);
                WIOKit::renameDevice(device, name);
                WIOKit::awaitPublishing(device);
            }
        }

        static uint8_t builtin[] = {0x00};
        this->GPU->setProperty("built-in", builtin, arrsize(builtin));
        this->deviceId = WIOKit::readPCIConfigValue(this->GPU, WIOKit::kIOPCIConfigDeviceID);
		this->pciRevision = WIOKit::readPCIConfigValue(NRed::callback->iGPU, WIOKit::kIOPCIConfigRevisionID);

        this->rmmio = this->GPU->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress5);
        PANIC_COND(UNLIKELY(!this->rmmio || !this->rmmio->getLength()), "x6000p", "Failed to map RMMIO");
        this->rmmioPtr = reinterpret_cast<uint32_t *>(this->rmmio->getVirtualAddress());

        this->revision = (this->readReg32(0xD31) & 0xF000000) >> 0x18;
        switch (this->deviceId) {
			case 0x73A5:
				this->chipType = ChipType::Navi21;
				this->enumRevision = 0x28; // NV_SIENNA_CICHLID_P_A0 = 40
				model = "AMD Radeon RX 6950 XT";
				break;
			case 0x73EF:
				this->chipType = ChipType::Navi23;
				this->enumRevision = 0x3c; // NV_DIMGREY_CAVEFISH_P_A0 = 60
				model = "AMD Radeon RX 6650 XT";
				break;
			default:
				PANIC("x6000p", "Unknown device ID");
		}
		
		if (model) {
			auto len = static_cast<uint32_t>(strlen(model) + 1);
			this->GPU->setProperty("model", const_cast<char *>(model), len);
			this->GPU->setProperty("ATY,FamilyName", const_cast<char *>("Radeon"), 7);
			this->GPU->setProperty("ATY,DeviceName", const_cast<char *>(model) + 11, len - 11);
		}
		DeviceInfo::deleter(devInfo);
	} else {
		SYSLOG("x6000p", "Failed to create DeviceInfo");
	}
}

void X6000P::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	auto boardId = BaseDeviceInfo::get().boardIdentifier;
	bool agdpboardid = true;
	const char *compatibleBoards[] {
		"Mac-27AD2F918AE68F61", // MacPro7,1
		"Mac-7BA5B2D9E42DDD94" // iMacPro1,1
	};
	for (size_t i = 0; i < arrsize(compatibleBoards); i++) {
		if (!strcmp(compatibleBoards[i], boardId)) {
			agdpboardid = false;
		}
	}

	static const uint8_t kAGDPBoardIDKeyOriginal[] = "board-id";
	static const uint8_t kAGDPBoardIDKeyPatched[] = "applehax";
    if (kextAGDP.loadIndex == index) {
        const LookupPatchPlus patches[] = {
            {&kextAGDP, kAGDPBoardIDKeyOriginal, kAGDPBoardIDKeyPatched, 1, agdpboardid},
        };
        PANIC_COND(!LookupPatchPlus::applyAll(&patcher, patches, address, size), "x6000p",
            "Failed to apply AGDP patch: %d", patcher.getError());
	}
	else if (kextRadeonX6000Framebuffer.loadIndex == index) {
		CAILAsicCapsEntry *orgAsicCapsTable = nullptr;

        SolveRequestPlus solveRequests[] = {
            {"__ZL20CAIL_ASIC_CAPS_TABLE", orgAsicCapsTable, kCailAsicCapsTablePattern},
        };
        PANIC_COND(!SolveRequestPlus::solveAll(&patcher, index, solveRequests, address, size), "x6000p",
            "Failed to resolve symbols");

		PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "x6000p",
            "Failed to enable kernel writing");

        *orgAsicCapsTable = {
            .familyId = 0x8F,
            .caps = this->chipType == ChipType::Navi21 ? ddiCapsNavi21 : ddiCapsNavi22, // Navi 23 uses Navi 22 caps
            .deviceId = this->deviceId,
            .revision = this->revision,
            .extRevision = static_cast<uint32_t>(this->enumRevision) + this->revision,
            .pciRevision = this->pciRevision,
        };
		MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
        DBGLOG("x6000p", "Applied DDI Caps patches");
	}
}
