// Copyright (C) 2022-2026  ChefKiss.
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
#include <Headers/kern_util.hpp>

struct FWMetadata {
    const UInt8 *data;
    const UInt32 length;
};

struct FWDescriptor {
    const char *name;
    const FWMetadata metadata;
};

extern const struct FWDescriptor firmware[];
extern const size_t firmwareCount;

inline const FWMetadata &getFWByName(const char *name) {
    for (size_t i = 0; i < firmwareCount; i++) {
        if (strcmp(firmware[i].name, name)) { continue; }

        return firmware[i].metadata;
    }
    PANIC("FW", "'%s' not found", name);
}
