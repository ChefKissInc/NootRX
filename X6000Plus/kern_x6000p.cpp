//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#include "kern_x6000p.hpp"
#include "kern_patcherplus.hpp"
#include "kern_patches.hpp"
#include <Headers/kern_api.hpp>
#include <Headers/kern_devinfo.hpp>

static const char *pathAGDP = "/System/Library/Extensions/AppleGraphicsControl.kext/Contents/PlugIns/"
                              "AppleGraphicsDevicePolicy.kext/Contents/MacOS/AppleGraphicsDevicePolicy";
							  
static KernelPatcher::KextInfo kextAGDP {"com.apple.driver.AppleGraphicsDevicePolicy", &pathAGDP, 1, {true}, {},
    KernelPatcher::KextInfo::Unloaded};

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
		auto model = "AMD Radeon Graphics" // fallback value
        switch (this->deviceId) {
			case 0x73AF:
				model = "AMD Radeon RX 6900 XTXH";
				break;
			case 0x73A5:
				model = "AMD Radeon RX 6950 XT";
				break;
			case 0x73EF:
				model = "AMD Radeon RX 6650 XT";
				break;
			default:
				PANIC("x6000p", "Unknown device ID")
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
	static const uint8_t kAGDPBoardIDKeyOriginal[] = "board-id";
	static const uint8_t kAGDPBoardIDKeyPatched[] = "applehax";
    if (kextAGDP.loadIndex == index) {
        const LookupPatchPlus patches[] = {
            {&kextAGDP, kAGDPBoardIDKeyOriginal, kAGDPBoardIDKeyPatched, 1},
        };
        PANIC_COND(!LookupPatchPlus::applyAll(&patcher, patches, address, size), "x6000p",
            "Failed to apply AGDP patches: %d", patcher.getError());
	}
}
