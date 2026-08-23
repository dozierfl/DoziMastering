file(READ "${SOURCE_ROOT}/src/mastering_app/MasteringComponent.cpp" component_source)
file(READ "${SOURCE_ROOT}/src/mastering_app/MasteringComponent.h" component_header)

if(NOT component_source MATCHES "liveProcessorChain\\.process\\(liveAudioBuffer,true")
    message(FATAL_ERROR "Playback must process source blocks through the live mastering chain")
endif()

if(component_source MATCHES "presetAudition" OR component_header MATCHES "presetAudition")
    message(FATAL_ERROR "Processor changes must not schedule an automatic offline render")
endif()

if(NOT component_source MATCHES "liveAudioBuffer.left.*beforePlaybackBuffer")
    message(FATAL_ERROR "The live processed path must originate from the imported premaster stream")
endif()

if(component_source MATCHES "glueAuditionEngine\\.process\\(afterPlaybackBuffer")
    message(FATAL_ERROR "Playback must not add a compressor pass to a previously rendered preview")
endif()
