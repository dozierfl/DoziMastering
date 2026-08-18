add_executable(dozi_mastering_tests MasteringTests.cpp)
target_link_libraries(dozi_mastering_tests PRIVATE Dozi::Core)
target_compile_options(dozi_mastering_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
add_test(NAME dozi_mastering_tests COMMAND dozi_mastering_tests)
add_executable(dozi_processor_control_tests ProcessorControlTests.cpp)
target_link_libraries(dozi_processor_control_tests PRIVATE Dozi::Core)
target_compile_options(dozi_processor_control_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
add_test(NAME dozi_processor_control_tests COMMAND dozi_processor_control_tests)
add_executable(dozi_hardware_control_tests HardwareControlTests.cpp
    "${CMAKE_SOURCE_DIR}/src/mastering_app/MasteringHardwareControls.cpp"
    "${CMAKE_SOURCE_DIR}/src/mastering_app/MasteringModuleInspector.cpp")
target_include_directories(dozi_hardware_control_tests PRIVATE "${CMAKE_SOURCE_DIR}/src/mastering_app")
target_link_libraries(dozi_hardware_control_tests PRIVATE Dozi::Core DoziMasteringAssets
    juce::juce_gui_extra juce::juce_recommended_config_flags)
target_compile_definitions(dozi_hardware_control_tests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_compile_options(dozi_hardware_control_tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME dozi_hardware_control_tests COMMAND dozi_hardware_control_tests)
add_executable(dozi_renderer_audition_tests RendererAuditionTests.cpp)
target_link_libraries(dozi_renderer_audition_tests PRIVATE Dozi::Core)
target_compile_options(dozi_renderer_audition_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
add_test(NAME dozi_renderer_audition_tests COMMAND dozi_renderer_audition_tests)
add_test(NAME dozi_runtime_text_ascii
    COMMAND "${CMAKE_COMMAND}" -DSOURCE_ROOT=${CMAKE_SOURCE_DIR}
        -P "${CMAKE_CURRENT_SOURCE_DIR}/VerifyRuntimeText.cmake")
add_test(NAME dozi_explicit_render_only
    COMMAND "${CMAKE_COMMAND}" -DSOURCE_ROOT=${CMAKE_SOURCE_DIR}
        -P "${CMAKE_CURRENT_SOURCE_DIR}/VerifyExplicitRenderOnly.cmake")
