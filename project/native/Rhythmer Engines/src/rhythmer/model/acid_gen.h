// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "rhythmer/model/pattern.h"
#include <cstdint>
namespace rhythmer {
// FM-1 SIX_GEN defaults and ranges. Seed is a full per-track TB-3PO stream.
struct AcidGenSettings {
    uint8_t density=70,accent=40,slide=25,octaves=2,octave=1,auto_bars=0;
    uint32_t seed=1;
};
Pattern acid_generate_pattern(const Pattern&, AcidGenSettings&, bool mutate, unsigned root, uint16_t scale);
}
