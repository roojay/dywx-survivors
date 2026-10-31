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
    # Reuse the production-linked regression programs on Linux and Windows.
    # Natural-play sampling is intentionally a separate, slower command.
    set(DYWX_CORE_TESTS
        core_command_tests
        full_content_tests
        full_spell_behavior_tests
        full_game_integration_tests
        mvp_acceptance
        mvp_arena_tests
        mvp_campaign_tests
        mvp_discovery_test
        mvp_economy_tests
        mvp_economy_boundary_tests
        mvp_owned_spell_tests
        mvp_proc_tests
        mvp_progression_test
        mvp_save_tests
        mvp_save_boundary_tests
        mvp_simulation_tests
        combat_boundary_regressions
        simulation_design_regressions
        opportunity_flow_regressions
        account_pity_regressions
        damage_feedback_regressions
        checkpoint_replay_regressions
    )
    foreach(test_name IN LISTS DYWX_CORE_TESTS)
        add_executable(${test_name} "${DYWX_ROOT}/tests/${test_name}.cpp")
        target_link_libraries(${test_name} PRIVATE Dywx::Core)
        # Existing suites use assert, including command calls in assertions.
        # Release and RelWithDebInfo must execute the same checks as Debug.
        if(MSVC)
            target_compile_options(${test_name} PRIVATE /UNDEBUG)
        else()
            target_compile_options(${test_name} PRIVATE -UNDEBUG)
        endif()
        set_target_properties(${test_name} PROPERTIES CXX_EXTENSIONS OFF)
        add_test(NAME ${test_name} COMMAND ${test_name})
        set_tests_properties(${test_name} PROPERTIES LABELS "core;regression")
    endforeach()
endif()
