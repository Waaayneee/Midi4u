add_library(midi4u_core STATIC
    DeviceRules.cpp
    DeviceSupervisor.cpp
    Engine.cpp
    Mixer.cpp
    SharedMemory.cpp
    Sync.cpp
    TapProtocol.cpp
    TapPluginSide.cpp
    TapReader.cpp)

target_include_directories(midi4u_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(midi4u_core PUBLIC samplerate)

if(MSVC)
    target_compile_options(midi4u_core PRIVATE /W4)
else()
    target_compile_options(midi4u_core PRIVATE -Wall -Wextra -Wpedantic)
endif()
