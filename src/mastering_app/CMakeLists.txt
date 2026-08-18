juce_add_gui_app(DoziMastering PRODUCT_NAME "Dozi Mastering" COMPANY_NAME "Dozi"
    BUNDLE_ID "com.dozi.mastering" VERSION "${PROJECT_VERSION}" MICROPHONE_PERMISSION_ENABLED FALSE
    ICON_BIG "${CMAKE_SOURCE_DIR}/assets/icons/dozi-mastering-1024.png")
juce_add_binary_data(DoziMasteringAssets SOURCES
    "${CMAKE_SOURCE_DIR}/assets/hardware/corrective-eq-skin.png"
    "${CMAKE_SOURCE_DIR}/assets/hardware/dynamic-eq-skin.png"
    "${CMAKE_SOURCE_DIR}/assets/hardware/de-esser-skin.png"
    "${CMAKE_SOURCE_DIR}/assets/hardware/glue-compressor-skin.png"
    "${CMAKE_SOURCE_DIR}/assets/hardware/multiband-compressor-skin.png"
    "${CMAKE_SOURCE_DIR}/assets/hardware/saturation-skin.png"
    "${CMAKE_SOURCE_DIR}/assets/hardware/width-mono-bass-skin.png"
    "${CMAKE_SOURCE_DIR}/assets/hardware/true-peak-limiter-skin.png")
target_sources(DoziMastering PRIVATE Main.cpp MasteringComponent.cpp MasteringComponent.h
    MasteringHardwareControls.cpp MasteringHardwareControls.h
    MasteringModuleInspector.cpp MasteringModuleInspector.h
    MasteringWaveformView.cpp MasteringWaveformView.h)
target_link_libraries(DoziMastering PRIVATE Dozi::Core DoziMasteringAssets juce::juce_gui_extra juce::juce_audio_formats juce::juce_audio_utils
    juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
target_compile_definitions(DoziMastering PRIVATE JUCE_APPLICATION_NAME_STRING="Dozi Mastering"
    JUCE_APPLICATION_VERSION_STRING="${PROJECT_VERSION}" JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
