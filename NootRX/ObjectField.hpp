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
#include <Headers/kern_util.hpp>

constexpr UInt32 InvalidOffset = 0xFFFFFFFF;

template<typename T>
class ObjectField {
    UInt32 offset {InvalidOffset};

    public:
    inline void operator=(const UInt32 offset) { this->offset = offset; }

    inline ObjectField<T> operator+(const UInt32 value) {
        PANIC_COND(this->offset == InvalidOffset, "ObjField", "value == InvalidOffset");
        ObjectField<T> ret {};
        ret.offset = this->offset + value;
        return ret;
    }

    inline T get(void *that) {
        PANIC_COND(that == nullptr, "ObjField", "that == nullptr");
        PANIC_COND(this->offset == InvalidOffset, "ObjField", "this->offset == InvalidOffset");
        return getMember<T>(that, this->offset);
    }

    inline T &getRef(void *that) {
        PANIC_COND(that == nullptr, "ObjField", "that == nullptr");
        PANIC_COND(this->offset == InvalidOffset, "ObjField", "this->offset == InvalidOffset");
        return getMember<T>(that, this->offset);
    }

    inline void set(void *that, T value) {
        PANIC_COND(that == nullptr, "ObjField", "that == nullptr");
        PANIC_COND(this->offset == InvalidOffset, "ObjField", "this->offset == InvalidOffset");
        getMember<T>(that, this->offset) = value;
    }
};
