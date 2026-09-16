#pragma once

#include <wiixlaunch/mod_log.h>          // WIIXL_LOG
#include <wiixlaunch/mod_runtime.h>      // memcpy/memset/strcmp for freestanding

#include <wiixlaunch/imports/wiixl_core.h>
#include <wiixlaunch/imports/wiixl_call.h>
#include <wiixlaunch/imports/wiixl_patch.h>

#include <cstdint>
#include <cstddef>

namespace WiiXLaunch {

namespace Surf {
WXL_USE_wiixl_core(InstallHook);
WXL_USE_wiixl_call(ResolveTarget);
WXL_USE_wiixl_patch(Write);
} // namespace Surf

// ResolveTarget(offset) for TOTK (Switch-only).
inline uintptr_t ResolveTarget(uintptr_t switchOffset) {
    return Surf::ResolveTarget(switchOffset, 0);
}

template <typename Fn>
inline Fn GetTargetFunction(uintptr_t switchOffset) {
    return reinterpret_cast<Fn>(Surf::ResolveTarget(switchOffset, 0));
}

struct CodePatch {
    static bool Write(uintptr_t targetOffset, const void* data, uint32_t size) {
        const uintptr_t addr = ResolveTarget(targetOffset);
        if (!addr) return false;
        uint8_t present[16];
        if (size > sizeof(present)) return false;
        const uint8_t* at = reinterpret_cast<const uint8_t*>(addr);
        for (uint32_t i = 0; i < size; ++i) present[i] = at[i];
        return Surf::Write(addr, data, size, present, size) != 0;
    }

    template <typename T>
    static bool WriteValue(uintptr_t targetOffset, T value) {
        return Write(targetOffset, &value, sizeof(T));
    }

    template <typename T>
    static bool WriteChecked(uintptr_t targetOffset, T expected, T value) {
        const uintptr_t addr = ResolveTarget(targetOffset);
        if (!addr) return false;
        return Surf::Write(addr, &value, sizeof(T), &expected, sizeof(T)) != 0;
    }

    static bool Nop(uintptr_t targetOffset) {
        return WriteValue<uint32_t>(targetOffset, 0xD503201Fu);   // aarch64 NOP
    }
};

namespace impl {

template <class T>
struct HookCommon {
    inline static uintptr_t s_orig = 0;

    static bool Install(uintptr_t switchOffset) {
        const uintptr_t target = Surf::ResolveTarget(switchOffset, 0);
        if (!target) {
            WIIXL_LOG("Climate53: hook target unresolved (sw=%p)",
                      reinterpret_cast<void*>(switchOffset));
            return false;
        }
        s_orig = Surf::InstallHook(target,
                                   reinterpret_cast<uintptr_t>(&T::Callback));
        if (!s_orig) {
            WIIXL_LOG("Climate53: InstallHook refused at %p",
                      reinterpret_cast<void*>(target));
            return false;
        }
        return true;
    }
};

template <class T>
struct TrampolineHookBase : HookCommon<T> {
    template <class Self = T, class... A>
    static auto Orig(A... args) -> decltype(Self::Callback(args...)) {
        using Fn = decltype(&Self::Callback);
        if (!HookCommon<T>::s_orig) return decltype(Self::Callback(args...))();
        return reinterpret_cast<Fn>(HookCommon<T>::s_orig)(args...);
    }
};

template <class T>
struct ReplaceHookBase : HookCommon<T> {};

} // namespace impl
} // namespace WiiXLaunch

#define WIIXL_HOOK_DEFINE_TRAMPOLINE(name) \
    struct name : ::WiiXLaunch::impl::TrampolineHookBase<name>

#define WIIXL_HOOK_DEFINE_REPLACE(name) \
    struct name : ::WiiXLaunch::impl::ReplaceHookBase<name>
