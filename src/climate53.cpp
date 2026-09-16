#include "wxlm_compat.hpp"
#include "totk_climate_addresses.hpp"

namespace Climate53 {

// Name of the 53rd climate. Three things have to agree on this string:
//   - the name registered in the game's climate name table (see Install),
//   - the "Climate" value in the ecocat entries inside Bootup.pack,
//   - romfs/WorldMgr/ResClimate/CustomClimate53.game__wm__ResClimate.bgyml,
// which is what Climate::initialize builds from the registered name.
static const char kCustomClimate53Name[] = "CustomClimate53";

// DIAGNOSTIC - set false to ship.
//
// Also points name table slot 0 ("HyrulePlainClimate") at our climate. The
// ecocat name lookup scans from index 0 upwards and takes the first match, so
// every area naming CustomClimate53 resolves to index 0 instead of 52. That
// takes the entire 53rd-climate mechanism out of the picture while keeping the
// data side - gyml, RSTB, repacked pack, resource load - exactly as it is.
//
//   storm appears  -> the data side is sound; the fault is specific to index 52
//   nothing        -> the data side never reaches the game, and the index work
//                     is irrelevant until that is fixed
static constexpr bool kDiagnoseViaSlot0 = true;

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
    //    the 13-byte tail member out of the way. This has to land before any
    //    bound is raised - a 53rd index into a 52-entry array walks off the end
    //    of the allocation and smashes the next heap block.
    if (!ApplyAll(kLayoutPatches,
                  sizeof(kLayoutPatches) / sizeof(kLayoutPatches[0]),
                  "layout")) {
        return false;
    }
    WIIXL_LOG("Climate53: record array 0x%X -> 0x%X bytes, tail member moved to +0x%X",
              (unsigned)kStockArraySize, (unsigned)kNewArraySize, (unsigned)kNewArraySize);

    // 2. Register the climate name in slot 52 of the game's name table. Without
    //    this, "CustomClimate53" in the ecocat matches nothing and every area
    //    silently keeps the index it already had.
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

        // Slot 0 is printed as a landmark: if it is not "HyrulePlainClimate"
        // the table base is wrong and nothing below means what it says.
        WIIXL_LOG("Climate53: name table slot 0  -> \"%s\"", table[0] ? table[0] : "<null>");
        WIIXL_LOG("Climate53: name table slot %d -> \"%s\"",
                  (int)kCustomClimateIndex, *slot ? *slot : "<null>");

        if (kDiagnoseViaSlot0) {
            table[0] = kCustomClimate53Name;
            WIIXL_LOG("Climate53: DIAGNOSTIC - slot 0 now reads \"%s\" too, so areas "
                      "resolve to index 0 and index 52 is out of the picture",
                      table[0]);
        }
    }

    // 3. Raise every climate index bound, including the five accessor clamps
    //    that were quietly rewriting index 52 to index 0.
    if (!ApplyAll(kBoundPatches,
                  sizeof(kBoundPatches) / sizeof(kBoundPatches[0]),
                  "bounds")) {
        return false;
    }

    WIIXL_LOG("Climate53: installed - climate %d is \"%s\"",
              (int)kCustomClimateIndex, kCustomClimate53Name);
    return true;
}

} // namespace Climate53
