//! Copyright © 2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5.
//! See LICENSE for details.

#pragma once
#include <Headers/kern_util.hpp>

struct Model {
    UInt16 rev {0};
    const char *name {nullptr};
};

struct DevicePair {
    UInt16 dev;
    const Model *models;
    size_t modelNum;
};

static constexpr Model dev73A2[] = {
    {0x00, "AMD Radeon Pro W6900X"},
};

static constexpr Model dev73A3[] = {
    {0x00, "AMD Radeon Pro W6800"},
};

static constexpr Model dev73A5[] = {
    {0xC0, "AMD Radeon RX 6950 XT"},
};

static constexpr Model dev73AB[] = {
    {0x00, "AMD Radeon Pro W6800X"},
};

static constexpr Model dev73AF[] = {
    {0xC0, "AMD Radeon RX 6900 XT"},
};

static constexpr Model dev73BF[] = {
    {0xC0, "AMD Radeon RX 6900 XT"},
    {0xC1, "AMD Radeon RX 6800 XT"},
    {0xC3, "AMD Radeon RX 6800"},
};

static constexpr Model dev73DF[] = {
    {0xC0, "AMD Radeon RX 6750 XT"},
    {0xC1, "AMD Radeon RX 6700 XT"},
    {0xC2, "AMD Radeon RX 6800M"},
    {0xC3, "AMD Radeon RX 6800M"},
    {0xC5, "AMD Radeon RX 6700 XT"},
    {0xCF, "AMD Radeon RX 6700M"},
    {0xDF, "AMD Radeon RX 6700"},
    {0xE5, "AMD Radeon RX 6750 GRE"},
    {0xFF, "AMD Radeon RX 6700"},
};

static constexpr Model dev73E0[] = {
    {0x00, "AMD Radeon Pro W6600X"},
};

static constexpr Model dev73E1[] = {
    {0x00, "AMD Radeon Pro W6600M"},
};

static constexpr Model dev73E3[] = {
    {0x00, "AMD Radeon Pro W6600"},
};

static constexpr Model dev73EF[] = {
    {0xC0, "AMD Radeon RX 6800S"},
    {0xC1, "AMD Radeon RX 6650 XT"},
    {0xC2, "AMD Radeon RX 6700S"},
    {0xC3, "AMD Radeon RX 6650M"},
    {0xC4, "AMD Radeon RX 6650M XT"},
};

static constexpr Model dev73FF[] = {
    {0xC1, "AMD Radeon RX 6600 XT"},
    {0xC3, "AMD Radeon RX 6600M"},
    {0xC7, "AMD Radeon RX 6600"},
    {0xCB, "AMD Radeon RX 6600S"},
};

static constexpr DevicePair devices[] = {
    {0x73A2, dev73A2, arrsize(dev73A2)},
    {0x73A3, dev73A3, arrsize(dev73A3)},
    {0x73A5, dev73A5, arrsize(dev73A5)},
    {0x73AB, dev73AB, arrsize(dev73AB)},
    {0x73AF, dev73AF, arrsize(dev73AF)},
    {0x73BF, dev73BF, arrsize(dev73BF)},
    {0x73DF, dev73DF, arrsize(dev73DF)},
    {0x73E0, dev73E0, arrsize(dev73E0)},
    {0x73E1, dev73E1, arrsize(dev73E1)},
    {0x73E3, dev73E3, arrsize(dev73E3)},
    {0x73EF, dev73EF, arrsize(dev73EF)},
    {0x73FF, dev73FF, arrsize(dev73FF)},
};

inline const char *getBranding(UInt16 dev, UInt16 rev) {
    for (auto &device : devices) {
        if (device.dev == dev) {
            for (size_t i = 0; i < device.modelNum; i++) {
                auto &model = device.models[i];
                if (model.rev == rev) { return model.name; }
            }
            break;
        }
    }
    return "AMD Radeon RX 6000 Series";
}
