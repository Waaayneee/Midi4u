#pragma once

// Marks functions that run on an audio thread. Place it after the parameter list, like noexcept.
// With MIDI4U_RTSAN, RealtimeSanitizer flags any allocation, lock or blocking call inside them.
#if defined(MIDI4U_RTSAN)
    #define MIDI4U_NONBLOCKING [[clang::nonblocking]]
#else
    #define MIDI4U_NONBLOCKING
#endif
