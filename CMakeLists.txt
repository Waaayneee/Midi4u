cmake_minimum_required(VERSION 3.22)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version")

project(Midi4u VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(MIDI4U_BUILD_JUCE "Build the Midi4u Tap plugin and the Midi4u App" ON)
option(MIDI4U_BUILD_TESTS "Build the unit and integration tests" ON)
option(MIDI4U_RTSAN "Build with Clang RealtimeSanitizer (needs upstream LLVM 20 or newer)" OFF)

if(MIDI4U_RTSAN)
    add_compile_options(-fsanitize=realtime)
    add_link_options(-fsanitize=realtime)
    add_compile_definitions(MIDI4U_RTSAN=1)
endif()

# libsamplerate: only the library, none of its tests or examples
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(LIBSAMPLERATE_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LIBSAMPLERATE_INSTALL OFF CACHE BOOL "" FORCE)
set(LIBSAMPLERATE_ENABLE_SINC_MEDIUM_CONVERTER OFF CACHE BOOL "" FORCE)
set(LIBSAMPLERATE_ENABLE_SINC_BEST_CONVERTER OFF CACHE BOOL "" FORCE)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5) # libsamplerate 0.2.2 asks for an older CMake than CMake 4 accepts
add_subdirectory(libs/libsamplerate EXCLUDE_FROM_ALL)
unset(CMAKE_POLICY_VERSION_MINIMUM)

add_subdirectory(core)

if(MIDI4U_BUILD_JUCE)
    add_subdirectory(libs/JUCE)
    add_subdirectory(plugin)
    add_subdirectory(app)
endif()

if(MIDI4U_BUILD_TESTS)
    add_subdirectory(libs/Catch2 EXCLUDE_FROM_ALL)
    enable_testing()
    add_subdirectory(tests)
endif()
