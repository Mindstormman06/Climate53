#include "wxlm_compat.hpp"
#include "totk_climate_addresses.hpp"

namespace Climate53 {

static constexpr bool kSkipClimateSaveRestore = true;

// --- climates.ini ------------------------------------------------------------
static char        g_iniBuffer[4096];
static const char* g_names[kMaxExtraClimates];
static uint32_t    g_nameCount = 0;

static const char* g_nameTable[kMaxClimateCount];

static bool IsSpace(char c) { return c == ' ' || c == '\t' || c == '\r'; }

static bool LoadClimateList() {
    const int32_t read = WiiXLaunch::Surf::ModReadFile("climates.ini", g_iniBuffer, sizeof(g_iniBuffer) - 1);
    if (read <= 0) {
        WIIXL_LOG("Climate53: no climates.ini (or empty) - nothing to add");
        return true;
    }
    g_iniBuffer[read] = '\0';

    uint32_t i = 0;
    const uint32_t len = static_cast<uint32_t>(read);
    while (i < len) {
        while (i < len && (IsSpace(g_iniBuffer[i]) || g_iniBuffer[i] == '\n')) ++i;
        if (i >= len) break;

        if (g_iniBuffer[i] == '#' || g_iniBuffer[i] == ';') {
            while (i < len && g_iniBuffer[i] != '\n') ++i;
            continue;
        }

        const uint32_t start = i;
        while (i < len && g_iniBuffer[i] != '\n') ++i;
        uint32_t end = i;

        if (i < len) ++i;

        while (end > start && IsSpace(g_iniBuffer[end - 1])) --end;
        if (end == start) continue;

        if (g_nameCount >= kMaxExtraClimates) {
            WIIXL_LOG("Climate53: climates.ini lists more than %d climates (the build ceiling) - refusing", (int)kMaxExtraClimates);
            return false;
        }
        g_iniBuffer[end] = '\0';
        g_names[g_nameCount++] = &g_iniBuffer[start];
    }
    return true;
}

// --- instruction rewriting ---------------------------------------------------
static uint32_t MovzWithImm(uint32_t from, uint32_t imm16) {
    return (from & 0xFFE0001Fu) | (imm16 << 5);
}
static uint32_t CmpWithDelta(uint32_t from, uint32_t delta) {
    const uint32_t imm = ((from >> 10) & 0xFFFu) + delta;
    return (from & 0xFFC003FFu) | (imm << 10);
}

static bool ApplyLayout(uint32_t count) {
    const uint32_t arraySize = count * kClimateRecordSize;
    for (uint32_t i = 0; i < sizeof(kLayoutPatches) / sizeof(kLayoutPatches[0]); ++i) {
        const LayoutPatch& p = kLayoutPatches[i];
        uint32_t to = 0;
        switch (p.kind) {
            case LayoutKind::AllocSize:    to = MovzWithImm(p.from, arraySize + 0x10); break;
            case LayoutKind::TailBase:     to = MovzWithImm(p.from, arraySize);        break;
            case LayoutKind::TailPlus8:    to = MovzWithImm(p.from, arraySize + 8);    break;
            case LayoutKind::TailStoreReg: to = 0xF900011Fu;                           break;
        }
        if (!WiiXLaunch::CodePatch::WriteChecked(p.offset, p.from, to)) {
            WIIXL_LOG("Climate53: layout patch failed at %p (%s)",
                      reinterpret_cast<void*>(p.offset), p.what);
            return false;
        }
    }
    WIIXL_LOG("Climate53: record array -> 0x%X bytes for %d climates, tail at +0x%X", (unsigned)arraySize, (int)count, (unsigned)arraySize);
    return true;
}

static bool ApplyBounds(uint32_t delta) {
    const uint32_t n = sizeof(kBoundPatches) / sizeof(kBoundPatches[0]);
    for (uint32_t i = 0; i < n; ++i) {
        const BoundPatch& p = kBoundPatches[i];
        if (!WiiXLaunch::CodePatch::WriteChecked(p.offset, p.from, CmpWithDelta(p.from, delta))) {
            WIIXL_LOG("Climate53: bound patch failed at %p (%s)", reinterpret_cast<void*>(p.offset), p.what);
            return false;
        }
    }
    WIIXL_LOG("Climate53: bounds - %d site(s) raised by %d", (int)n, (int)delta);
    return true;
}

static bool RelocateNameTable() {
    const uintptr_t globalAddr = WiiXLaunch::ResolveTarget(Offsets::Climate_NameTable_Global);
    const uintptr_t expected   = WiiXLaunch::ResolveTarget(Offsets::Climate_NameTable_Base);
    if (!globalAddr || !expected) {
        WIIXL_LOG("Climate53: could not resolve the climate name table!");
        return false;
    }

    uintptr_t* global = reinterpret_cast<uintptr_t*>(globalAddr);
    if (*global != expected) {
        WIIXL_LOG("Climate53: name table global holds %p, expected %p - refusing to relocate something this is not", (void*)*global, (void*)expected);
        return false;
    }

    const char* const* stock = reinterpret_cast<const char* const*>(*global + Offsets::Climate_NameTable_Rel);
    for (uint32_t i = 0; i < kStockClimateCount; ++i) g_nameTable[i] = stock[i];


    WIIXL_LOG("Climate53: stock slot 0 -> \"%s\"", g_nameTable[0] ? g_nameTable[0] : "<null>");

    for (uint32_t i = 0; i < g_nameCount; ++i) {
        g_nameTable[kStockClimateCount + i] = g_names[i];
        WIIXL_LOG("Climate53: climate %d = \"%s\"", (int)(kStockClimateCount + i), g_names[i]);
    }

    *global = reinterpret_cast<uintptr_t>(g_nameTable) - Offsets::Climate_NameTable_Rel;
    WIIXL_LOG("Climate53: name table relocated to %p (%d entries)", (void*)g_nameTable, (int)(kStockClimateCount + g_nameCount));
    return true;
}

bool Install() {
    WIIXL_LOG("Climate53: Installing TotK 1.2.1 custom climate mod...");

    if (!LoadClimateList()) return false;
    if (g_nameCount == 0) {
        WIIXL_LOG("Climate53: no climates configured - leaving the game stock");
        return true;
    }

    const uint32_t count = kStockClimateCount + g_nameCount;   // N
    const uint32_t delta = g_nameCount;

    if (!ApplyLayout(count)) return false;

    if (!RelocateNameTable()) return false;

    if (!ApplyBounds(delta)) return false;

    if (kSkipClimateSaveRestore) {
        const SimplePatch& p = kSkipSaveRestorePatch;
        if (!WiiXLaunch::CodePatch::WriteChecked(p.offset, p.from, p.to)) {
            WIIXL_LOG("Climate53: could not skip the saved forecast restore (%s)", p.what);
            return false;
        }
        WIIXL_LOG("Climate53: the saved weather forecast will NOT be restored");
    }

    WIIXL_LOG("Climate53: installed - %d climates (%d stock + %d added)",
              (int)count, (int)kStockClimateCount, (int)g_nameCount);
    return true;
}

} // namespace Climate53
