#include "wxlm_compat.hpp"

namespace Climate53 { bool Install(); }

extern "C" __attribute__((used)) void WiiXLaunch_ModEntry() {
    WIIXL_LOG("climate53: entry point called");
    Climate53::Install();
}
