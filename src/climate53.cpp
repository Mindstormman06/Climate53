#include "wxlm_compat.hpp"
#include "totk_climate_addresses.hpp"

namespace Climate53 {

static const char kCustomClimate53Name[] = "CustomClimate53";

static constexpr bool kDiagnoseViaSlot0 = false;

// Stop the save file from restoring the old weather forecast over the one
// generated from this climate's rates. See kSkipSaveRestorePatch.
static constexpr bool kSkipClimateSaveRestore = true;

static bool ApplyAll(const Patch32* patches, uint32_t count, const char* group) {
    for (uint32_t i = 0; i < count; ++i) {
        const Patch32& p = patches[i];
        if (!WiiXLaunch::CodePatch::WriteChecked(p.offset, p.from, p.to)) {
            WIIXL_LOG("Climate53: %s patch failed at %p (%s)", group, reinterpret_cast<void*>(p.offset), p.what);
            return false;
        }
    }
    WIIXL_LOG("Climate53: %s - %d patch(es) applied", group, (int)count);
    return true;
}

bool Install() {
    WIIXL_LOG("Climate53: Installing TotK 1.2.1 53rd Climate mod...");

    if (!ApplyAll(kLayoutPatches, sizeof(kLayoutPatches) / sizeof(kLayoutPatches[0]), "layout")) {
        return false;
    }
    WIIXL_LOG("Climate53: record array 0x%X -> 0x%X bytes, tail member moved to +0x%X", (unsigned)kStockArraySize, (unsigned)kNewArraySize, (unsigned)kNewArraySize);

    {
        const uintptr_t slot0Addr  = WiiXLaunch::ResolveTarget(Offsets::Climate_NameTable_Slot0);
        const uintptr_t slot52Addr = WiiXLaunch::ResolveTarget(Offsets::Climate_NameTable_Slot52);
        if (!slot0Addr || !slot52Addr) {
            WIIXL_LOG("Climate53: could not resolve the climate name table!");
            return false;
        }

        const char** table = reinterpret_cast<const char**>(slot0Addr);
        const char** slot  = reinterpret_cast<const char**>(slot52Addr);

        if (*slot != nullptr) {
            WIIXL_LOG("Climate53: name table slot %d already holds %p - refusing to claim it!",
                      (int)kCustomClimateIndex, (const void*)*slot);
            return false;
        }
        *slot = kCustomClimate53Name;

        WIIXL_LOG("Climate53: name table slot 0  -> \"%s\"", table[0] ? table[0] : "<null>");
        WIIXL_LOG("Climate53: name table slot %d -> \"%s\"",
                  (int)kCustomClimateIndex, *slot ? *slot : "<null>");

        if (kDiagnoseViaSlot0) {
            table[0] = kCustomClimate53Name;
            WIIXL_LOG("Climate53: DIAGNOSTIC - slot 0 now reads \"%s\" too, so areas " "resolve to index 0 and index 52 is out of the picture", table[0]);
        }
    }

    if (!ApplyAll(kBoundPatches, sizeof(kBoundPatches) / sizeof(kBoundPatches[0]), "bounds")) {
        return false;
    }

    if (kSkipClimateSaveRestore) {
        if (!ApplyAll(&kSkipSaveRestorePatch, 1, "skip save-restore")) return false;
        WIIXL_LOG("Climate53: the saved weather forecast will NOT be restored - the "
                  "forecast generated from this climate stands");
    }

    WIIXL_LOG("Climate53: installed - climate %d is \"%s\"", (int)kCustomClimateIndex, kCustomClimate53Name);
    return true;
}

} // namespace Climate53
