#pragma once

#include <cstdint>

namespace Climate53 {

constexpr uint32_t kStockClimateCount = 52;
constexpr uint32_t kClimateRecordSize = 0x178;

constexpr uint32_t kMaxClimateCount   = 128;
constexpr uint32_t kMaxExtraClimates  = kMaxClimateCount - kStockClimateCount;

// --- layout ------------------------------------------------------------------
enum class LayoutKind : uint8_t {
    AllocSize,
    TailBase,
    TailPlus8,
    TailStoreReg,
};

struct LayoutPatch {
    uintptr_t  offset;
    uint32_t   from;
    LayoutKind kind;
    const char* what;
};

constexpr LayoutPatch kLayoutPatches[] = {
    { 0x0D6881C, 0x52898E15, LayoutKind::AllocSize, "object allocation size"       },
    { 0x0D68844, 0x52898C08, LayoutKind::TailBase,  "ctor: tail base for the zero" },
    { 0x0D68854, 0xF92632DF, LayoutKind::TailStoreReg, "ctor: str xzr at the tail"    },
    { 0x099B2A0, 0x52898C08, LayoutKind::TailBase,  "tail float store"             },
    { 0x099B2B8, 0x52898D08, LayoutKind::TailPlus8, "tail +8 field address"        },
    { 0x099EA38, 0x52898C09, LayoutKind::TailBase,  "tail float load"              },
    { 0x099F260, 0x52898C09, LayoutKind::TailBase,  "tail float load"              },
};

// --- bounds ------------------------------------------------------------------
struct BoundPatch {
    uintptr_t offset;
    uint32_t  from;
    const char* what;
};

constexpr BoundPatch kBoundPatches[] = {
    { 0x07E85BC, 0x7100D01F, "getTemperatureByFieldMapArea clamp" },
    { 0x0802BE4, 0x7100D05F, "accessor clamp @0802be4"            },
    { 0x08C1D08, 0x7100D11F, "bound @08c1d08"                     },
    { 0x091B238, 0x7100CC3F, "ClimatePalette::loadResources"      },
    { 0x099B97C, 0x7100CD1F, "id guard @099b97c"                  },
    { 0x099BB24, 0x7100CD5F, "id guard @099bb24"                  },
    { 0x099BCB0, 0x7100CD7F, "id guard @099bcb0"                  },
    { 0x099CB38, 0x7100D11F, "bound @099cb38"                     },
    { 0x099EA58, 0x7100D13F, "bound @099ea58"                     },
    { 0x099EC1C, 0x7100D13F, "bound @099ec1c"                     },
    { 0x099EFE4, 0x7100D13F, "bound @099efe4"                     },
    { 0x099F24C, 0x7100D13F, "clamp @099f24c"                     },
    { 0x099F30C, 0x7100D13F, "clamp @099f30c"                     },
    { 0x0D6942C, 0x7100CF5F, "Climate::initialize upper limit"    },
    { 0x0D69534, 0xF100D35F, "Climate::initialize loop"           },
    { 0x0D6B8D0, 0xF100D11F, "ecocat climate name lookup"         },
    { 0x0DB428C, 0x7100CD1F, "onLoadGameData id bound 1"          },
    { 0x0DB4388, 0x7100CD1F, "onLoadGameData id bound 2"          },
    { 0x0DB4570, 0x7100D11F, "onLoadGameData loop"                },
    { 0x1219868, 0x7100D11F, "bound @1219868"                     },
    { 0x1219B40, 0x7100D03F, "Climate::getEnvEffect clamp"        },
    { 0x1B4D040, 0x7100D11F, "Climate::onSaveGameData loop"       },
    { 0x1B4D194, 0x7100D11F, "Climate::generateForecast loop"     },
    { 0x1B4D2EC, 0x7100D03F, "accessor clamp @1b4d2ec"            },
    { 0x1B4D30C, 0x7100D03F, "isEnableShootingStar clamp"         },
    { 0x1B4D340, 0x7100D05F, "accessor clamp @1b4d340"            },
    { 0x1B53ACC, 0x7100D27F, "bound @1b53acc"                     },
    { 0x1B53B20, 0x7100D27F, "bound @1b53b20"                     },
    { 0x1B54514, 0x7100D11F, "bound @1b54514"                     },
    { 0x287DE70, 0x7100CD7F, "getClimateByFieldMapArea"           },
};

struct SimplePatch { uintptr_t offset; uint32_t from; uint32_t to; const char* what; };
constexpr SimplePatch kSkipSaveRestorePatch = {
    0x0DB4234, 0xD101C3FF, 0xD65F03C0, "Climate::onLoadGameData -> ret"
};

namespace Offsets {
    constexpr uintptr_t Climate_NameTable_Global = 0x0463B258;
    constexpr uintptr_t Climate_NameTable_Rel    = 0x498;
    constexpr uintptr_t Climate_NameTable_Base   = 0x043EE3B8;
    constexpr uintptr_t Climate_NameTable_Slot0  = 0x043EE850;
}

} // namespace Climate53
