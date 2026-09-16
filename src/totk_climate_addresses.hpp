#pragma once

#include <cstdint>

namespace Climate53 {

// Index of the climate this mod adds (the 53rd, 0-based).
constexpr uint32_t kCustomClimateIndex = 52;

// One climate record is 0x178 bytes. Stock, the Climate object is allocated
// 0x4C70 bytes: 52 * 0x178 = 0x4C60 of record array, then a 13-byte tail member
// at +0x4C60 and 3 bytes of padding.
//
// This mod grows the object so the 53rd record is a real array entry rather
// than something bolted on the side:
//
//        stock                          patched
//   0x0000 .. 0x4C60  records 0-51   0x0000 .. 0x4DD8  records 0-52
//   0x4C60 .. 0x4C6D  tail member    0x4DD8 .. 0x4DE5  tail member
//   size 0x4C70                      size 0x4DE8
//
// Every accessor then works untouched, because index 52 is in range and lands
// on real memory. The tail member is referenced by exactly six instructions,
// all verified to reach the Climate object (via [parent+0x220] or as `this`).
constexpr uint32_t kClimateRecordSize = 0x178;
constexpr uint32_t kStockArraySize    = 52 * kClimateRecordSize; // 0x4C60
constexpr uint32_t kNewArraySize      = 53 * kClimateRecordSize; // 0x4DD8

struct Patch32 {
    uintptr_t offset;
    uint32_t  from;
    uint32_t  to;
    const char* what;
};

// --- layout -----------------------------------------------------------------
// Generated from the decompressed main NSO; `from` is the live word at rest, so
// every one of these is origin-checked against the real binary.
constexpr Patch32 kLayoutPatches[] = {
    { 0x0D6881C, 0x52898E15, 0x5289BD15, "object allocation size 0x4C70 -> 0x4DE8" },
    { 0x0D68844, 0x52898C08, 0x5289BB08, "ctor: tail base for the 13-byte zero"    },
    { 0x0D68854, 0xF92632DF, 0xF926EEDF, "ctor: str xzr at the tail"               },
    { 0x099B2A0, 0x52898C08, 0x5289BB08, "tail float store"                        },
    { 0x099B2B8, 0x52898D08, 0x5289BC08, "tail +8 field address"                   },
    { 0x099EA38, 0x52898C09, 0x5289BB09, "tail float load"                         },
    { 0x099F260, 0x52898C09, 0x5289BB09, "tail float load"                         },
};

// --- bounds -----------------------------------------------------------------
// Every climate index bound in the binary: found by scanning .text for the
// 0x178 record stride and correlating each nearby compare, plus the name lookup
// and palette guards which are not stride-adjacent.
//
// Five of these are the accessor clamp `index < 52 ? index : 0` (0x0802BE4,
// 0x1219B40, 0x1B4D2EC, 0x1B4D30C, 0x1B4D340) - that idiom is what silently
// turned climate 52 back into climate 0 and made the mod look inert.
//
// A bound missed here is benign: that path just keeps clamping to 0.
constexpr Patch32 kBoundPatches[] = {
    { 0x07E85BC, 0x7100D01F, 0x7100D41F, "bound @07e85bc"                  },
    { 0x0802BE4, 0x7100D05F, 0x7100D45F, "accessor clamp @0802be4"         },
    { 0x08C1D08, 0x7100D11F, 0x7100D51F, "bound @08c1d08"                  },
    { 0x091B238, 0x7100CC3F, 0x7100D03F, "ClimatePalette::loadResources"   },
    { 0x099CB38, 0x7100D11F, 0x7100D51F, "bound @099cb38"                  },
    { 0x099EA58, 0x7100D13F, 0x7100D53F, "bound @099ea58"                  },
    { 0x099EC1C, 0x7100D13F, 0x7100D53F, "bound @099ec1c"                  },
    { 0x099EFE4, 0x7100D13F, 0x7100D53F, "bound @099efe4"                  },
    { 0x0D6942C, 0x7100CF5F, 0x7100D35F, "Climate::initialize upper limit" },
    { 0x0D69534, 0xF100D35F, 0xF100D75F, "Climate::initialize loop"        },
    { 0x0D6B8D0, 0xF100D11F, 0xF100D51F, "ecocat climate name lookup"      },
    { 0x0DB428C, 0x7100CD1F, 0x7100D11F, "onLoadGameData id bound 1"       },
    { 0x0DB4388, 0x7100CD1F, 0x7100D11F, "onLoadGameData id bound 2"       },
    { 0x0DB4570, 0x7100D11F, 0x7100D51F, "onLoadGameData loop"             },
    { 0x1219868, 0x7100D11F, 0x7100D51F, "bound @1219868"                  },
    { 0x1219B40, 0x7100D03F, 0x7100D43F, "Climate::getEnvEffect clamp"     },
    { 0x1B4D040, 0x7100D11F, 0x7100D51F, "Climate::onSaveGameData loop"    },
    { 0x1B4D194, 0x7100D11F, 0x7100D51F, "Climate::generateForecast loop"  },
    { 0x1B4D2EC, 0x7100D03F, 0x7100D43F, "accessor clamp @1b4d2ec"         },
    { 0x1B4D30C, 0x7100D03F, 0x7100D43F, "isEnableShootingStar clamp"      },
    { 0x1B4D340, 0x7100D05F, 0x7100D45F, "accessor clamp @1b4d340"         },
    { 0x1B53ACC, 0x7100D27F, 0x7100D67F, "bound @1b53acc"                  },
    { 0x1B53B20, 0x7100D27F, 0x7100D67F, "bound @1b53b20"                  },
    { 0x1B54514, 0x7100D11F, 0x7100D51F, "bound @1b54514"                  },
};

namespace Offsets {
    // The climate name registry: a static array of 52 `const char*` in .data at
    // 0x043EE850, pointing into a string blob at 0x043EE3B8. Code reaches it as
    // [*(0x0463B258) + 0x498 + index*8]. Two null slots follow the 52 entries;
    // the first is index 52.
    //
    // This lives in .data, which the game already has mapped RW, so it is
    // outside the alias wiixl.patch writes code through - it needs a plain
    // store, not a patch.
    constexpr uintptr_t Climate_NameTable_Slot0  = 0x043EE850; // -> "HyrulePlainClimate"
    constexpr uintptr_t Climate_NameTable_Slot52 = 0x043EE9F0; // null at rest
}

} // namespace Climate53
