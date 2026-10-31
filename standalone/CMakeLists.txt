cmake_minimum_required(VERSION 3.20)
project(DywxStandalone LANGUAGES CXX)

# Deliberately exclude the UBT module entry point and all Runtime/UI sources.
set(DYWX_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")
add_library(DywxCore STATIC
    "${DYWX_ROOT}/Source/DywxCore/Private/MVP/Arena.cpp"
    "${DYWX_ROOT}/Source/DywxCore/Private/MVP/Campaign.cpp"
    "${DYWX_ROOT}/Source/DywxCore/Private/MVP/ContentCatalog.cpp"
    "${DYWX_ROOT}/Source/DywxCore/Private/MVP/Economy.cpp"
    "${DYWX_ROOT}/Source/DywxCore/Private/MVP/Progression.cpp"
    "${DYWX_ROOT}/Source/DywxCore/Private/MVP/SaveCodec.cpp"
    "${DYWX_ROOT}/Source/DywxCore/Private/MVP/Simulation.cpp"
)
add_library(Dywx::Core ALIAS DywxCore)
target_compile_features(DywxCore PUBLIC cxx_std_20)
target_compile_definitions(DywxCore PUBLIC DYWXCORE_STANDALONE=1)
target_include_directories(DywxCore PUBLIC "${DYWX_ROOT}/Source/DywxCore/Public")
set_target_properties(DywxCore PROPERTIES CXX_EXTENSIONS OFF)

option(DYWX_BUILD_HEADLESS "Build the existing natural full-game simulation" ON)
if(DYWX_BUILD_HEADLESS)
    add_executable(dywx_headless "${DYWX_ROOT}/tests/full_game_playthrough.cpp")
    target_link_libraries(dywx_headless PRIVATE Dywx::Core)
    set_target_properties(dywx_headless PROPERTIES CXX_EXTENSIONS OFF)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(core_command_tests "${DYWX_ROOT}/tests/core_command_tests.cpp")
    target_link_libraries(core_command_tests PRIVATE Dywx::Core)
    # This existing test suite uses assert, including command calls in asserts.
    # Keep those checks active even in Release/RelWithDebInfo configurations.
    if(MSVC)
        target_compile_options(core_command_tests PRIVATE /UNDEBUG)
    else()
        target_compile_options(core_command_tests PRIVATE -UNDEBUG)
    endif()
    set_target_properties(core_command_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME core_command_tests COMMAND core_command_tests)
endif()
