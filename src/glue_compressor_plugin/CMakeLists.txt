juce_add_plugin(DoziGlueCompressor
    COMPANY_NAME "Dozi"
    BUNDLE_ID "com.dozi.gluecompressor"
    PLUGIN_MANUFACTURER_CODE Dozi
    PLUGIN_CODE Dgcp
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "DOZI Glue Compressor"
    DESCRIPTION "DOZI stereo bus and glue compressor"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(DoziGlueCompressor PRIVATE
    GlueCompressorEngine.cpp GlueCompressorEngine.h
    PluginProcessor.cpp PluginProcessor.h
    PluginEditor.cpp PluginEditor.h)
target_link_libraries(DoziGlueCompressor PRIVATE
    juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
target_compile_definitions(DoziGlueCompressor PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
