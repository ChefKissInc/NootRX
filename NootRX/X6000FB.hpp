// Copyright (C) 2023-2026  ChefKiss.
//
// Licensed under the ChefKiss Open Knowledge License version 1.
//
// This work is licensed to you under the terms of the ChefKiss Open Knowledge
// License, either version 1 of the License, or (at your option) any later
// version published by ChefKiss.
//
// This work may not be used to train or develop artificial intelligence,
// and may not be used with artificial intelligence in any other way. See
// the ChefKiss Open Knowledge License for details.
//
// This work is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// ChefKiss Open Knowledge License for more details.
//
// You should have received a copy of the ChefKiss Open Knowledge License
// along with this work.  If not, see https://git.chefkiss.dev/ChefKiss/OpenKnowledge/raw/branch/master/LICENSE-v1.md.

#pragma once
#include <Headers/kern_patcher.hpp>
#include <Headers/kern_util.hpp>

class X6000FB {
    static X6000FB *callback;

    public:
    void init();
    bool processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size);

    private:
    mach_vm_address_t orgInitWithPciInfo {0};

    static UInt32 wrapGetEnumeratedRevision(void *that);
    static bool wrapInitWithPciInfo(void *that, void *pciDevice);
    static void wrapDoGPUPanic(void *that, char const *fmt, ...);
    static void wrapDmLoggerWrite(void *logger, const UInt32 logType, const char *fmt, ...);
};

//------ Patterns ------//

static const UInt8 kCailAsicCapsTablePattern[] = {0x6E, 0x00, 0x00, 0x00, 0x98, 0x67, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};

static const UInt8 kDmLoggerWritePattern[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41, 0x54,
    0x53, 0x48, 0x81, 0xEC, 0x88, 0x04, 0x00, 0x00};

static const UInt8 kDalDmLoggerShouldLogPartialPattern[] = {0x48, 0x8D, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x8B, 0x04, 0x81,
    0x0F, 0xA3, 0xD0, 0x0F, 0x92, 0xC0};
static const UInt8 kDalDmLoggerShouldLogPartialPatternMask[] = {0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

//------ Patches ------//

// Enable all Display Core logs
static const UInt8 kInitPopulateDcInitDataOriginal[] = {0x48, 0xB9, 0xDB, 0x1B, 0xFF, 0x7E, 0x10, 0x00, 0x00, 0x00};
static const UInt8 kInitPopulateDcInitDataPatched[] = {0x48, 0xB9, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Enable all AmdBiosParserHelper logs
static const UInt8 kBiosParserHelperInitWithDataOriginal[] = {0x08, 0xC7, 0x07, 0x01, 0x00, 0x00, 0x00};
static const UInt8 kBiosParserHelperInitWithDataPatched[] = {0x08, 0xC7, 0x07, 0xFF, 0x00, 0x00, 0x00};
