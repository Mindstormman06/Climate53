#include "wxlm_compat.hpp"
#include "totk_climate_addresses.hpp"

namespace Climate53 {

// Name of the 53rd climate. Four things have to agree on this string:
//   - the name registered in the game's climate name table (see Install),
//   - the "Climate" value in the ecocat entries inside Bootup.pack,
//   - romfs/WorldMgr/ResClimate/CustomClimate53.game__wm__ResClimate.bgyml,
//   - the RESTBL entry for that path.
static const char kCustomClimate53Name[] = "CustomClimate53";

// Climate::onLoadGameData restores the weather forecast from the save file,
// overwriting the one Climate::initialize just generated from this climate's
// rates. Without this, temperature and wind take effect immediately but the
// weather keeps following whatever schedule the save was carrying.
//
// The cost is that the forecast no longer persists across save/load - it is
// re-rolled on each load rather than continuing the stored schedule.
static constexpr bool kSkipClimateSaveRestore = true;

static bool ApplyAll(const Patch32* patches, uint32_t count, const char* group) {
    for (uint32_t i = 0; i < count; ++i) {
        const Patch32& p = patches[i];
        if (!WiiXLaunch::CodePatch::WriteChecked(p.offset, p.from, p.to)) {
            WIIXL_LOG("Climate53: %s patch failed at %p (%s)",
                      group, reinterpret_cast<void*>(p.offset), p.what);
            return false;
        }
    }
    WIIXL_LOG("Climate53: %s - %d patch(es) applied", group, (int)count);
    return true;
}

bool Install() {
    WIIXL_LOG("Climate53: Installing TotK 1.2.1 53rd Climate mod...");

    // 1. Grow the Climate object so record 52 is a real array entry, and move
    //    the tail member out of the way. This must land before any bound is
    //    raised: a 53rd index into a 52-entry array runs off the end of the
    //    allocation and smashes the next heap block.
    if (!ApplyAll(kLayoutPatches,
                  sizeof(kLayoutPatches) / sizeof(kLayoutPatches[0]), "layout")) {
        return false;
    }
    WIIXL_LOG("Climate53: record array 0x%X -> 0x%X bytes, tail member moved to +0x%X",
              (unsigned)kStockArraySize, (unsigned)kNewArraySize, (unsigned)kNewArraySize);

    // 2. Register the climate name in slot 52 of the game's name table.
    //    That table lives in .data, which is already mapped RW and therefore
    //    sits outside the alias wiixl.patch writes code through - it needs a
    //    plain store, so the origin check is done here instead.
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

        // Slot 0 is logged as a landmark: if it is not "HyrulePlainClimate"
        // the table base is wrong and nothing below means what it says.
        WIIXL_LOG("Climate53: name table slot 0  -> \"%s\"", table[0] ? table[0] : "<null>");
        WIIXL_LOG("Climate53: name table slot %d -> \"%s\"",
                  (int)kCustomClimateIndex, *slot ? *slot : "<null>");
    }

    // 3. Raise every place the game caps a climate index at 52.
    if (!ApplyAll(kBoundPatches,
                  sizeof(kBoundPatches) / sizeof(kBoundPatches[0]), "bounds")) {
        return false;
    }

    // 4. Let the generated forecast survive loading a save.
    if (kSkipClimateSaveRestore) {
        if (!ApplyAll(&kSkipSaveRestorePatch, 1, "skip save-restore")) return false;
        WIIXL_LOG("Climate53: the saved weather forecast will NOT be restored - the "
                  "forecast generated from this climate stands");
    }

    WIIXL_LOG("Climate53: installed - climate %d is \"%s\"",
              (int)kCustomClimateIndex, kCustomClimate53Name);
    return true;
}

} // namespace Climate53
