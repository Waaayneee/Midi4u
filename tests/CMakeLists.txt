add_executable(midi4u_tests
    DeviceRulesTests.cpp
    IntegrationTests.cpp
    MixerTests.cpp
    RingBufferTests.cpp
    SyncTests.cpp
    TapProtocolTests.cpp)

target_link_libraries(midi4u_tests PRIVATE midi4u_core Catch2::Catch2WithMain)

add_test(NAME unit COMMAND midi4u_tests "[unit]")
add_test(NAME integration COMMAND midi4u_tests "[integration]")
